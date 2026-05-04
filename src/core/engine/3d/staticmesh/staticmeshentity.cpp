#include "staticmeshentity.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "staticmesh.h"
#include "staticmeshasset.h"

IMPLEMENT_CLASS(MStaticMeshEntity)

MStaticMeshEntity::MStaticMeshEntity()
{
    name                = "StaticMeshEntity";
    prevTransformMatrix = SMatrix4(0.f);
    materialSlots.resize(1);
    MRenderQueue::addToSubmitables(this);
}

MStaticMeshEntity::~MStaticMeshEntity()
{
    MRenderQueue::removeFromSubmitables(this);
    delete errorMaterialInstance;
}

void MStaticMeshEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    const std::string& meshPath = meshAssetPath.get();
    if (!meshPath.empty())
    {
        auto* asset = MAssetManager::getInstance()->getAsset<MStaticMeshAsset>(meshPath.c_str());
        if (asset) setStaticMeshAsset(asset);
        else       MWARN(STR("MStaticMeshEntity: mesh not found: ") + meshPath);
    }

    const std::string& matPath = materialAssetPath.get();
    if (!matPath.empty())
    {
        auto* asset = MAssetManager::getInstance()->getAsset<MMaterialAsset>(matPath.c_str());
        if (asset) setMaterialAsset(asset, 0);
        else       MWARN(STR("MStaticMeshEntity: material not found: ") + matPath);
    }
}

void MStaticMeshEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (staticMeshAsset.isNull()) return;

    const SMatrix4 current = getTransformMatrix();
    if (current != prevTransformMatrix) { prevTransformMatrix = current; calculateBounds(); }

    // Lazy-load error material once.
    if (!errorMaterialInstance)
    {
        auto* asset = MAssetManager::getInstance()
            ->getAsset<MMaterialAsset>("meteor_assets/engine_assets/materials/error_material.material");
        if (asset) errorMaterialInstance = asset->getMaterial();
    }

    const auto meshes    = staticMeshAsset->getMeshes();
    const int  drawCount = std::min((int)meshes.size(), (int)materialSlots.size());

    for (int i = 0; i < drawCount; ++i)
    {
        const auto* mesh = meshes[i];
        if (!mesh) continue;

        // Use the asset's shared material directly — no instancing.
        MMaterial* mat = materialSlots[i].getMaterial();
        if (!mat) mat = errorMaterialInstance;
        if (!mat) continue;

        SRenderItem item;
        item.vao         = mesh->getVAO();
        item.ebo         = mesh->getEBO();
        item.indexCount  = mesh->getIndexCount();
        item.vertexCount = mesh->getVertexCount();
        item.transform   = current;
        item.material    = mat;
        item.bounds      = bounds;
        item.castsShadow = castsShadow.get();
        item.sortOrder   = static_cast<int>(ERenderStageOrder::RS_Opaque);
        collector->submitRenderItem(item);
    }
}

void MStaticMeshEntity::onExit()
{
    MSpatialEntity::onExit();
    MRenderQueue::removeFromSubmitables(this);
}

void MStaticMeshEntity::onUpdate(float dt)
{
    MSpatialEntity::onUpdate(dt);
    const SMatrix4 m = getTransformMatrix();
    if (m != prevTransformMatrix) { prevTransformMatrix = m; calculateBounds(); }
}

void MStaticMeshEntity::onDrawGizmo(SVector2 res) { MSpatialEntity::onDrawGizmo(res); }

void MStaticMeshEntity::setStaticMeshAsset(TAssetHandle<MStaticMeshAsset> asset)
{
    if (!asset) { staticMeshAsset = {}; meshAssetPath = std::string(""); return; }

    staticMeshAsset = asset;
    meshAssetPath   = asset->getPath().str();

    const int newCount = (int)asset->getMeshes().size();
    const int oldCount = (int)materialSlots.size();

    if (newCount > oldCount)
        for (int i = oldCount; i < newCount; ++i) materialSlots.emplace_back();
    else
        materialSlots.erase(materialSlots.begin() + newCount, materialSlots.end());

    calculateBounds();
}

void MStaticMeshEntity::setMaterialAsset(TAssetHandle<MMaterialAsset> asset, int slotId)
{
    if (slotId < 0 || slotId >= (int)materialSlots.size())
    {
        MERROR(SString::format("MStaticMeshEntity::setMaterialAsset: slotId {0} out of range", slotId));
        return;
    }
    materialSlots[slotId].assetHandle = asset;

    if (slotId == 0)
        materialAssetPath = (asset && asset.isValid()) ? asset->getPath().str() : std::string("");
}

MMaterialAsset* MStaticMeshEntity::getMaterialAsset(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return nullptr;
    return materialSlots[slotId].assetHandle.get();
}

MMaterial* MStaticMeshEntity::getMaterialInstance(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return nullptr;
    return materialSlots[slotId].getMaterial();
}

void MStaticMeshEntity::calculateBounds()
{
    if (!staticMeshAsset) return;
    SVector3 mn(FLT_MAX), mx(-FLT_MAX);
    const SMatrix4 world = getTransformMatrix();
    for (const auto* mesh : staticMeshAsset->getMeshes())
        for (const auto& v : mesh->getVertices())
        {
            SVector3 wp = world * SVector4(v.Position, 1.f);
            mn = glm::min(mn, wp); mx = glm::max(mx, wp);
        }
    bounds.min = mn; bounds.max = mx;
}