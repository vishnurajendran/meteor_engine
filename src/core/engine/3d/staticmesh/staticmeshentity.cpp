#include "staticmeshentity.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "staticmesh.h"
#include "staticmeshasset.h"

IMPLEMENT_SPATIAL_CLASS(MStaticMeshEntity)

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
    errorMaterialInstance = nullptr;
}

// -- serialization -----------------------------------------------------------

void MStaticMeshEntity::onSerialise(pugi::xml_node& node)
{
    MSpatialEntity::onSerialise(node);
    // DECLARE_FIELDs (meshAsset, castsShadow) are written automatically by
    // SerializedClassBase before this method is called.

    // Write material slots manually since they are a dynamic-length array.
    // Each slot stores both asset ID and path for the same fallback behavior
    // that TAssetRef provides.
    //
    // Format:
    //   <materialSlots>
    //       <slot index="0">
    //           <id>guid-string</id>
    //           <path>assets/materials/brick.material</path>
    //       </slot>
    //   </materialSlots>
    auto slotsNode = node.append_child("materialSlots");
    for (int i = 0; i < (int)materialSlots.size(); ++i)
    {
        auto slotNode = slotsNode.append_child("slot");
        slotNode.append_attribute("index").set_value(i);
        const auto& ref = materialSlots[i].assetRef;
        slotNode.append_child("id").text().set(ref.getAssetId().c_str());
        slotNode.append_child("path").text().set(ref.getPath().c_str());
    }
}

void MStaticMeshEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    // At this point DECLARE_FIELDs are already loaded by SerializedClassBase.
    // meshAsset.get() has its id and/or path populated from the <meshAsset>
    // XML node (if present in the file).

    // -- backward compat: old <meshAssetPath> string field -------------------
    // Older scene files wrote a plain path string under <meshAssetPath>.
    // That DECLARE_FIELD no longer exists, so the node was not auto-loaded.
    // Check for it and populate meshAsset from the path if needed.
    if (meshAsset.get().isEmpty())
    {
        auto oldNode = node.child("meshAssetPath");
        if (oldNode)
        {
            SString oldPath = oldNode.text().as_string("");
            if (!oldPath.empty())
                meshAsset = TAssetRef<MStaticMeshAsset>(SString{}, oldPath);
        }
    }

    // Resolve the mesh and resize material slots to match sub-mesh count.
    // This must happen before reading material slot data from XML.
    auto* resolvedMesh = meshAsset.get().resolve();
    if (resolvedMesh)
    {
        const int meshCount = (int)resolvedMesh->getMeshes().size();
        const int oldCount  = (int)materialSlots.size();
        if (meshCount > oldCount)
            materialSlots.resize(meshCount);
        else if (meshCount < oldCount)
            materialSlots.erase(materialSlots.begin() + meshCount, materialSlots.end());
    }

    // -- read material slots -------------------------------------------------
    auto slotsNode = node.child("materialSlots");
    if (slotsNode)
    {
        for (auto slotNode : slotsNode.children("slot"))
        {
            int idx = slotNode.attribute("index").as_int(-1);
            if (idx < 0 || idx >= (int)materialSlots.size()) continue;

            // New format: <slot> has <id> and <path> children.
            auto idNode   = slotNode.child("id");
            auto pathNode = slotNode.child("path");

            if (idNode || pathNode)
            {
                SString id   = idNode   ? SString(idNode.text().as_string(""))   : SString{};
                SString path = pathNode ? SString(pathNode.text().as_string("")) : SString{};
                materialSlots[idx].assetRef = TAssetRef<MMaterialAsset>(id, path);
            }
            else
            {
                // Old format: slot text content is the path directly.
                //   <slot index="0">assets/materials/brick.material</slot>
                SString matPath = slotNode.text().as_string("");
                if (!matPath.empty())
                    materialSlots[idx].assetRef = TAssetRef<MMaterialAsset>(SString{}, matPath);
            }
        }
    }
    else
    {
        // Legacy format: single <materialAssetPath> string (slot 0 only).
        auto oldMatNode = node.child("materialAssetPath");
        if (oldMatNode)
        {
            SString matPath = oldMatNode.text().as_string("");
            if (!matPath.empty() && !materialSlots.empty())
                materialSlots[0].assetRef = TAssetRef<MMaterialAsset>(SString{}, matPath);
        }
    }

    calculateBounds();
}

// -- rendering ---------------------------------------------------------------

void MStaticMeshEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (!isEnabledInHierarchy())
        return;

    // Resolve the mesh once per frame. Avoids repeated map lookups inside
    // the per-sub-mesh loop.
    auto* meshPtr = meshAsset.get().resolve();
    if (!meshPtr) return;

    const SMatrix4 current = getTransformMatrix();
    if (current != prevTransformMatrix) { prevTransformMatrix = current; calculateBounds(); }

    // Lazy-load error material once.
    if (!errorMaterialInstance)
    {
        const auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
            ->getAsset<MMaterialAsset>("meteor_assets/engine_assets/materials/error_material.material");
        if (asset) errorMaterialInstance = asset->getMaterial();
    }

    const auto meshes    = meshPtr->getMeshes();
    const int  drawCount = std::min((int)meshes.size(), (int)materialSlots.size());

    for (int i = 0; i < drawCount; ++i)
    {
        const auto* mesh = meshes[i];
        if (!mesh) continue;

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

// -- lifecycle ---------------------------------------------------------------

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

void MStaticMeshEntity::onDrawGizmo(SVector2 res)
{
    MSpatialEntity::onDrawGizmo(res);

#if METEOR_EDITOR
    if (!debug_drawBounds)
        return;

    const auto& bounds = getBounds();
    const SVector3& extents = {
        (bounds.max.x - bounds.min.x)/2,
        (bounds.max.y - bounds.min.y)/2,
        (bounds.max.z - bounds.min.z)/2
    };

    MGizmos::drawWireCube(bounds.getCentre(), extents, SColor::yellow(), 2.0f);

#endif

}

// -- mutators ----------------------------------------------------------------

void MStaticMeshEntity::setStaticMeshAsset(TAssetRef<MStaticMeshAsset> asset)
{
    meshAsset = asset;

    auto* resolved = meshAsset.get().resolve();
    if (!resolved)
    {
        materialSlots.clear();
        materialSlots.resize(1);
        return;
    }

    const int newCount = (int)resolved->getMeshes().size();
    const int oldCount = (int)materialSlots.size();

    if (newCount > oldCount)
        for (int i = oldCount; i < newCount; ++i) materialSlots.emplace_back();
    else
        materialSlots.erase(materialSlots.begin() + newCount, materialSlots.end());

    calculateBounds();
}

void MStaticMeshEntity::setMaterialAsset(TAssetRef<MMaterialAsset> asset, int slotId)
{
    if (slotId < 0 || slotId >= (int)materialSlots.size())
    {
        MERROR(SString::format("MStaticMeshEntity::setMaterialAsset: slotId {0} out of range", slotId));
        return;
    }
    materialSlots[slotId].assetRef = asset;
}

void MStaticMeshEntity::swapMaterialSlots(int a, int b)
{
    if (a < 0 || a >= (int)materialSlots.size()) return;
    if (b < 0 || b >= (int)materialSlots.size()) return;
    if (a == b) return;
    std::swap(materialSlots[a], materialSlots[b]);
}

TAssetHandle<MMaterialAsset> MStaticMeshEntity::getMaterialAsset(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return TAssetHandle<MMaterialAsset>();
    return materialSlots[slotId].assetRef.getHandle();
}

MMaterial* MStaticMeshEntity::getMaterialInstance(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return nullptr;
    return materialSlots[slotId].getMaterial();
}

void MStaticMeshEntity::calculateBounds()
{
    auto* meshPtr = meshAsset.get().resolve();
    if (!meshPtr) return;
    SVector3 mn(FLT_MAX), mx(-FLT_MAX);
    const SMatrix4 world = getTransformMatrix();
    for (const auto* mesh : meshPtr->getMeshes())
        for (const auto& v : mesh->getVertices())
        {
            SVector3 wp = world * SVector4(v.Position, 1.f);
            mn = glm::min(mn, wp); mx = glm::max(mx, wp);
        }
    bounds.min = mn; bounds.max = mx;
}