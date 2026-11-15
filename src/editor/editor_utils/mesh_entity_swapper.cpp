#include "mesh_entity_swapper.h"

#include <vector>

#include "core/engine/3d/skeletal/skeletal_mesh_asset.h"
#include "core/engine/3d/skeletal/skeletal_mesh_entity.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"
#include "editor/app/editorapplication.h"

namespace
{
    MAsset* findAsset(const SString& assetId)
    {
        auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        return am ? am->getAssetById<MAsset>(assetId).get() : nullptr;
    }

    std::vector<MSpatialEntity*> collectMismatched(const SString& assetId)
    {
        std::vector<MSpatialEntity*> result;
        auto* sceneManager = MEngineSubsystemRegistry::getSubsystem<MSceneManager>();
        MScene* scene = sceneManager ? sceneManager->getActiveScene() : nullptr;
        MAsset* asset = findAsset(assetId);
        if (!scene || !asset || assetId.empty())
            return result;

        const bool isSkeletal = dynamic_cast<MSkeletalMeshAsset*>(asset) != nullptr;
        const bool isStatic   = dynamic_cast<MStaticMeshAsset*>(asset) != nullptr;

        for (const auto& [object, ptr] : scene->getAllEntities())
        {
            MSpatialEntity* entity = ptr.get();
            if (!entity || entity->hasExited()) continue;

            if (isSkeletal)
                if (auto* sme = dynamic_cast<MStaticMeshEntity*>(entity))
                    if (sme->meshAsset.get().getAssetId() == assetId)
                        result.push_back(entity);
            if (isStatic)
                if (auto* ske = dynamic_cast<MSkeletalMeshEntity*>(entity))
                    if (ske->meshAsset.get().getAssetId() == assetId)
                        result.push_back(entity);
        }
        return result;
    }

    // Name, parent, transform, flags, script, material slots, children.
    void moveCommonState(MMeshEntityBase* from, MMeshEntityBase* to)
    {
        to->setName(from->getName());
        if (auto* parent = from->getParent())
            to->setParent(parent);
        to->setRelativePosition(from->getRelativePosition());
        to->setRelativeRotation(from->getRelativeRotation());
        to->setRelativeScale(from->getRelativeScale());
        to->setEnabled(from->getEnabled());
        to->setCastsShadow(from->getCastsShadow());
        if (MAsset* script = from->getScriptAssetRef())
            to->setScriptAssetRef(script);

        const int slots = std::min(from->getMaterialSlotCount(), to->getMaterialSlotCount());
        for (int i = 0; i < slots; ++i)
            to->setMaterialAsset(TAssetRef<MMaterialAsset>(from->getMaterialAsset(i)), i);

        const std::vector<MSpatialEntity*> children = from->getChildren();   // copy: reparenting edits the list
        for (auto* child : children)
            if (child) child->setParent(to);
    }
}

namespace MMeshEntitySwapper
{
    int countMismatched(const SString& assetId)
    {
        return static_cast<int>(collectMismatched(assetId).size());
    }

    int swapMismatched(const SString& assetId)
    {
        MAsset* asset = findAsset(assetId);
        int swapped = 0;
        for (MSpatialEntity* entity : collectMismatched(assetId))
        {
            MMeshEntityBase* replacement = nullptr;
            if (auto* skeletal = dynamic_cast<MSkeletalMeshAsset*>(asset))
            {
                auto* created = MSpatialEntity::createInstance<MSkeletalMeshEntity>(entity->getName());
                created->setSkeletalMeshAsset(TAssetRef<MSkeletalMeshAsset>(skeletal));
                replacement = created;
            }
            else if (auto* staticAsset = dynamic_cast<MStaticMeshAsset*>(asset))
            {
                auto* created = MSpatialEntity::createInstance<MStaticMeshEntity>(entity->getName());
                created->setStaticMeshAsset(TAssetRef<MStaticMeshAsset>(staticAsset));
                replacement = created;
            }
            if (!replacement) continue;

            moveCommonState(dynamic_cast<MMeshEntityBase*>(entity), replacement);
            if (MEditorApplication::SelectedObject == entity)
                MEditorApplication::SelectedObject = replacement;
            entity->destroy();
            ++swapped;
        }
        if (swapped > 0)
            MLOG(SString::format("MeshEntitySwapper:: swapped {0} entities", SString::fromInt(swapped)));
        return swapped;
    }
}
