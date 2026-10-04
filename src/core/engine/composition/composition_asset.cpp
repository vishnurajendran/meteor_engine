//
// composition_asset.cpp
//

#include "composition_asset.h"

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>

#include "composition_utility.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/logger.h"

namespace
{
// Removes composition link data from `entity` and every descendant entity —
// used both for hashing and for writing files (flattens nested instances).
void stripLinkData(pugi::xml_node entity)
{
    entity.remove_child(MCompositionAsset::LINK_FIELD_NAME);
    entity.remove_attribute(MCompositionAsset::SOURCE_HASH_ATTRIBUTE);
    entity.remove_attribute(MCompositionAsset::STATE_HASH_ATTRIBUTE);

    if (pugi::xml_node children = entity.child("children"))
        for (pugi::xml_node child : children.children("entity"))
            stripLinkData(child);
}

// Collects pugixml output into a std::string instead of a stream.
struct SStringWriter final : pugi::xml_writer
{
    std::string out;
    void write(const void* data, size_t size) override
    {
        out.append(static_cast<const char*>(data), size);
    }
};

// FNV-1a 64-bit — tiny, deterministic across compilers and runs (std::hash
// isn't guaranteed to be), which matters because hashes are saved to disk.
std::uint64_t fnv1a64(const std::string& s)
{
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s)
    {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}
} // namespace

MCompositionAsset::MCompositionAsset(const SString& path) : MAsset(path)
{
    name  = "CompositionAsset";
    valid = loadFromSource();
}

bool MCompositionAsset::loadFromSource()
{
    document.reset();
    contentHash = SString();

    std::vector<uint8_t> bytes;
    if (!MAssetSources::getActive()->readBytes(path, bytes))
    {
        MERROR(SString("MCompositionAsset:: cannot read ") + path);
        return false;
    }

    const auto res = document.load_buffer(bytes.data(), bytes.size());
    if (res.status != pugi::status_ok)
    {
        MERROR(SString("MCompositionAsset:: failed to parse ") + path + " - " + res.description());
        return false;
    }

    const pugi::xml_node rootEntity = getRootEntityNode();
    if (!rootEntity)
    {
        MERROR(SString("MCompositionAsset:: missing <composition><entity> in ") + path);
        return false;
    }

    contentHash = computeContentHash(rootEntity);
    return true;
}

bool MCompositionAsset::requestReload()
{
    const SString previousHash = contentHash;
    valid = loadFromSource();

    // Only touch the scene when the content actually changed — our own Save
    // already refreshed instances, so the watcher's follow-up reload of the
    // same file is a no-op here.
    if (valid && previousHash != contentHash)
        MCompositionUtility::refreshInstances(this);

    return valid;
}

pugi::xml_node MCompositionAsset::getRootEntityNode() const
{
    return document.child(ROOT_TAG).child("entity");
}

bool MCompositionAsset::writeContent(const pugi::xml_node& entityNode)
{
    pugi::xml_document doc;
    buildFileDocument(entityNode, doc);
    if (!saveDocument(path, doc))
        return false;

    // Reload without refreshing instances — the caller (MCompositionUtility::
    // saveInstance) updates the saving instance first, then refreshes the rest.
    valid = loadFromSource();
    return valid;
}

void MCompositionAsset::buildFileDocument(const pugi::xml_node& entityNode, pugi::xml_document& out)
{
    out.reset();
    pugi::xml_node comp = out.append_child(ROOT_TAG);
    comp.append_attribute("formatVersion") = 1;

    pugi::xml_node root = comp.append_copy(entityNode);
    stripLinkData(root);

    // Each instance owns its root position, so the asset stores the root at
    // the origin. Rotation and scale are kept as the defaults for new instances.
    if (pugi::xml_node pos = root.child("relativePosition"))
    {
        pos.child("x").text().set(0.0f);
        pos.child("y").text().set(0.0f);
        pos.child("z").text().set(0.0f);
    }
}

bool MCompositionAsset::writeFile(const SString& path, const pugi::xml_node& entityNode)
{
    pugi::xml_document doc;
    buildFileDocument(entityNode, doc);
    return saveDocument(path, doc);
}

bool MCompositionAsset::saveDocument(const SString& path, const pugi::xml_document& doc)
{
    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR(SString("MCompositionAsset:: active asset source is read-only: ") + path);
        return false;
    }

    std::ostringstream oss;
    doc.save(oss);
    if (!target->writeText(MAssetPath::normalize(path), SString(oss.str())))
    {
        MERROR(SString("MCompositionAsset:: failed to write ") + path);
        return false;
    }
    return true;
}

SString MCompositionAsset::computeContentHash(const pugi::xml_node& entityNode)
{
    if (!entityNode)
        return SString();

    // Work on a copy — never mutate the caller's document.
    pugi::xml_document tmp;
    pugi::xml_node root = tmp.append_copy(entityNode);
    stripLinkData(root);

    // Per-instance data on the root — not part of the composition's content.
    root.remove_attribute("name");
    root.remove_attribute("enabled");
    root.remove_child("relativePosition");
    root.remove_child("relativeRotation");
    root.remove_child("relativeScale");

    SStringWriter writer;
    tmp.save(writer, "", pugi::format_raw | pugi::format_no_declaration);

    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx",
                  static_cast<unsigned long long>(fnv1a64(writer.out)));
    return SString(buf);
}
