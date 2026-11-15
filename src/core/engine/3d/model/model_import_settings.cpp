#include "model_import_settings.h"

#include <algorithm>

#include "pugixml.hpp"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/utils/logger.h"

namespace
{
    constexpr const char* IMPORT_AS_NAMES[] = { "Auto", "StaticMesh", "SkeletalMesh", "AnimationOnly" };
    constexpr const char* PIVOT_NAMES[]     = { "FileOrigin", "Center", "BottomCenter" };
}

const char* SModelImportSettings::toString(EModelImportAs value)
{
    const int i = std::clamp(static_cast<int>(value), 0, 3);
    return IMPORT_AS_NAMES[i];
}

const char* SModelImportSettings::toString(EMeshPivot value)
{
    const int i = std::clamp(static_cast<int>(value), 0, 2);
    return PIVOT_NAMES[i];
}

EModelImportAs SModelImportSettings::importAsFromString(const SString& text, EModelImportAs fallback)
{
    for (int i = 0; i < 4; ++i)
        if (text == IMPORT_AS_NAMES[i])
            return static_cast<EModelImportAs>(i);
    int asInt = 0;
    if (text.tryParseInt(asInt) && asInt >= 0 && asInt <= 3)
        return static_cast<EModelImportAs>(asInt);
    return fallback;
}

EMeshPivot SModelImportSettings::pivotFromString(const SString& text, EMeshPivot fallback)
{
    for (int i = 0; i < 3; ++i)
        if (text == PIVOT_NAMES[i])
            return static_cast<EMeshPivot>(i);
    // Older metas wrote the pivot as 0 / 1 / 2.
    int asInt = 0;
    if (text.tryParseInt(asInt) && asInt >= 0 && asInt <= 2)
        return static_cast<EMeshPivot>(asInt);
    return fallback;
}

void SModelImportSettings::load(const pugi::xml_node& node)
{
    if (!node) return;
    if (auto n = node.child("importAs"))
        importAs = importAsFromString(SString(n.text().as_string("")), importAs);
    if (auto n = node.child("scaleFactor"))
        setScaleFactor(n.text().as_float(1.0f));
    if (auto n = node.child("pivot"))
        pivot = pivotFromString(SString(n.text().as_string("")), pivot);
    if (auto n = node.child("maxBoneInfluences"))
        setMaxBoneInfluences(n.text().as_int(DEFAULT_MAX_BONE_INFLUENCES));
    if (auto n = node.child("sampleRate"))
        setSampleRate(n.text().as_float(DEFAULT_SAMPLE_RATE));
}

void SModelImportSettings::write(pugi::xml_node& node) const
{
    while (node.first_child())
        node.remove_child(node.first_child());

    node.append_child("importAs").text().set(toString(importAs));
    node.append_child("scaleFactor").text().set(scaleFactor);
    node.append_child("pivot").text().set(toString(pivot));
    node.append_child("maxBoneInfluences").text().set(maxBoneInfluences);
    node.append_child("sampleRate").text().set(sampleRate);
}

bool SModelImportSettings::saveToMeta(const SString& assetPath) const
{
    auto target = MAssetSources::getWritable();
    if (!target)
        return false;

    // Read the existing meta so the GUID and anything else in it is kept.
    pugi::xml_document doc;
    target->readMeta(assetPath, doc);

    auto root = doc.child("asset_id");
    if (!root)
    {
        MERROR("SModelImportSettings::saveToMeta - meta file missing <asset_id> root: " + assetPath);
        return false;
    }

    auto settings = root.child("meshImport");
    if (!settings)
        settings = root.append_child("meshImport");
    write(settings);

    return target->writeMeta(assetPath, doc);
}
