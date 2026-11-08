//
// Created by ssj5v on 29-09-2024.
//

#ifndef MESH_H
#define MESH_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

class MStaticMesh;
namespace pugi { class xml_node; }
struct aiScene;
struct aiNode;
struct aiMesh;

// Where a mesh's origin (pivot) is placed on import.
enum class EMeshPivot : int
{
    FileOrigin   = 0,   // as authored - keeps any placement baked into the file
    Center       = 1,   // centre of the bounding box
    BottomCenter = 2,   // centre of the bounding box's base - good for props that sit on the ground
};

/// This Asset holds the reference for static meshes. An asset can
/// hold multiple static mesh instances.
SCRIPT_BIND_CLASS()
class MStaticMeshAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MStaticMeshAsset)
private:
    std::vector<MStaticMesh*> meshes;

    // Import settings, saved in the asset's .meta under <meshImport>.
    // Uniform scale baked into the vertex positions at load, so entities can
    // keep a transform scale of 1 for assets authored in other units.
    float scaleFactor = 1.0f;
    // Moves the origin. Files often carry the position the model had in the
    // artist's scene, which would otherwise offset it from its entity.
    EMeshPivot pivot = EMeshPivot::FileOrigin;

    // Bounds relative to the chosen pivot, before scaleFactor is applied.
    SVector3 sourceBoundsMin = SVector3(0.0f);
    SVector3 sourceBoundsMax = SVector3(0.0f);

public:
    explicit MStaticMeshAsset(const SString& path);
    MStaticMeshAsset(const SString& path, const pugi::xml_node& importSettings);
    ~MStaticMeshAsset() override;

    // -- Import settings ------------------------------------------------------
    float getScaleFactor() const { return scaleFactor; }
    // Takes effect on the next reload (requestReload / Save & Reimport).
    void  setScaleFactor(float s) { scaleFactor = s > 0.0f ? s : 1.0f; }
    EMeshPivot getPivot() const { return pivot; }
    void       setPivot(EMeshPivot p) { pivot = p; }   // takes effect on the next reload

    // Size of the mesh as it would load with the current scale factor -
    // updates live while the factor is edited, before it is reimported.
    SVector3 getScaledBoundsSize() const { return (sourceBoundsMax - sourceBoundsMin) * scaleFactor; }

    void loadImportSettings(const pugi::xml_node& node);
    bool saveImportSettings();

    /// Returns all static meshes within the asset.
    SCRIPT_BIND_FUNC()
    std::vector<MStaticMesh*> getMeshes() const;

    bool requestReload() override { loadMesh(); return valid; }

private:
    MStaticMesh* processMesh(aiMesh *mesh, const SVector3& origin, float scale);
    void processNode(aiNode *node, const aiScene *scene, const SVector3& origin, std::vector<MStaticMesh*>& meshes);
    // Imports through the active asset source (see MAssimpAssetIOSystem), so
    // companion files like .mtl / .bin are read through the source as well.
    // On failure the previously loaded meshes are kept.
    void loadMesh();
};

#endif //MESH_H
