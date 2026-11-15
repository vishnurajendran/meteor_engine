#include "mesh_entity_base.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/graphics/core/render-pipeline/render_queue.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"

namespace
{
    constexpr const char* ERROR_MATERIAL_PATH = "meteor_assets/engine_assets/materials/error_material.material";
}

MMeshEntityBase::MMeshEntityBase()
{
    materialSlots.resize(1);
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().addToSubmitables(this);
}

MMeshEntityBase::~MMeshEntityBase()
{
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().removeFromSubmitables(this);
    errorMaterialInstance = nullptr;
}

void MMeshEntityBase::onExit()
{
    MSpatialEntity::onExit();
    if (auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>())
        renderer->getRenderQueue().removeFromSubmitables(this);
}

// -- Material slots ------------------------------------------------------------

void MMeshEntityBase::resizeMaterialSlots(int subMeshCount)
{
    if (subMeshCount <= 0)
    {
        materialSlots.clear();
        materialSlots.resize(1);
        return;
    }
    const int oldCount = (int)materialSlots.size();
    if (subMeshCount > oldCount)
        materialSlots.resize(subMeshCount);
    else if (subMeshCount < oldCount)
        materialSlots.erase(materialSlots.begin() + subMeshCount, materialSlots.end());
}

void MMeshEntityBase::setMaterialAsset(TAssetRef<MMaterialAsset> asset, int slotId)
{
    if (slotId < 0 || slotId >= (int)materialSlots.size())
    {
        MERROR(SString::format("{0}::setMaterialAsset: slotId {1} out of range",
                               SString(typeInfo().name), SString::fromInt(slotId)));
        return;
    }
    materialSlots[slotId].assetRef = asset;
}

void MMeshEntityBase::swapMaterialSlots(int a, int b)
{
    if (a < 0 || a >= (int)materialSlots.size()) return;
    if (b < 0 || b >= (int)materialSlots.size()) return;
    if (a == b) return;
    std::swap(materialSlots[a], materialSlots[b]);
}

TAssetHandle<MMaterialAsset> MMeshEntityBase::getMaterialAsset(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return TAssetHandle<MMaterialAsset>();
    return materialSlots[slotId].assetRef.getHandle();
}

MMaterial* MMeshEntityBase::getMaterialInstance(int slotId) const
{
    if (slotId < 0 || slotId >= (int)materialSlots.size()) return nullptr;
    return materialSlots[slotId].getMaterial();
}

MMaterial* MMeshEntityBase::resolveSlotMaterialOrError(int slotId)
{
    if (!errorMaterialInstance)
    {
        if (auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>())
        {
            const auto asset = am->getAsset<MMaterialAsset>(ERROR_MATERIAL_PATH);
            if (asset) errorMaterialInstance = asset->getMaterial();
        }
    }

    MMaterial* mat = getMaterialInstance(slotId);
    return mat ? mat : errorMaterialInstance;
}

void MMeshEntityBase::fillCommonRenderItem(SRenderItem& item, MMaterial* material) const
{
    item.material    = material;
    item.bounds      = bounds;
    item.castsShadow = castsShadow.get();
    item.sortOrder   = static_cast<int>(material && material->isTransparent() ? ERenderStageOrder::RS_Transparent
                                                                              : ERenderStageOrder::RS_Opaque);
}

// -- Serialization ---------------------------------------------------------------
//
//   <materialSlots>
//       <slot index="0">guid:3d4c5ab2-...</slot>
//   </materialSlots>
//
// Each slot uses the same reference format as Field<TAssetRef<T>>:
// "guid:<id>" when known, otherwise the bare path.

void MMeshEntityBase::serialiseMaterialSlots(pugi::xml_node& node) const
{
    auto slotsNode = node.append_child("materialSlots");
    for (int i = 0; i < (int)materialSlots.size(); ++i)
    {
        auto slotNode = slotsNode.append_child("slot");
        slotNode.append_attribute("index").set_value(i);
        const auto& ref = materialSlots[i].assetRef;
        ref.resolve(); // upgrade path-only refs to a GUID before writing
        const SString refStr = !ref.getAssetId().empty()
            ? SString(ASSET_REF_GUID_PREFIX) + ref.getAssetId()
            : ref.getPath();
        slotNode.text().set(refStr.c_str());
    }
}

// The slot list must already be sized to the mesh (resizeMaterialSlots) -
// indices past the end are dropped.
void MMeshEntityBase::deserialiseMaterialSlots(const pugi::xml_node& node)
{
    auto slotsNode = node.child("materialSlots");
    if (slotsNode)
    {
        for (auto slotNode : slotsNode.children("slot"))
        {
            int idx = slotNode.attribute("index").as_int(-1);
            if (idx < 0 || idx >= (int)materialSlots.size()) continue;

            // Legacy format: <slot> has <id> and <path> children.
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
                // Current format — slot text is a reference string:
                //   <slot index="0">guid:3d4c5ab2-...</slot>
                //   <slot index="0">assets/materials/brick.material</slot>   (path — also the oldest format)
                SString ref = slotNode.text().as_string("");
                if (IAssetManagerSubsystem::isGuidReference(ref))
                    materialSlots[idx].assetRef = TAssetRef<MMaterialAsset>(SString(ref.str().substr(ASSET_REF_GUID_PREFIX_LEN)));
                else if (!ref.empty())
                    materialSlots[idx].assetRef = TAssetRef<MMaterialAsset>(SString{}, ref);
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
}

// -- Gizmos ----------------------------------------------------------------------

void MMeshEntityBase::onDrawGizmo(SVector2 res)
{
    MSpatialEntity::onDrawGizmo(res);

#if METEOR_EDITOR
    if (!debug_drawBounds)
        return;

    const SVector3 extents = (bounds.max - bounds.min) * 0.5f;
    MGizmos::drawWireCube(bounds.getCentre(), extents, SColor::yellow(), 2.0f);
#endif
}
