//
// skeletal_model_loader.h
//
// Reads skeletal meshes and animation clips from a model file through Assimp
// (and the active asset source). Used by MSkeletalMeshAsset, MAnimClipAsset
// and MModelAssetImporter's importAs detection.
//
// Rules (see the animation system design):
//   - Node transforms are not baked into skinned vertices with
//     PreTransformVertices; vertices stay in bind space.
//   - scaleFactor applies to vertex positions, bone translations,
//     inverse-bind matrices and clip translation keys alike.
//   - pivot becomes SSkeleton::rootOffset rather than a vertex shift.
//   - FBX pivots are not preserved, so there are no $AssimpFbx$ helper bones.
//   - Bone names are kept exactly as authored.
//   - Each vertex's influences are sorted by weight, cut to
//     maxBoneInfluences (0 = keep all) and renormalized. A vertex with no
//     influences gets one, at full weight, on the node that holds its mesh.
//   - Clip tracks are resampled to evenly spaced keys at sampleRate.
//
#pragma once
#ifndef SKELETAL_MODEL_LOADER_H
#define SKELETAL_MODEL_LOADER_H

#include <vector>

#include "core/engine/3d/model/model_import_settings.h"
#include "skeletal_mesh.h"
#include "skeleton.h"

class MAnimClip;
struct aiNode;
struct aiScene;
namespace Assimp { class Importer; }

struct SSkeletalModelData
{
    struct SSubMesh
    {
        std::vector<SVertex>        vertices;
        std::vector<uint32_t>       offsets;      // vertexCount + 1
        std::vector<SBoneInfluence> influences;
        std::vector<unsigned int>   indices;
    };

    SSkeleton             skeleton;
    std::vector<SSubMesh> subMeshes;
    std::vector<MAnimClip*> clips;      // the caller takes ownership

    // Bind pose, scaled, with the pivot applied - for bounds and the inspector.
    SVector3 boundsMin = SVector3(0.0f);
    SVector3 boundsMax = SVector3(0.0f);
    // How far the skin reaches past the deform joints' box, per axis. Runtime
    // bounds are the joints' box grown by this (see MSkeletalMeshEntity).
    SVector3 jointBoundsPadding = SVector3(0.0f);
};

// Stateless: every member is static.
class MSkeletalModelLoader
{
public:
    enum class EContent
    {
        MeshesAndClips,   // skeletal mesh asset
        ClipsOnly,        // animation-only asset
    };

    MSkeletalModelLoader() = delete;

    // False (with a reason in outError) when the file can't be read or has
    // nothing of the requested content.
    static bool load(const SString& path, const SModelImportSettings& settings, EContent content,
                     SSkeletalModelData& out, SString& outError);

    // What importAs = Auto resolves to for this file:
    //   meshes with bones          -> SkeletalMesh
    //   no meshes, has animations  -> AnimationOnly
    //   anything else              -> StaticMesh (also when unreadable)
    static EModelImportAs detect(const SString& path);

private:
    // Importer through the active asset source, FBX pivots not preserved.
    static void configureImporter(Assimp::Importer& importer);

    // Every node, parent-first (depth-first pre-order), becomes a bone.
    static void collectNodes(const aiNode* node, int parent, std::vector<const aiNode*>& nodes, SSkeleton& skeleton);

    static bool buildSkeletonAndMeshes(const aiScene* scene, const SString& path, const SModelImportSettings& settings,
                                       SSkeletalModelData& out, SString& outError);

    // Resamples every animation to evenly spaced keys at settings.sampleRate.
    static void buildClips(const aiScene* scene, const SString& path, const SModelImportSettings& settings,
                           std::vector<MAnimClip*>& outClips);

    // "mixamo.com", "Take 001"... - a lone take with one of these is named after the file.
    static bool isGenericTakeName(const SString& name);
};

#endif // SKELETAL_MODEL_LOADER_H
