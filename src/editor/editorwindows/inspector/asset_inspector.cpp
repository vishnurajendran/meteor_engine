// asset_inspector.cpp

#include "asset_inspector.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/audio/interfaces/audioclip_interface.h"
#include "core/engine/audio/interfaces/engine_interface.h"
#include "core/engine/skybox/cubemapasset.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/utils/logger.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/material_properties_controls.h"
#include "imgui-SFML.h"
#include "imgui.h"

#include <algorithm>
#include <cstdio>

#include "editor/audio_helpers/audio_waveform.h"
#include "editor/editorassetmanager/editorassetmanager.h"

std::map<SString, MMaterialPropertyControl*> MAssetInspector::propControlCache;
std::map<SString, MAssetInspector::SCubemapFaceControls> MAssetInspector::cubemapFaceCache;
std::map<SString, MAudioWaveform*> MAssetInspector::waveformCache;
MAssetInspector::SAudioPreviewState MAssetInspector::audioPreview;

// Number of bins for waveform peak reduction. 512 gives good resolution
// for typical inspector widths (300-500 px) without excessive memory.
static constexpr int WAVEFORM_BINS = 512;

// -- dispatch -----------------------------------------------------------------

void MAssetInspector::draw(MAsset* asset)
{
    if (!asset) return;

    // If the inspected asset changed while a preview was playing, release it.
    if (audioPreview.source && asset->getAssetId() != audioPreview.activeAssetId)
        releaseAudioPreview();

    if (auto* mat = dynamic_cast<MMaterialAsset*>(asset))
        drawMaterialAsset(mat);
    else if (auto* cube = dynamic_cast<MCubemapAsset*>(asset))
        drawCubemapAsset(cube);
    else if (auto* clip = dynamic_cast<MAudioClipAsset*>(asset))
        drawAudioClipAsset(clip);
    else if (auto* tex = dynamic_cast<MTextureAsset*>(asset))
        drawTextureAsset(tex);
    else if (auto* txt = dynamic_cast<MTextAsset*>(asset))
        drawTextAsset(txt);
    else if (auto* shd = dynamic_cast<MShaderAsset*>(asset))
        drawShaderAsset(shd);
    else if (auto* mesh = dynamic_cast<MStaticMeshAsset*>(asset))
        drawStaticMeshAsset(mesh);
    else
        drawGenericAsset(asset);
}

// -- audio clip ---------------------------------------------------------------

void MAssetInspector::drawAudioClipAsset(MAudioClipAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.85f, 0.75f, 1.f));
    ImGui::TextUnformatted("Audio Clip Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 120.f;
    if (ImGui::BeginTable("##audioclip_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:", asset->getName().c_str());
        row("Path:", asset->getPath().c_str());

        // -- Metadata from the runtime clip -----------------------------------
        IAudioClip* clip = asset->getAudioClip();
        if (clip)
        {
            char buf[64];

            float len = clip->getLength();
            if (len >= 60.f)
            {
                int mins = static_cast<int>(len) / 60;
                float secs = len - static_cast<float>(mins * 60);
                std::snprintf(buf, sizeof(buf), "%d:%05.2f", mins, secs);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), "%.2f s", len);
            }
            row("Duration:", buf);

            std::snprintf(buf, sizeof(buf), "%u", clip->getNoOfChannels());
            row("Channels:", buf);

            std::snprintf(buf, sizeof(buf), "%u Hz", clip->getSampleRate());
            row("Sample Rate:", buf);

            std::snprintf(buf, sizeof(buf), "%llu",
                          static_cast<unsigned long long>(clip->getFrameCount()));
            row("Frames:", buf);
        }
        else
        {
            row("Info:", "(clip not loaded)");
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // -- Waveform -------------------------------------------------------------
    {
        const SString& id = asset->getAssetId();
        MAudioWaveform*& wf = waveformCache[id];

        // Kick off async generation on first view.
        if (!wf)
            wf = MAudioWaveform::generate(asset->getPath(), WAVEFORM_BINS);

        drawWaveform(wf);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Preload toggle -------------------------------------------------------
    {
        bool preload = asset->isPreloaded();
        if (ImGui::Checkbox("Preload Audio", &preload))
        {
            asset->setPreloaded(preload);

            if (audioPreview.source)
                releaseAudioPreview();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Preload decodes the entire file into memory up front.\n"
                "Streaming reads from disk during playback (lower memory, "
                "slightly higher CPU).");
    }

    ImGui::Spacing();

    // -- Save -----------------------------------------------------------------

    if (ImGui::Button("Save##audioclip_save", ImVec2(-FLT_MIN, 0)))
    {
        if (!asset->save())
            ImGui::OpenPopup("##audioclip_save_err");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Persist the preload setting to the .meta file");

    if (ImGui::BeginPopup("##audioclip_save_err"))
    {
        ImGui::TextUnformatted("Failed to save audio clip settings. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Preview player -------------------------------------------------------

    const bool isPlaying = audioPreview.source != nullptr;

    if (!isPlaying)
    {
        if (ImGui::Button("Play##audioclip_play", ImVec2(-FLT_MIN, 0)))
        {
            auto* engine = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();
            if (engine)
            {
                IAudioClip* clip = asset->getAudioClip();
                if (clip)
                {
                    IAudioSource* src = engine->createAudioSource();
                    if (src)
                    {
                        src->setClip(clip);
                        src->setVolume(audioPreview.volume);
                        src->setLooping(false);
                        src->play();

                        audioPreview.source        = src;
                        audioPreview.engine        = engine;
                        audioPreview.activeAssetId = asset->getAssetId();
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                                   "Audio engine subsystem not available.");
            }
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.15f, 0.15f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.75f, 0.20f, 0.20f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.40f, 0.10f, 0.10f, 1.00f));
        if (ImGui::Button("Stop##audioclip_stop", ImVec2(-FLT_MIN, 0)))
            releaseAudioPreview();
        ImGui::PopStyleColor(3);

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##audioclip_vol", &audioPreview.volume,
                             0.005f, 0.0f, 1.0f, "Volume: %.2f"))
        {
            audioPreview.volume = std::clamp(audioPreview.volume, 0.0f, 1.0f);
            if (audioPreview.source)
                audioPreview.source->setVolume(audioPreview.volume);
        }
    }
}

// -- waveform rendering -------------------------------------------------------

void MAssetInspector::drawWaveform(MAudioWaveform* waveform)
{
    if (!waveform)
        return;

    if (!waveform->isReady())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
        ImGui::TextUnformatted("Generating waveform...");
        ImGui::PopStyleColor();
        return;
    }

    if (waveform->hasFailed())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("Could not generate waveform.");
        ImGui::PopStyleColor();
        return;
    }

    const auto& mins     = waveform->getMinPeaks();
    const auto& maxs     = waveform->getMaxPeaks();
    const int   bins     = waveform->getBinCount();
    const int   channels = waveform->getChannelCount();
    if (bins == 0 || channels == 0)
        return;

    // Total height scales with channel count. Each channel gets its own
    // strip so the user can see per-channel detail (stereo spread, etc.).
    static constexpr float CHANNEL_H = 48.f;
    static constexpr float GAP       = 2.f;
    const float totalH = static_cast<float>(channels) * CHANNEL_H
                       + static_cast<float>(channels - 1) * GAP;

    const float avail = ImGui::GetContentRegionAvail().x;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // One colour per channel so they're visually distinct.
    // Falls back to the first colour for channels beyond the palette.
    static constexpr ImU32 channelColors[] = {
        IM_COL32( 70, 210, 180, 220),   // teal   (Left / Mono)
        IM_COL32(100, 160, 240, 220),   // blue   (Right)
        IM_COL32(220, 180,  70, 220),   // gold   (Center)
        IM_COL32(200, 100, 200, 220),   // purple (LFE)
        IM_COL32( 80, 200, 120, 220),   // green  (Surround L)
        IM_COL32(230, 120,  90, 220),   // orange (Surround R)
    };
    static constexpr int paletteSize = sizeof(channelColors) / sizeof(channelColors[0]);

    static const char* channelLabels[] = {
        "L", "R", "C", "LFE", "SL", "SR"
    };
    static constexpr int labelCount = sizeof(channelLabels) / sizeof(channelLabels[0]);

    for (int ch = 0; ch < channels; ++ch)
    {
        const float stripY = origin.y + static_cast<float>(ch) * (CHANNEL_H + GAP);

        // Dark background per channel strip.
        dl->AddRectFilled(
            { origin.x, stripY },
            { origin.x + avail, stripY + CHANNEL_H },
            IM_COL32(20, 20, 20, 255), 4.f);

        const float centerY = stripY + CHANNEL_H * 0.5f;
        const float halfH   = CHANNEL_H * 0.45f;
        const ImU32 color   = channelColors[ch % paletteSize];

        for (int i = 0; i < bins; ++i)
        {
            const float t  = static_cast<float>(i) / static_cast<float>(bins);
            const float x  = origin.x + t * avail;
            const float y0 = centerY - maxs[ch][i] * halfH;
            const float y1 = centerY - mins[ch][i] * halfH;
            dl->AddLine({ x, y0 }, { x, y1 }, color);
        }

        // Subtle center line.
        dl->AddLine({ origin.x, centerY },
                    { origin.x + avail, centerY },
                    IM_COL32(255, 255, 255, 20));

        // Channel label in the top-left corner of each strip.
        const char* label = (ch < labelCount) ? channelLabels[ch] : "?";
        // Mono files get no label -- there's only one strip, it's obvious.
        if (channels > 1)
        {
            dl->AddText({ origin.x + 4.f, stripY + 2.f },
                        IM_COL32(255, 255, 255, 90), label);
        }
    }

    // Reserve layout space for the entire waveform block.
    ImGui::Dummy({ avail, totalH });
}

void MAssetInspector::releaseAudioPreview()
{
    if (!audioPreview.source)
        return;

    IAudioEngineSubsystem* engine = audioPreview.engine;
    if (!engine)
        engine = MEngineSubsystemRegistry::getSubsystem<IAudioEngineSubsystem>();

    if (engine)
        engine->releaseAudioSource(audioPreview.source);

    audioPreview.source        = nullptr;
    audioPreview.clip          = nullptr;
    audioPreview.engine        = nullptr;
    audioPreview.activeAssetId.clear();
}

// -- material -----------------------------------------------------------------

void MAssetInspector::drawMaterialAsset(MMaterialAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.f));
    ImGui::TextUnformatted("Material Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##mat_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:",   asset->getName().c_str());
        row("Shader:", asset->getShaderPath().c_str());
        row("Mode:",   asset->getShadingMode() == MMaterial::ShadingMode::Lit ? "Lit" : "Unlit");
        row("Path:",   asset->getPath().c_str());

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // -- Thumbnail preview -- shows the pipeline-rendered sphere at full size
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
        if (editorAM)
        {
            sf::Texture* thumb = editorAM->getThumbnail(asset);
            if (!thumb)
                editorAM->requestThumbnail(asset);

            if (thumb)
            {
                auto sfSize      = thumb->getSize();
                float avail      = ImGui::GetContentRegionAvail().x;
                float previewSize = std::min(avail, static_cast<float>(sfSize.x));

                float indent = (avail - previewSize) * 0.5f;
                if (indent > 0.f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

                ImVec2 tl = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(tl.x - 1, tl.y - 1),
                    ImVec2(tl.x + previewSize + 1, tl.y + previewSize + 1),
                    IM_COL32(60, 60, 60, 255));

                ImGui::Image(*thumb, ImVec2(previewSize, previewSize));
            }
        }
    }

    ImGui::Spacing();

    if (!asset->getMaterial())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("Material not built yet (shader may be missing).");
        ImGui::PopStyleColor();
        return;
    }

    const SString& id = asset->getAssetId();
    if (!propControlCache.contains(id))
        propControlCache[id] = new MMaterialPropertyControl();

    auto* ctrl = propControlCache[id];

    const int propCount = (int)asset->getMaterial()->getProperties().size();
    if (ctrl->getLastPropertyCount() != propCount)
    {
        ctrl->clearTextureReferences();
        ctrl->setLastPropertyCount(propCount);
    }

    ctrl->draw(asset);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save##mat_save", ImVec2(-FLT_MIN, 0)))
    {
        if (asset->save())
        {
            // Evict the stale thumbnail and re-queue so the asset browser
            // reflects the updated material properties.
            auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
            if (editorAM)
                editorAM->invalidateThumbnail(asset);
        }
        else
        {
            ImGui::OpenPopup("##save_err");
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write current properties back to %s", asset->getPath().c_str());

    if (ImGui::BeginPopup("##save_err"))
    {
        ImGui::TextUnformatted("Failed to save material. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// -- texture ------------------------------------------------------------------

void MAssetInspector::drawTextureAsset(MTextureAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.5f, 1.f, 1.f));
    ImGui::TextUnformatted("Texture Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Bumped from 100 to 120 so "Compression:" does not clip
    constexpr float LW = 120.f;
    if (ImGui::BeginTable("##tex_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:", asset->getName().c_str());
        row("Path:", asset->getPath().c_str());

        auto* tex = asset->getTexture();
        if (tex && tex->getCoreTexture())
        {
            auto size = tex->getCoreTexture()->getSize();
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%u x %u", size.x, size.y);
            row("Size:", buf);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();

    // -- Preview -- fits to available width, preserves aspect ratio
    auto* tex = asset->getTexture();
    if (tex && tex->getCoreTexture())
    {
        auto sfSize    = tex->getCoreTexture()->getSize();
        float avail    = ImGui::GetContentRegionAvail().x;
        float aspect   = sfSize.x > 0 ? (float)sfSize.y / (float)sfSize.x : 1.f;
        float previewW = std::min(avail, 256.f);
        float previewH = previewW * aspect;

        // Center the preview horizontally within the available region
        float indent = (avail - previewW) * 0.5f;
        if (indent > 0.f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

        // Subtle border around the preview so it reads against dark backgrounds
        ImVec2 tl = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(tl.x - 1, tl.y - 1),
            ImVec2(tl.x + previewW + 1, tl.y + previewH + 1),
            IM_COL32(60, 60, 60, 255));

        ImGui::Image(*tex->getCoreTexture(), ImVec2(previewW, previewH));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("Texture not loaded.");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Import settings -- all changes are in-memory until Save & Reimport
    if (ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("##tex_import", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LW);
            ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

            // Filter Min -- controls how the texture is sampled when minified.
            // Mipmap modes require glGenerateMipmap; loadWithSettings handles this.
            {
                static const char* labels[] = {
                    "Nearest",
                    "Linear",
                    "Nearest Mipmap Nearest",
                    "Linear Mipmap Nearest",
                    "Nearest Mipmap Linear",
                    "Linear Mipmap Linear"
                };
                int cur = static_cast<int>(asset->getFilterMin());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Filter Min:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##filterMin", &cur, labels, IM_ARRAYSIZE(labels)))
                    asset->setFilterMin(static_cast<ETextureFilterMin>(cur));
            }

            // Filter Mag -- controls how the texture is sampled when magnified.
            // Only Nearest and Linear are valid GL mag filters.
            {
                static const char* labels[] = { "Nearest", "Linear" };
                int cur = static_cast<int>(asset->getFilterMag());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Filter Mag:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##filterMag", &cur, labels, IM_ARRAYSIZE(labels)))
                    asset->setFilterMag(static_cast<ETextureFilterMag>(cur));
            }

            // Wrap S -- horizontal wrap mode
            {
                static const char* labels[] = {
                    "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
                };
                int cur = static_cast<int>(asset->getWrapS());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Wrap S:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##wrapS", &cur, labels, IM_ARRAYSIZE(labels)))
                    asset->setWrapS(static_cast<ETextureWrap>(cur));
            }

            // Wrap T -- vertical wrap mode
            {
                static const char* labels[] = {
                    "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
                };
                int cur = static_cast<int>(asset->getWrapT());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Wrap T:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##wrapT", &cur, labels, IM_ARRAYSIZE(labels)))
                    asset->setWrapT(static_cast<ETextureWrap>(cur));
            }

            // Max Import Size -- images above this are CPU-downscaled at load time
            {
                int maxSize = asset->getMaxImportSize();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Max Size:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputInt("##maxSize", &maxSize, 128, 1024))
                {
                    if (maxSize < 0) maxSize = 0;
                    asset->setMaxImportSize(maxSize);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(
                        "Maximum dimension in pixels (0 = no limit).\n"
                        "Textures larger than this are downscaled on import.");
            }

            // Compression -- re-uploads to a compressed internal format on the GPU
            {
                static const char* labels[] = {
                    "None",
                    "DXT1 (BC1, no alpha)",
                    "DXT5 (BC3, with alpha)"
                };
                int cur = static_cast<int>(asset->getCompression());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Compression:");
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##compression", &cur, labels, IM_ARRAYSIZE(labels)))
                    asset->setCompression(static_cast<ETextureCompression>(cur));
            }

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Save & Reimport -- persists to .meta, then reloads the GL texture
    if (ImGui::Button("Save & Reimport##tex_save", ImVec2(-FLT_MIN, 0)))
    {
        if (asset->saveImportSettings())
        {
            if (!asset->requestReload())
                MWARN("Texture reimport failed for: " + asset->getPath());
        }
        else
        {
            ImGui::OpenPopup("##tex_save_err");
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write import settings to .meta and reload the texture on the GPU");

    if (ImGui::BeginPopup("##tex_save_err"))
    {
        ImGui::TextUnformatted("Failed to save texture settings. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// -- cubemap ------------------------------------------------------------------

void MAssetInspector::drawCubemapAsset(MCubemapAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 0.9f, 1.f));
    ImGui::TextUnformatted("Cubemap Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##cube_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:", asset->getName().c_str());
        row("Path:", asset->getPath().c_str());

        ImGui::EndTable();
    }

    ImGui::Spacing();

    const SString& id = asset->getAssetId();
    if (!cubemapFaceCache.contains(id))
    {
        SCubemapFaceControls ctrls;
        for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
        {
            ctrls.faces[i] = new MAssetReferenceControl();
            ctrls.faces[i]->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> a)
            { return dynamic_cast<MTextureAsset*>(a.get()) != nullptr; };
        }
        cubemapFaceCache[id] = ctrls;
    }

    auto& ctrls = cubemapFaceCache[id];

    static const char* faceDisplayNames[] = {
        "Right (+X):", "Left (-X):", "Top (+Y):",
        "Bottom (-Y):", "Back (+Z):", "Front (-Z):"
    };

    if (ImGui::CollapsingHeader("Cubemap Faces", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("##cube_faces", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LW);
            ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

            for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
            {
                SString facePath = asset->getFacePath(i);
                TAssetHandle<MAsset> expected = !facePath.empty()
                    ? MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MTextureAsset>(facePath)
                    : TAssetHandle<MTextureAsset>();
                if (ctrls.faces[i]->getAssetReference() != expected)
                    ctrls.faces[i]->setAssetReference(expected);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(faceDisplayNames[i]);
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                ImGui::PushID(i);
                if (ctrls.faces[i]->drawControl(""))
                {
                    const auto newRef = ctrls.faces[i]->getAssetReference();
                    asset->setFacePath(i,
                        newRef ? newRef->getPath() : SString(""));
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save & Rebuild##cube_save", ImVec2(-FLT_MIN, 0)))
    {
        if (asset->save())
        {
            if (!asset->requestReload())
                MWARN("Cubemap rebuild failed -- check face paths in the log.");
        }
        else
        {
            ImGui::OpenPopup("##cube_save_err");
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Save cubemap XML and rebuild GPU cubemap texture");

    if (ImGui::BeginPopup("##cube_save_err"))
    {
        ImGui::TextUnformatted("Failed to save cubemap. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// -- shader -------------------------------------------------------------------

static const char* shaderPropertyTypeName(SShaderPropertyType type)
{
    switch (type)
    {
        case Bool:        return "Bool";
        case Int:         return "Int";
        case Float:       return "Float";
        case UniformVec2: return "Vec2";
        case UniformVec3: return "Vec3";
        case UniformVec4: return "Vec4";
        case Color:       return "Color";
        case Texture:     return "Texture";
        case Matrix4:     return "Matrix4";
        default:          return "Unknown";
    }
}

void MAssetInspector::drawShaderAsset(MShaderAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.7f, 1.0f, 1.f));
    ImGui::TextUnformatted("Shader Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    MShader* shader = asset->getShader();

    ImGui::Text("Name:  %s", asset->getName().c_str());
    ImGui::Text("Path:  %s", asset->getPath().c_str());

    // Valid/invalid badge
    if (shader)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.f));
        ImGui::TextUnformatted("\xe2\x97\x8f Compiled");  // filled circle UTF-8
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.f));
        ImGui::TextUnformatted("\xe2\x97\x8f Compile Error");
        ImGui::PopStyleColor();
    }

    if (!shader) return;

    ImGui::Spacing();

    // Read-only property list
    const auto& props = shader->getProperties();
    const auto& order = shader->getPropertyOrder();

    if (props.empty())
    {
        ImGui::TextDisabled("No properties declared.");
        return;
    }

    if (ImGui::CollapsingHeader("Shader Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("##shader_props", 2,
                               ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthFixed, 70.f);
            ImGui::TableHeadersRow();

            for (const auto& key : order)
            {
                auto it = props.find(key);
                if (it == props.end()) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(key.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("%s", shaderPropertyTypeName(it->second.getType()));
            }

            ImGui::EndTable();
        }
    }
}

// -- static mesh --------------------------------------------------------------

static void formatCount(char* buf, size_t bufSize, int64_t n)
{
    if      (n >= 1'000'000'000'000LL) std::snprintf(buf, bufSize, "%.1fT", static_cast<double>(n) / 1e12);
    else if (n >= 1'000'000'000LL)     std::snprintf(buf, bufSize, "%.1fB", static_cast<double>(n) / 1e9);
    else if (n >= 1'000'000LL)         std::snprintf(buf, bufSize, "%.1fM", static_cast<double>(n) / 1e6);
    else                               std::snprintf(buf, bufSize, "%lld",  static_cast<long long>(n));
}

void MAssetInspector::drawStaticMeshAsset(MStaticMeshAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.4f, 1.f));
    ImGui::TextUnformatted("Static Mesh Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name:  %s", asset->getName().c_str());
    ImGui::Text("Path:  %s", asset->getPath().c_str());

    auto meshes = asset->getMeshes();
    int meshCount = static_cast<int>(meshes.size());

    int64_t totalVerts = 0;
    int64_t totalIndices = 0;
    for (const auto* m : meshes)
    {
        if (!m) continue;
        totalVerts   += m->getVertexCount();
        totalIndices += m->getIndexCount();
    }
    int64_t totalTris = totalIndices / 3;

    ImGui::Spacing();

    char vertBuf[32], triBuf[32];
    formatCount(vertBuf, sizeof(vertBuf), totalVerts);
    formatCount(triBuf,  sizeof(triBuf),  totalTris);

    if (ImGui::BeginTable("##mesh_stats", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed, 100.f);
        ImGui::TableSetupColumn("value",  ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Submeshes");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", meshCount);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Vertices");
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(vertBuf);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Triangles");
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(triBuf);

        ImGui::EndTable();
    }
}

// -- text ---------------------------------------------------------------------

void MAssetInspector::drawTextAsset(MTextAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 0.6f, 1.f));
    ImGui::TextUnformatted("Text Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
}

// -- generic fallback ---------------------------------------------------------

void MAssetInspector::drawGenericAsset(MAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.f));
    ImGui::TextUnformatted("Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
    ImGui::Text("ID:   %s", asset->getAssetId().c_str());
}