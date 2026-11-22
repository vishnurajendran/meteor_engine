// asset_inspector_ui.cpp
//
// Inspectors for the UI asset types (.muia, .rml, .rcss) and fonts.

#include "asset_inspector.h"

#include <algorithm>
#include <cstring>
#include <map>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/font/font_asset.h"
#include "core/engine/scripting/interface/script_asset.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/assets/ui_structure_asset.h"
#include "core/engine/ui/assets/ui_style_asset.h"
#include "core/engine/ui/assets/ui_widget_asset.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/memory/mnew.h"
#include "core/utils/logger.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "font_preview_cache.h"
#include "imgui.h"

namespace
{
    constexpr float LABEL_WIDTH = 120.0f;

    IAssetManagerSubsystem* assetManager()
    {
        return MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    }

    void header(const char* title, const ImVec4& colour)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    void row(const char* label, const SString& value)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
        ImGui::TableSetColumnIndex(1);
        ImGui::TextWrapped("%s", value.c_str());
    }

    bool beginInfoTable(const char* id)
    {
        if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_None))
            return false;
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LABEL_WIDTH);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);
        return true;
    }

    int countLines(const SString& text)
    {
        if (text.empty())
            return 0;
        return 1 + static_cast<int>(std::count(text.begin(), text.end(), '\n'));
    }

    void openExternally(MAsset* asset)
    {
        if (ImGui::Button("Open in external editor", ImVec2(-FLT_MIN, 0)))
            if (auto* editorAM = dynamic_cast<MEditorAssetManager*>(assetManager()))
                editorAM->openAsset(asset);
    }

    // Reference controls are kept per asset and slot, like the cubemap faces.
    MAssetReferenceControl* referenceControl(const SString& key, std::function<bool(TAssetHandle<MAsset>)> accept)
    {
        static std::map<SString, MAssetReferenceControl*> controls;
        auto& control = controls[key];
        if (!control)
        {
            control = mnew<MAssetReferenceControl>();
            control->canAcceptAssetFuncCallback = std::move(accept);
        }
        return control;
    }

    // Draws one reference slot; returns true with `reference` changed when the
    // user dropped or cleared an asset. References are stored as GUIDs.
    bool referenceSlot(const SString& key, const char* label, SString& reference,
                       std::function<bool(TAssetHandle<MAsset>)> accept)
    {
        auto* manager = assetManager();
        MAssetReferenceControl* control = referenceControl(key, std::move(accept));

        const TAssetHandle<MAsset> current = manager && !reference.empty()
            ? manager->getAssetFromReference<MAsset>(reference) : TAssetHandle<MAsset>();
        if (control->getAssetReference() != current)
            control->setAssetReference(current);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(key.c_str());
        const bool changed = control->drawControl("");
        ImGui::PopID();
        if (changed)
        {
            const auto picked = control->getAssetReference();
            reference = picked ? picked.toRefString() : SString();
        }
        return changed;
    }
}

// =====================================================================================
// .muia
// =====================================================================================

void MAssetInspector::drawUIWidgetAsset(MUIWidgetAsset* asset)
{
    header("UI Widget", ImVec4(0.45f, 0.8f, 1.f, 1.f));

    if (beginInfoTable("##muia_info"))
    {
        row("Name:", asset->getWidgetName());
        row("Path:", asset->getPath());
        row("Root class:", SString("w-") + asset->getWidgetName());
        ImGui::EndTable();
    }
    ImGui::Spacing();

    const SString id = asset->getAssetId();
    bool changed = false;

    if (ImGui::CollapsingHeader("Parts", ImGuiTreeNodeFlags_DefaultOpen) && beginInfoTable("##muia_parts"))
    {
        SString structure = asset->getStructureRef();
        if (referenceSlot(id + "#structure", "Structure:", structure,
                          [](TAssetHandle<MAsset> a) { return dynamic_cast<MUIStructureAsset*>(a.get()) != nullptr; }))
        {
            asset->setStructureRef(structure);
            changed = true;
        }

        // Styles: applied in this order, later ones win.
        std::vector<SString> styles = asset->getStyleRefs();
        int removeIndex = -1;
        for (size_t i = 0; i < styles.size(); ++i)
        {
            const SString label = SString::format("Style {0}:", SString::fromInt(static_cast<int>(i) + 1));
            if (referenceSlot(id + "#style" + SString::fromInt(static_cast<int>(i)), label.c_str(), styles[i],
                              [](TAssetHandle<MAsset> a) { return dynamic_cast<MUIStyleAsset*>(a.get()) != nullptr; }))
            {
                asset->setStyleRefs(styles);
                changed = true;
            }
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton("Remove"))
                removeIndex = static_cast<int>(i);
            ImGui::PopID();
        }
        if (removeIndex >= 0)
        {
            styles.erase(styles.begin() + removeIndex);
            asset->setStyleRefs(styles);
            changed = true;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(1);
        if (ImGui::SmallButton("+ Add style"))
        {
            styles.push_back(SString());
            asset->setStyleRefs(styles);
        }

        SString behaviour = asset->getBehaviourRef();
        if (referenceSlot(id + "#behaviour", "Behaviour:", behaviour,
                          [](TAssetHandle<MAsset> a) { return dynamic_cast<IScriptAsset*>(a.get()) != nullptr; }))
        {
            asset->setBehaviourRef(behaviour);
            changed = true;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Tick when paused:");
        ImGui::TableSetColumnIndex(1);
        bool tick = asset->getTickWhenPaused();
        if (ImGui::Checkbox("##tickWhenPaused", &tick))
        {
            asset->setTickWhenPaused(tick);
            changed = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Default for every instance: whether onWidgetTick runs while the game is paused.\n"
                              "Scripts can change it per widget with setTickWhenPaused.");
        ImGui::EndTable();
    }
    (void)changed;

    ImGui::Spacing();
    ImGui::BeginDisabled(!asset->isDirty());
    if (ImGui::Button(asset->isDirty() ? "Save *##muia_save" : "Save##muia_save", ImVec2(-FLT_MIN, 0)))
    {
        // Saving rebuilds live widgets that use it (hot reload).
        if (asset->save())
            if (auto* manager = dynamic_cast<MAssetManager*>(assetManager()))
                manager->reloadAsset(asset);
    }
    ImGui::EndDisabled();
}

// =====================================================================================
// .rml
// =====================================================================================

void MAssetInspector::drawUIStructureAsset(MUIStructureAsset* asset)
{
    header("UI Structure (.rml)", ImVec4(0.45f, 0.8f, 1.f, 1.f));

    if (beginInfoTable("##rml_info"))
    {
        row("Name:", asset->getName());
        row("Path:", asset->getPath());
        row("Lines:", SString::fromInt(countLines(asset->getSource())));
        ImGui::EndTable();
    }
    ImGui::Spacing();

    // Nested widgets, with a nudge towards GUID references.
    const auto& references = asset->getWidgetReferences();
    if (ImGui::CollapsingHeader("Nested widgets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (references.empty())
            ImGui::TextDisabled("None - nest a widget with <widget src=\"guid:...\" id=\"name\"/>");

        int pathReferences = 0;
        auto* manager = assetManager();
        for (const auto& reference : references)
        {
            const auto target = manager ? manager->getAssetFromReference<MUIWidgetAsset>(reference.src)
                                        : TAssetHandle<MUIWidgetAsset>();
            if (!reference.isGuid)
                ++pathReferences;

            if (!target)
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Missing: %s", reference.src.c_str());
            else
                ImGui::Text("%s  %s", target->getWidgetName().c_str(),
                            reference.isGuid ? "" : "(path - convert to a GUID)");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", reference.src.c_str());
        }

        if (pathReferences > 0)
        {
            ImGui::Spacing();
            if (ImGui::Button("Convert paths to GUIDs", ImVec2(-FLT_MIN, 0)))
            {
                const int converted = asset->convertWidgetPathsToGuids();
                if (converted >= 0)
                    MLOG(SString::format("{0}: {1} widget reference(s) now use GUIDs", asset->getPath(),
                                         SString::fromInt(converted)));
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Rewrites src=\"assets/...\" as src=\"guid:...\" in the file,\n"
                                  "so moving or renaming the nested widget keeps the reference.");
        }
    }

    ImGui::Spacing();
    openExternally(asset);
}

// =====================================================================================
// .rcss
// =====================================================================================

void MAssetInspector::drawUIStyleAsset(MUIStyleAsset* asset)
{
    header("UI Style (.rcss)", ImVec4(0.45f, 0.8f, 1.f, 1.f));

    if (beginInfoTable("##rcss_info"))
    {
        row("Name:", asset->getName());
        row("Path:", asset->getPath());
        row("Lines:", SString::fromInt(countLines(asset->getSource())));
        ImGui::EndTable();
    }
    ImGui::Spacing();
    ImGui::TextWrapped("Edits apply to live widgets in place while playing.");
    ImGui::Spacing();
    openExternally(asset);
}

// =====================================================================================
// Fonts
// =====================================================================================

void MAssetInspector::drawFontAsset(MFontAsset* asset)
{
    header("Font Asset", ImVec4(0.9f, 0.75f, 0.45f, 1.f));

    const bool isDefault = MViewportSubsystem::isDefaultFontPath(asset->getPath());
    if (beginInfoTable("##font_info"))
    {
        row("Name:", asset->getName());
        row("Path:", asset->getPath());
        row("Family:", asset->getFileFamily());
        row("UI family:", isDefault ? SString(MViewportSubsystem::DEFAULT_FONT_FAMILY) : asset->getFamily());
        row("Style:", asset->getStyleName());
        row("Weight:", SString::fromInt(asset->getWeight()));
        row("Italic:", asset->isItalic() ? "Yes" : "No");
        ImGui::EndTable();
    }
    if (asset->usedFallbackMetadata())
        ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "The font's tables could not be read; named after the file.");
    if (isDefault)
        ImGui::TextDisabled("Engine default font: documents use it as \"%s\".", MViewportSubsystem::DEFAULT_FONT_FAMILY);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen) && beginInfoTable("##font_import"))
    {
        // One edit buffer, re-seeded when another font is inspected.
        static char    familyBuffer[256] = {};
        static SString bufferFor;
        if (bufferFor != asset->getAssetId())
        {
            bufferFor = asset->getAssetId();
            std::strncpy(familyBuffer, asset->getFamilyOverride().c_str(), sizeof(familyBuffer) - 1);
            familyBuffer[sizeof(familyBuffer) - 1] = '\0';
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Family override:");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputTextWithHint("##familyOverride", asset->getFileFamily().c_str(), familyBuffer, sizeof(familyBuffer)))
            asset->setFamilyOverride(SString(familyBuffer).trimmed());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("The family .rcss files use for this font (font-family: \"...\").\n"
                              "Empty: the family the file declares.");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Fallback:");
        ImGui::TableSetColumnIndex(1);
        bool fallback = asset->isFallback();
        if (ImGui::Checkbox("##fallback", &fallback))
            asset->setFallback(fallback);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use this font for characters the requested font doesn't have.");
        ImGui::EndTable();

        if (ImGui::Button("Save & Reimport##font_save", ImVec2(-FLT_MIN, 0)) && asset->saveImportSettings())
            if (auto* manager = dynamic_cast<MAssetManager*>(assetManager()))
                manager->reloadAsset(asset);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("A changed family applies to running UI from the next Play.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Preview: added to ImGui's atlas the first time, before the next frame.
    if (const auto* fonts = MFontPreviewCache::get(asset))
    {
        for (ImFont* font : *fonts)
        {
            ImGui::PushFont(font);
            ImGui::TextWrapped("%s", MFontPreviewCache::PREVIEW_TEXT);
            ImGui::PopFont();
            ImGui::Spacing();
        }
    }
    else
    {
        ImGui::TextDisabled("Loading preview...");
    }
}
