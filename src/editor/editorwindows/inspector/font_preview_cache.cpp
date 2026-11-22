//
// font_preview_cache.cpp
//

#include "font_preview_cache.h"

#include <algorithm>

#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/font/font_asset.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"
#include "imgui-SFML.h"
#include "imgui.h"

std::unordered_map<std::string, MFontPreviewCache::SEntry>& MFontPreviewCache::entries()
{
    static std::unordered_map<std::string, SEntry> map;
    return map;
}

std::vector<std::string>& MFontPreviewCache::pending()
{
    static std::vector<std::string> list;
    return list;
}

const MFontPreviewCache::FontSet* MFontPreviewCache::get(const MFontAsset* font)
{
    if (!font || !font->isValid() || font->getAssetId().empty())
        return nullptr;

    const std::string key = font->getAssetId().str();
    const auto it = entries().find(key);
    if (it != entries().end() && it->second.generation == font->getGeneration())
        return it->second.ready ? &it->second.fonts : nullptr;   // not ready: ImGui couldn't load it

    // Missing or out of date: (re)add before the next frame.
    auto& queue = pending();
    if (std::find(queue.begin(), queue.end(), key) == queue.end())
        queue.push_back(key);
    return nullptr;
}

void MFontPreviewCache::processPending()
{
    auto& queue = pending();
    if (queue.empty())
        return;

    auto* manager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!manager)
    {
        queue.clear();
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    bool added = false;
    for (const std::string& key : queue)
    {
        const MFontAsset* font = manager->getAssetById<MFontAsset>(SString(key)).get();
        if (!font || !font->isValid())
            continue;

        // ImGui can't remove one font from the atlas: a reloaded font is added
        // again and the old entry is simply no longer used.
        SEntry entry;
        entry.generation = font->getGeneration();
        entry.data       = font->getSharedData();

        ImFontConfig config;
        config.FontDataOwnedByAtlas = false;   // `data` stays alive in the entry
        bool complete = true;
        for (size_t i = 0; i < PREVIEW_SIZES.size(); ++i)
        {
            entry.fonts[i] = io.Fonts->AddFontFromMemoryTTF(
                const_cast<uint8_t*>(entry.data->data()), static_cast<int>(entry.data->size()),
                PREVIEW_SIZES[i], &config, io.Fonts->GetGlyphRangesDefault());
            complete = complete && entry.fonts[i] != nullptr;
        }
        if (!complete)
        {
            MWARN("Font preview: ImGui could not load " + font->getPath());
            entry.ready = false;
            entries()[key] = std::move(entry);   // don't retry this generation
            continue;
        }
        entry.ready = true;
        entries()[key] = std::move(entry);
        added = true;
    }
    queue.clear();

    if (added && !ImGui::SFML::UpdateFontTexture())
        MWARN("Font preview: could not rebuild the font atlas");
}
