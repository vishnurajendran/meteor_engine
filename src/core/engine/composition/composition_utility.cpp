//
// composition_utility.cpp
//

#include "composition_utility.h"

#include <algorithm>

#include "composition_asset.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/utils/logger.h"

std::function<void(MSpatialEntity*, MSpatialEntity*)> MCompositionUtility::onInstanceReplaced;

namespace
{
MScene* activeScene()
{
    auto* sm = MSceneManager::getSceneManagerInstance();
    return sm ? sm->getActiveScene() : nullptr;
}

int indexOf(const std::vector<MSpatialEntity*>& list, const MSpatialEntity* entity)
{
    const auto it = std::ranges::find(list, entity);
    return it == list.end() ? static_cast<int>(list.size())
                            : static_cast<int>(std::distance(list.begin(), it));
}

// Name, enabled flag and transform belong to the instance, not the asset.
void copyInstanceRootState(MSpatialEntity* from, MSpatialEntity* to)
{
    to->setName(from->getName());
    to->setEnabled(from->getEnabled());
    to->setRelativePosition(from->getRelativePosition());
    to->setRelativeRotation(from->getRelativeRotation());
    to->setRelativeScale(from->getRelativeScale());
}

// Same as above, but read from a scene node (scene loading path).
// Mirrors what MSpatialEntity::onDeserialise reads.
void applySceneRootState(const pugi::xml_node& node, MSpatialEntity* to)
{
    if (auto a = node.attribute("name"))    to->setName(SString(a.value()));
    if (auto a = node.attribute("enabled")) to->setEnabled(a.as_bool(true));

    std::vector<FieldBase*> tmp;
    Field<SVector3>    pos  (tmp, "relativePosition", to->getRelativePosition());
    Field<SQuaternion> rot  (tmp, "relativeRotation", to->getRelativeRotation());
    Field<SVector3>    scale(tmp, "relativeScale",    to->getRelativeScale());
    for (auto* f : tmp) f->load(node);

    to->setRelativePosition(pos.get());
    to->setRelativeRotation(rot.get());
    to->setRelativeScale(scale.get());
}
} // namespace

// ---- Queries --------------------------------------------------------------------

MSpatialEntity* MCompositionUtility::findInstanceRoot(MSpatialEntity* entity)
{
    for (MSpatialEntity* e = entity; e; e = e->getParent())
        if (!e->compAssetReference.get().isEmpty())
            return e;
    return nullptr;
}

MCompositionAsset* MCompositionUtility::getAsset(const MSpatialEntity* instanceRoot)
{
    if (!instanceRoot)
        return nullptr;
    return instanceRoot->compAssetReference.get().resolve();
}

SString MCompositionUtility::computeInstanceHash(MSpatialEntity* instanceRoot)
{
    if (!instanceRoot)
        return SString();
    pugi::xml_document doc;
    const pugi::xml_node node = instanceRoot->serialiseEntity(doc);
    return MCompositionAsset::computeContentHash(node);
}

ECompositionInstanceState MCompositionUtility::getState(MSpatialEntity* instanceRoot)
{
    if (!instanceRoot || instanceRoot->compAssetReference.get().isEmpty())
        return ECompositionInstanceState::NotLinked;

    MCompositionAsset* asset = getAsset(instanceRoot);
    if (!asset || !asset->isValid())
        return ECompositionInstanceState::MissingAsset;

    const SString& stateHash = instanceRoot->getCompStateHash();
    if (!stateHash.empty() && computeInstanceHash(instanceRoot) != stateHash)
        return ECompositionInstanceState::Modified;

    if (instanceRoot->getCompSourceHash() != asset->getContentHash())
        return ECompositionInstanceState::OutOfDate;

    return ECompositionInstanceState::Synced;
}

// ---- Creating / linking -----------------------------------------------------------

bool MCompositionUtility::writeNewAsset(MSpatialEntity* entity, const SString& path)
{
    if (!entity)
        return false;
    pugi::xml_document doc;
    const pugi::xml_node node = entity->serialiseEntity(doc);
    return MCompositionAsset::writeFile(path, node);
}

void MCompositionUtility::link(MSpatialEntity* entity, MCompositionAsset* asset)
{
    if (!entity || !asset)
        return;
    entity->compAssetReference = TAssetRef<MCompositionAsset>(asset);
    entity->setCompositionHashes(asset->getContentHash(), computeInstanceHash(entity));
}

void MCompositionUtility::unlink(MSpatialEntity* instanceRoot)
{
    if (!instanceRoot)
        return;
    instanceRoot->compAssetReference = TAssetRef<MCompositionAsset>();
    instanceRoot->setCompositionHashes(SString(), SString());
}

MSpatialEntity* MCompositionUtility::instantiate(MCompositionAsset* asset)
{
    if (!asset || !asset->isValid())
        return nullptr;

    // Asset content is already flattened (no links inside), so there is
    // nothing to resolve — skip the composition check for speed.
    MSpatialEntity* root = MSpatialEntity::deserialiseEntity(asset->getRootEntityNode(), false);
    if (!root)
    {
        MERROR(SString("Composition:: failed to instantiate ") + asset->getPath());
        return nullptr;
    }

    root->updateTransforms();
    link(root, asset);
    return root;
}

// ---- Instance actions -------------------------------------------------------------

bool MCompositionUtility::saveInstance(MSpatialEntity* instanceRoot)
{
    MCompositionAsset* asset = getAsset(instanceRoot);
    if (!asset)
    {
        MERROR("Composition:: save failed — instance has no valid asset");
        return false;
    }

    pugi::xml_document doc;
    const pugi::xml_node node = instanceRoot->serialiseEntity(doc);
    if (!asset->writeContent(node))
        return false;

    // This instance now IS the asset content — mark it synced before the
    // refresh so it isn't rebuilt.
    instanceRoot->setCompositionHashes(asset->getContentHash(), computeInstanceHash(instanceRoot));
    refreshInstances(asset);

    MLOG(SString("Composition:: saved ") + asset->getPath());
    return true;
}

MSpatialEntity* MCompositionUtility::resetInstance(MSpatialEntity* instanceRoot)
{
    MCompositionAsset* asset = getAsset(instanceRoot);
    if (!asset || !asset->isValid())
    {
        MERROR("Composition:: reset failed — instance has no valid asset");
        return nullptr;
    }

    MScene* scene = activeScene();
    if (!scene)
        return nullptr;

    MSpatialEntity* newRoot = MSpatialEntity::deserialiseEntity(asset->getRootEntityNode(), false);
    if (!newRoot)
    {
        MERROR(SString("Composition:: failed to rebuild from ") + asset->getPath());
        return nullptr;
    }

    copyInstanceRootState(instanceRoot, newRoot);

    // Take the old root's place in the hierarchy (it moves one slot down and
    // is removed when the scene processes destroyed entities).
    if (MSpatialEntity* parent = instanceRoot->getParent())
    {
        parent->insertChildAt(newRoot, indexOf(parent->getChildren(), instanceRoot));
    }
    else
    {
        scene->insertRootEntityAt(newRoot, indexOf(scene->getRootEntities(), instanceRoot));
        newRoot->updateTransforms();
    }

    newRoot->compAssetReference = TAssetRef<MCompositionAsset>(asset);
    newRoot->setCompositionHashes(asset->getContentHash(), computeInstanceHash(newRoot));

    destroySubtree(instanceRoot);

    if (onInstanceReplaced)
        onInstanceReplaced(instanceRoot, newRoot);

    return newRoot;
}

void MCompositionUtility::refreshInstances(MCompositionAsset* asset)
{
    MScene* scene = activeScene();
    if (!asset || !asset->isValid() || !scene)
        return;

    // Collect first — resetting changes the hierarchy we'd be walking.
    std::vector<MSpatialEntity*> instances;
    for (MSpatialEntity* root : std::vector<MSpatialEntity*>(scene->getRootEntities()))
        collectInstances(root, asset->getAssetId(), instances);

    int rebuilt = 0, skipped = 0;
    for (MSpatialEntity* instance : instances)
    {
        switch (getState(instance))
        {
        case ECompositionInstanceState::OutOfDate:
            if (resetInstance(instance)) ++rebuilt;
            break;
        case ECompositionInstanceState::Modified:
            ++skipped;   // never overwrite unsaved instance edits
            break;
        default:
            break;
        }
    }

    if (rebuilt || skipped)
        MLOG(SString::format("Composition:: {0} — rebuilt {1}, kept {2} modified instance(s)",
                             asset->getPath(), std::to_string(rebuilt), std::to_string(skipped)));
}

// ---- Scene loading ----------------------------------------------------------------

MSpatialEntity* MCompositionUtility::tryBuildFromAsset(const pugi::xml_node& sceneNode)
{
    // Every entity writes this field (empty for non-instances) — bail out
    // cheaply for the common case before doing any asset lookups.
    const pugi::xml_node linkNode = sceneNode.child(MCompositionAsset::LINK_FIELD_NAME);
    if (!linkNode || (linkNode.text().empty() && !linkNode.first_child()))
        return nullptr;

    // Read the link with the same Field code the entity would use.
    std::vector<FieldBase*> tmp;
    Field<TAssetRef<MCompositionAsset>> linkField(tmp, MCompositionAsset::LINK_FIELD_NAME, {});
    linkField.load(sceneNode);

    MCompositionAsset* asset = linkField.get().resolve();
    if (!asset || !asset->isValid())
        return nullptr;   // missing asset — keep the snapshot so nothing is lost

    const SString stateHash (sceneNode.attribute(MCompositionAsset::STATE_HASH_ATTRIBUTE).as_string(""));
    const SString sourceHash(sceneNode.attribute(MCompositionAsset::SOURCE_HASH_ATTRIBUTE).as_string(""));

    // No state hash (e.g. a hand-written node with only the reference) counts as clean.
    const bool modified = !stateHash.empty()
                          && MCompositionAsset::computeContentHash(sceneNode) != stateHash;
    const bool stale    = sourceHash != asset->getContentHash();

    if (modified || !stale)
        return nullptr;   // snapshot is either the user's edits or already current

    MSpatialEntity* entity = MSpatialEntity::deserialiseEntity(asset->getRootEntityNode(), false);
    if (!entity)
        return nullptr;

    applySceneRootState(sceneNode, entity);
    entity->compAssetReference = TAssetRef<MCompositionAsset>(asset);
    entity->setCompositionHashes(asset->getContentHash(), computeInstanceHash(entity));

    MLOG(SString("Composition:: refreshed instance '") + entity->getName() + "' from " + asset->getPath());
    return entity;
}

// ---- Helpers ----------------------------------------------------------------------

void MCompositionUtility::destroySubtree(MSpatialEntity* entity)
{
    if (!entity)
        return;
    // MSpatialEntity::destroy() only marks the entity itself, so walk the
    // children explicitly — otherwise they'd stay alive, orphaned.
    for (MSpatialEntity* child : std::vector<MSpatialEntity*>(entity->getChildren()))
        destroySubtree(child);
    entity->destroy();
}

void MCompositionUtility::collectInstances(MSpatialEntity* entity, const SString& assetId,
                                           std::vector<MSpatialEntity*>& out)
{
    if (!entity)
        return;

    if (entity->compAssetReference.get().getAssetId() == assetId)
    {
        // Don't descend — resetting this root replaces everything below it.
        out.push_back(entity);
        return;
    }

    for (MSpatialEntity* child : entity->getChildren())
        collectInstances(child, assetId, out);
}
