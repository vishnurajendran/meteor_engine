#include "add_entity.h"
#include <GL/glew.h>
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/audio/audio_entity/audio_source_entity.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/physics/entities/box_collision_body_entity.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "editor/window/menubar/menubartree.h"

// Mesh asset paths
static constexpr const char* MAT_DEFAULT     = "meteor_assets/engine_assets/materials/lit_default.material";
static constexpr const char* MESH_CUBE     = "meteor_assets/engine_assets/mesh/primitive/box.glb";
static constexpr const char* MESH_SPHERE   = "meteor_assets/engine_assets/mesh/primitive/sphere.glb";
static constexpr const char* MESH_CYLINDER = "meteor_assets/engine_assets/mesh/primitive/cylinder.glb";
static constexpr const char* MESH_PLANE    = "meteor_assets/engine_assets/mesh/primitive/plane.glb";
static constexpr const char* MESH_CONE     = "meteor_assets/engine_assets/mesh/primitive/cone.glb";
static constexpr const char* MESH_CAPSULE  = "meteor_assets/engine_assets/mesh/primitive/capsule.glb";

static void spawnPrimitive(const char* meshAssetPath, const char* name)
{
    auto* entity = MSpatialEntity::createInstance<MStaticMeshEntity>(name);
    const auto mesh = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MStaticMeshAsset>(meshAssetPath);
    const auto material = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MMaterialAsset>(MAT_DEFAULT);
    if (!mesh && !material)
    {
        entity->setMaterialAsset(material);
        entity->setStaticMeshAsset(mesh);
    }
    else
    {
        MERROR("Failed to load mesh asset");
        if (!mesh) MERROR("Missing Mesh Asset");
        if (!material) MERROR("Missing Material Asset");
        entity->destroy();
    }
}

bool MAddCubeItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MAddCubeItem());
    return true;
}();
void MAddCubeItem::onSelect() { spawnPrimitive(MESH_CUBE, "Cube"); }

bool MAddSphereItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddSphereItem());
    return true;
}();
void MAddSphereItem::onSelect() { spawnPrimitive(MESH_SPHERE, "Sphere"); }

bool MAddCylinderItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddCylinderItem());
    return true;
}();
void MAddCylinderItem::onSelect() { spawnPrimitive(MESH_CYLINDER, "Cylinder"); }

bool MAddPlaneItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddPlaneItem());
    return true;
}();

void MAddPlaneItem::onSelect() { spawnPrimitive(MESH_PLANE, "Plane"); }

bool MAddConeItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddConeItem());
    return true;
}();

void MAddConeItem::onSelect() { spawnPrimitive(MESH_CONE, "Cone"); }

bool MAddCapsuleItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddCapsuleItem());
    return true;
}();
void MAddCapsuleItem::onSelect() { spawnPrimitive(MESH_CAPSULE, "Capsule"); }

// Static Mesh
bool MAddStaticMeshItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddStaticMeshItem());
    return true;
}();
void MAddStaticMeshItem::onSelect() {  MSpatialEntity::createInstance<MStaticMeshEntity>("StaticMesh"); }

void MAddEmptySpatialItem::onSelect() { MSpatialEntity::createInstance("Spatial Entity"); }
bool MAddEmptySpatialItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MAddEmptySpatialItem());
    return true;
}();
