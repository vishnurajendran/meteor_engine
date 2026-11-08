//
// entity_duplicator.cpp
//

#include "entity_duplicator.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "pugixml.hpp"

#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/utils/logger.h"
#include "editor/app/editorapplication.h"

namespace
{
// Root element of entity text (clipboard). Bump the version if the payload
// changes in a way older editors can't read.
constexpr const char* ROOT_TAG     = "meteor_entities";
constexpr const char* VERSION_ATTR = "version";
constexpr int         VERSION      = 1;

// "Cube (3)" -> "Cube", "Cube" -> "Cube". Only strips a trailing " (<digits>)".
std::string stripCopySuffix(const std::string& name)
{
    if (name.size() < 4 || name.back() != ')')
        return name;

    const auto open = name.rfind(" (");
    if (open == std::string::npos)
        return name;

    const auto digitsBegin = open + 2;
    const auto digitsEnd   = name.size() - 1;
    if (digitsBegin >= digitsEnd)
        return name;

    for (auto i = digitsBegin; i < digitsEnd; ++i)
        if (!std::isdigit(static_cast<unsigned char>(name[i])))
            return name;

    return name.substr(0, open);
}

int indexOf(const std::vector<MSpatialEntity*>& list, const MSpatialEntity* entity)
{
    const auto it = std::ranges::find(list, entity);
    return it == list.end() ? static_cast<int>(list.size())
                            : static_cast<int>(std::distance(list.begin(), it));
}

MScene* activeScene()
{
    auto* sceneManager = MEngineSubsystemRegistry::getSubsystem<MSceneManager>();
    return sceneManager ? sceneManager->getActiveScene() : nullptr;
}

// Builds one entity subtree from its scene XML node and inserts it under
// `parent` (scene root when null) at `index` (clamped; -1 = end).
MSpatialEntity* instantiateNode(const pugi::xml_node& node, MScene* scene, MSpatialEntity* parent, int index)
{
    // deserialiseEntity registers every entity in the subtree with the active
    // scene; the new root lands at the end of the scene root list and is
    // moved into place below.
    // resolveCompositions = false - copy a composition instance exactly as it
    // is (including unsaved edits) instead of rebuilding it from its .comp.
    // The copy keeps the link, so it is another instance of the same asset.
    MSpatialEntity* clone = MSpatialEntity::deserialiseEntity(node, false);
    if (!clone)
        return nullptr;

    if (parent)
    {
        const int count = static_cast<int>(parent->getChildren().size());
        parent->insertChildAt(clone, (index < 0 || index > count) ? count : index);   // updates transforms
    }
    else
    {
        // The clone is already at the end of the root list; take it out of the
        // count so "end" means after every other root.
        const int count = static_cast<int>(scene->getRootEntities().size()) - 1;
        scene->insertRootEntityAt(clone, (index < 0 || index > count) ? count : index);
        clone->updateTransforms();
    }
    return clone;
}
} // namespace

// ---- Halves -------------------------------------------------------------------

SString MEntityDuplicator::serialiseToText(const MSpatialEntity* source)
{
    if (!canDuplicate(source))
        return {};

    // Serialise the subtree exactly as a scene save would.
    pugi::xml_document doc;
    auto root = doc.append_child(ROOT_TAG);
    root.append_attribute(VERSION_ATTR).set_value(VERSION);
    if (!source->serialiseEntity(root))
    {
        MERROR(SString("Copy:: failed to serialise ") + source->getName());
        return {};
    }

    std::ostringstream out;
    doc.save(out, "  ");
    return SString(out.str().c_str());
}

bool MEntityDuplicator::isEntityText(const SString& text)
{
    // Cheap check first - this runs whenever a menu asks whether Paste is
    // available, on whatever happens to be on the clipboard.
    if (text.empty() || text.str().find(ROOT_TAG) == std::string::npos)
        return false;

    pugi::xml_document doc;
    if (!doc.load_string(text.c_str()))
        return false;
    const auto root = doc.child(ROOT_TAG);
    return root && root.first_child();
}

std::vector<MSpatialEntity*> MEntityDuplicator::instantiateFromText(const SString& text,
                                                                     MSpatialEntity* parent, int index)
{
    std::vector<MSpatialEntity*> created;

    MScene* scene = activeScene();
    if (!scene)
    {
        MWARN("Paste:: no active scene");
        return created;
    }

    pugi::xml_document doc;
    if (!doc.load_string(text.c_str()) || !doc.child(ROOT_TAG))
    {
        MWARN("Paste:: clipboard does not hold entities");
        return created;
    }

    const auto root = doc.child(ROOT_TAG);
    if (root.attribute(VERSION_ATTR).as_int(0) > VERSION)
        MWARN("Paste:: entities were copied from a newer editor; some data may not load");

    for (auto node : root.children())
    {
        MSpatialEntity* clone = instantiateNode(node, scene, parent, index);
        if (!clone)
        {
            MERROR("Paste:: failed to rebuild an entity");
            continue;
        }
        created.push_back(clone);
        if (index >= 0) ++index;   // keep multiple roots in order
    }
    return created;
}

void MEntityDuplicator::makeNameUnique(MSpatialEntity* entity, bool keepIfFree, const MSpatialEntity* ignore)
{
    if (!entity) return;

    MScene* scene = activeScene();
    std::vector<MSpatialEntity*> empty;
    const auto& siblings = entity->getParent() ? entity->getParent()->getChildren()
                         : scene               ? scene->getRootEntities()
                                               : empty;

    auto taken = [&](const std::string& candidate)
    {
        return std::ranges::any_of(siblings, [&](const MSpatialEntity* e)
        {
            return e && e != entity && e != ignore && e->getName().str() == candidate;
        });
    };

    const std::string current = entity->getName().str();
    if (keepIfFree && !taken(current))
        return;

    const std::string base = stripCopySuffix(current);
    for (int i = 1; ; ++i)
    {
        std::string candidate = base + " (" + std::to_string(i) + ")";
        if (!taken(candidate))
        {
            entity->setName(SString(candidate.c_str()));
            return;
        }
    }
}

// ---- Duplicate ------------------------------------------------------------------

bool MEntityDuplicator::canDuplicate(const MSpatialEntity* source)
{
    if (!source)
        return false;
    // Editor-only helpers (scene camera etc.) are never saved, so never copied.
    return (source->getEntityFlags() & EEntityFlags::HideInEditor) != EEntityFlags::HideInEditor;
}

MSpatialEntity* MEntityDuplicator::duplicate(MSpatialEntity* source)
{
    // Copy, then paste right after the original under the same parent.
    const SString text = serialiseToText(source);
    if (text.empty())
        return nullptr;

    MSpatialEntity* parent = source->getParent();
    int index = 0;
    if (parent)
        index = indexOf(parent->getChildren(), source) + 1;
    else if (MScene* scene = activeScene())
        index = indexOf(scene->getRootEntities(), source) + 1;

    const auto created = instantiateFromText(text, parent, index);
    if (created.empty())
    {
        MERROR(SString("Duplicate:: failed to rebuild ") + source->getName());
        return nullptr;
    }

    MSpatialEntity* clone = created.front();
    makeNameUnique(clone, false);
    MLOG(SString::format("Duplicate:: {0} -> {1}", source->getName(), clone->getName()));
    return clone;
}

MSpatialEntity* MEntityDuplicator::duplicateAndSelect(MObject* object)
{
    auto* source = dynamic_cast<MSpatialEntity*>(object);
    MSpatialEntity* clone = duplicate(source);
    if (clone)
        MEditorApplication::SelectedObject = clone;
    return clone;
}
