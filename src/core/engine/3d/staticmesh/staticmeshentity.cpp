#include "staticmeshentity.h"

#include <cfloat>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/interfaces/render_item_collector.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "staticmesh.h"
#include "staticmeshasset.h"

IMPLEMENT_SPATIAL_CLASS(MStaticMeshEntity)

MStaticMeshEntity::MStaticMeshEntity()
{
    name                = "StaticMeshEntity";
    prevTransformMatrix = SMatrix4(0.f);
}

// -- serialization -----------------------------------------------------------

void MStaticMeshEntity::onSerialise(pugi::xml_node& node)
{
    MSpatialEntity::onSerialise(node);
    // DECLARE_FIELDs (meshAsset, castsShadow) are written automatically by
    // SerializedClassBase before this method is called.
    serialiseMaterialSlots(node);
}

void MStaticMeshEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    // At this point DECLARE_FIELDs are already loaded by SerializedClassBase.

    // -- backward compat: old <meshAssetPath> string field -------------------
    // Older scene files wrote a plain path string under <meshAssetPath>.
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

    // Resize material slots to the sub-mesh count before reading them.
    if (auto* resolvedMesh = meshAsset.get().resolve())
        resizeMaterialSlots((int)resolvedMesh->getMeshes().size());
    else
        warnIfWrongAssetType();

    deserialiseMaterialSlots(node);
    calculateBounds();
}

void MStaticMeshEntity::warnIfWrongAssetType()
{
    if (warnedWrongType || meshAsset.get().isEmpty())
        return;

    auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!am)
        return;

    const auto& ref = meshAsset.get();
    MAsset* other = !ref.getAssetId().empty() ? am->getAssetById<MAsset>(ref.getAssetId()).get() : nullptr;
    if (!other && !ref.getPath().empty())
        other = am->getAsset<MAsset>(ref.getPath()).get();

    if (other)
    {
        warnedWrongType = true;
        MWARN(SString::format("'{0}': mesh '{1}' is no longer a static mesh (it imports as {2}). "
                              "Use a Skeletal Mesh entity, or set the model's Import As back to Static Mesh.",
                              getName(), other->getPath(), SString(other->typeInfo().name)));
    }
}

// -- rendering ---------------------------------------------------------------

void MStaticMeshEntity::submitRenderItem(IRenderItemCollector* collector)
{
    if (!isEnabledInHierarchy())
        return;

    auto* meshPtr = meshAsset.get().resolve();
    if (!meshPtr)
    {
        warnIfWrongAssetType();
        return;
    }

    const SMatrix4 current = getTransformMatrix();
    if (current != prevTransformMatrix) { prevTransformMatrix = current; calculateBounds(); }

    const auto meshes    = meshPtr->getMeshes();
    const int  drawCount = std::min((int)meshes.size(), (int)materialSlots.size());

    for (int i = 0; i < drawCount; ++i)
    {
        const auto* mesh = meshes[i];
        if (!mesh) continue;

        MMaterial* mat = resolveSlotMaterialOrError(i);
        if (!mat) continue;

        SRenderItem item;
        item.vao         = mesh->getVAO();
        item.ebo         = mesh->getEBO();
        item.indexCount  = mesh->getIndexCount();
        item.vertexCount = mesh->getVertexCount();
        item.transform   = current;
        fillCommonRenderItem(item, mat);
        collector->submitRenderItem(item);
    }
}

// -- lifecycle ---------------------------------------------------------------

void MStaticMeshEntity::onUpdate(float dt)
{
    MSpatialEntity::onUpdate(dt);
    const SMatrix4 m = getTransformMatrix();
    if (m != prevTransformMatrix) { prevTransformMatrix = m; calculateBounds(); }
}

// -- mutators ----------------------------------------------------------------

void MStaticMeshEntity::setStaticMeshAsset(TAssetRef<MStaticMeshAsset> asset)
{
    meshAsset = asset;
    warnedWrongType = false;

    auto* resolved = meshAsset.get().resolve();
    resizeMaterialSlots(resolved ? (int)resolved->getMeshes().size() : 0);
    if (resolved)
        calculateBounds();
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
