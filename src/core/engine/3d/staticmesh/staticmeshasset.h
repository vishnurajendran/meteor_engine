//
// Created by ssj5v on 29-09-2024.
//

#ifndef MESH_H
#define MESH_H

#include <vector>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"
#include "core/engine/3d/model/model_import_settings.h"

class MStaticMesh;
namespace pugi { class xml_node; }
struct aiScene;
struct aiNode;
struct aiMesh;

/// This Asset holds the reference for static meshes. An asset can
/// hold multiple static mesh instances.
SCRIPT_BIND_CLASS()
class MStaticMeshAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MStaticMeshAsset)
private:
    std::vector<MStaticMesh*> meshes;

    // Import settings, saved in the asset's .meta under <meshImport> (see
    // model_import_settings.h). scaleFactor is a uniform scale baked into the
    // vertex positions at load, so entities can keep a transform scale of 1
    // for assets authored in other units; pivot moves the origin, since files
    // often carry the position the model had in the artist's scene.
    SModelImportSettings importSettings;

    // Bounds relative to the chosen pivot, before scaleFactor is applied.
    SVector3 sourceBoundsMin = SVector3(0.0f);
    SVector3 sourceBoundsMax = SVector3(0.0f);

public:
    explicit MStaticMeshAsset(const SString& path);
    MStaticMeshAsset(const SString& path, const pugi::xml_node& importSettings);
    ~MStaticMeshAsset() override;

    // -- Import settings ------------------------------------------------------
    float getScaleFactor() const { return importSettings.scaleFactor; }
    // Takes effect on the next reload (requestReload / Save & Reimport).
    void  setScaleFactor(float s) { importSettings.setScaleFactor(s); }
    EMeshPivot getPivot() const { return importSettings.pivot; }
    void       setPivot(EMeshPivot p) { importSettings.pivot = p; }   // takes effect on the next reload

    // Every import setting, including importAs. Changing importAs changes the
    // asset class: save, then reimport through the asset manager
    // (MAssetManager::reimportAsset), which recreates the asset.
    const SModelImportSettings& getImportSettings() const { return importSettings; }
    SModelImportSettings&       getImportSettings()       { return importSettings; }

    // Size of the mesh as it would load with the current scale factor -
    // updates live while the factor is edited, before it is reimported.
    SVector3 getScaledBoundsSize() const { return (sourceBoundsMax - sourceBoundsMin) * importSettings.scaleFactor; }

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
