#include "add_entity.h"

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/camera.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"
#include "core/engine/skybox/procedural_sky/procedural_sky.h"
#include "editor/window/menubar/menubartree.h"

// Mesh asset paths
static constexpr const char* MESH_CUBE     = "meteor_assets/engine_assets/meshes/cube.obj";
static constexpr const char* MESH_SPHERE   = "meteor_assets/engine_assets/meshes/sphere.obj";
static constexpr const char* MESH_CYLINDER = "meteor_assets/engine_assets/meshes/cylinder.obj";
static constexpr const char* MESH_PLANE    = "meteor_assets/engine_assets/meshes/plane.obj";
static constexpr const char* MESH_CONE     = "meteor_assets/engine_assets/meshes/cone.obj";
static constexpr const char* MESH_CAPSULE  = "meteor_assets/engine_assets/meshes/capsule.obj";

// ── Helpers ───────────────────────────────────────────────────────────────────

static void spawnPrimitive(const char* meshAssetPath, const char* name)
{
    auto* entity = MSpatialEntity::createInstance<MStaticMeshEntity>(name);
    if (auto* mesh = MAssetManager::getInstance()->getAsset<MStaticMeshAsset>(meshAssetPath))
        entity->setStaticMeshAsset(mesh);
}

// ── Primitives ────────────────────────────────────────────────────────────────

bool MAddCubeItem::registered = []() {
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

// ── Lights ────────────────────────────────────────────────────────────────────

bool MAddPointLightItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddPointLightItem());
    return true;
}();
void MAddPointLightItem::onSelect()
{
    MSpatialEntity::createInstance<MPointLight>("Point Light");
}

bool MAddSpotLightItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddSpotLightItem());
    return true;
}();
void MAddSpotLightItem::onSelect()
{
    MSpatialEntity::createInstance<MSpotLight>("Spot Light");
}

bool MAddDirectionalLightItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddDirectionalLightItem());
    return true;
}();
void MAddDirectionalLightItem::onSelect()
{
    MSpatialEntity::createInstance<MDirectionalLight>("Directional Light");
}

bool MAddAmbientLightItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddAmbientLightItem());
    return true;
}();
void MAddAmbientLightItem::onSelect()
{
    MSpatialEntity::createInstance<MAmbientLightEntity>("Ambient Light");
}

// ── Sky ───────────────────────────────────────────────────────────────────────

bool MAddProceduralSkyboxItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddProceduralSkyboxItem());
    return true;
}();

void MAddProceduralSkyboxItem::onSelect()
{
    MSpatialEntity::createInstance<MProceduralSkyboxEntity>("Procedural Skybox");
}

// ── Misc ──────────────────────────────────────────────────────────────────────

bool MAddCameraItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddCameraItem());
    return true;
}();

void MAddCameraItem::onSelect()
{
    auto* e = MSpatialEntity::createInstance("Camera");
}

bool MAddEmptySpatialItem::registered = []() {
    MMenubarTreeNode::registerItem(new MAddEmptySpatialItem());
    return true;
}();
void MAddEmptySpatialItem::onSelect()
{
    MSpatialEntity::createInstance("Spatial");
}