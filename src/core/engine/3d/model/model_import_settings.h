//
// model_import_settings.h
//
// Import settings shared by every model file (gltf, glb, fbx, obj), saved in
// the asset's .meta under <meshImport>:
//
//   <asset_id id="...">
//       <meshImport>
//           <importAs>Auto</importAs>               Auto | StaticMesh | SkeletalMesh | AnimationOnly
//           <scaleFactor>0.01</scaleFactor>
//           <pivot>FileOrigin</pivot>               FileOrigin | Center | BottomCenter (or 0 / 1 / 2)
//           <maxBoneInfluences>8</maxBoneInfluences> 0 = no limit
//           <sampleRate>30</sampleRate>
//       </meshImport>
//   </asset_id>
//
// importAs picks the asset class (see MModelAssetImporter). Auto is resolved
// on the first import and written back, so the meta never keeps Auto once a
// writable source has seen the file.
//
#pragma once
#ifndef MODEL_IMPORT_SETTINGS_H
#define MODEL_IMPORT_SETTINGS_H

#include "core/utils/sstring.h"

namespace pugi { class xml_node; }

// Where a mesh's origin (pivot) is placed on import.
enum class EMeshPivot : int
{
    FileOrigin   = 0,   // as authored - keeps any placement baked into the file
    Center       = 1,   // centre of the bounding box
    BottomCenter = 2,   // centre of the bounding box's base - good for props that sit on the ground
};

// Which asset class a model file becomes.
enum class EModelImportAs : int
{
    Auto          = 0,   // detected on first import, then written back
    StaticMesh    = 1,   // MStaticMeshAsset - rigged files import in bind pose, bones ignored
    SkeletalMesh  = 2,   // MSkeletalMeshAsset - meshes, skeleton and embedded clips
    AnimationOnly = 3,   // MAnimClipAsset - every clip (take) in the file
};

struct SModelImportSettings
{
    static constexpr int   DEFAULT_MAX_BONE_INFLUENCES = 8;
    static constexpr float DEFAULT_SAMPLE_RATE         = 30.0f;

    EModelImportAs importAs          = EModelImportAs::Auto;
    float          scaleFactor       = 1.0f;
    EMeshPivot     pivot             = EMeshPivot::FileOrigin;
    int            maxBoneInfluences = DEFAULT_MAX_BONE_INFLUENCES;   // 0 = keep all
    float          sampleRate        = DEFAULT_SAMPLE_RATE;           // keys per second

    // Missing values keep their defaults. Accepts the pivot as a name or as
    // the older integer form.
    void load(const pugi::xml_node& meshImportNode);
    // Writes every value into an existing <meshImport> node (children replaced).
    void write(pugi::xml_node& meshImportNode) const;

    // Rewrites <meshImport> in the asset's meta through the writable asset
    // source, keeping the GUID and anything else in the file. False when the
    // source is read-only (published games) or the meta is missing.
    bool saveToMeta(const SString& assetPath) const;

    void setScaleFactor(float s)        { scaleFactor = s > 0.0f ? s : 1.0f; }
    void setSampleRate(float r)         { sampleRate = r > 0.0f ? r : DEFAULT_SAMPLE_RATE; }
    void setMaxBoneInfluences(int n)    { maxBoneInfluences = n < 0 ? 0 : n; }

    static const char*    toString(EModelImportAs value);
    static const char*    toString(EMeshPivot value);
    static EModelImportAs importAsFromString(const SString& text, EModelImportAs fallback = EModelImportAs::Auto);
    static EMeshPivot     pivotFromString(const SString& text, EMeshPivot fallback = EMeshPivot::FileOrigin);
};

#endif // MODEL_IMPORT_SETTINGS_H
