//
// font_asset.cpp
//

#include "font_asset.h"

#include <atomic>

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "font_metadata.h"
#include "pugixml.hpp"

namespace
{
    std::atomic<uint64_t> g_fontGeneration{0};
}

MFontAsset::MFontAsset(const SString& path) : MAsset(path)
{
    name  = FileIO::getFileName(path);
    valid = load();
}

MFontAsset::MFontAsset(const SString& path, const pugi::xml_node& importSettings) : MAsset(path)
{
    name = FileIO::getFileName(path);
    loadImportSettings(importSettings);
    valid = load();
}

SString MFontAsset::getFamily() const
{
    return familyOverride.empty() ? fileFamily : familyOverride;
}

bool MFontAsset::requestReload()
{
    valid = load();
    return valid;
}

bool MFontAsset::load()
{
    // Read through the asset source, so fonts work from a package too.
    auto bytes = std::make_shared<std::vector<uint8_t>>();
    if (!MAssetSources::getActive()->readBytes(path, *bytes) || bytes->empty())
    {
        MERROR("MFontAsset: failed to read " + path);
        return false;
    }

    SFontMetadata meta;
    metadataFallback = !MFontMetadataReader::read(bytes->data(), bytes->size(), meta);
    if (metadataFallback)
    {
        // Still importable: named after the file, regular weight.
        meta.family    = FileIO::getFileNameWithoutExtension(path);
        meta.styleName = "Regular";
        meta.weight    = 400;
        meta.italic    = false;
        MWARN("MFontAsset: could not read the font tables of " + path + " - using the file name as its family");
    }

    fileFamily = meta.family;
    styleName  = meta.styleName;
    weight     = meta.weight;
    italic     = meta.italic;

    data       = std::move(bytes);
    generation = ++g_fontGeneration;
    return true;
}

// -- Import settings ---------------------------------------------------------------

void MFontAsset::loadImportSettings(const pugi::xml_node& node)
{
    if (!node)
        return;
    if (auto n = node.child("familyOverride")) familyOverride = SString(n.text().as_string());
    if (auto n = node.child("fallback"))       fallback       = n.text().as_bool(false);
}

bool MFontAsset::saveImportSettings()
{
    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MFontAsset::saveImportSettings - active asset source is read-only: " + path);
        return false;
    }

    // Keep the GUID and anything else already in the meta.
    pugi::xml_document doc;
    target->readMeta(path, doc);
    auto root = doc.child("asset_id");
    if (!root)
    {
        MERROR("MFontAsset::saveImportSettings - meta file missing <asset_id> root: " + path);
        return false;
    }

    root.remove_child("fontImport");
    auto settings = root.append_child("fontImport");
    settings.append_child("familyOverride").text().set(familyOverride.c_str());
    settings.append_child("fallback").text().set(fallback);
    return target->writeMeta(path, doc);
}
