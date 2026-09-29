//
// entity_duplicator.cpp
//

#include "entity_duplicator.h"

#include <algorithm>
#include <cctype>
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

// First "<base> (N)" (N >= 1) not used by any entity in `siblings`.
SString makeUniqueSiblingName(const SString& sourceName,
                              const std::vector<MSpatialEntity*>& siblings)
{
    const std::string base = stripCopySuffix(sourceName.str());

    auto taken = [&](const std::string& candidate)
    {
        return std::ranges::any_of(siblings, [&](const MSpatialEntity* e)
        {
            return e && e->getName().str() == candidate;
        });
    };

    for (int i = 1; ; ++i)
    {
        std::string candidate = base + " (" + std::to_string(i) + ")";
        if (!taken(candidate))
            return SString(candidate.c_str());
    }
}

int indexOf(const std::vector<MSpatialEntity*>& list, const MSpatialEntity* entity)
{
    const auto it = std::ranges::find(list, entity);
    return it == list.end() ? static_cast<int>(list.size())
                            : static_cast<int>(std::distance(list.begin(), it));
}
} // namespace

bool MEntityDuplicator::canDuplicate(const MSpatialEntity* source)
{
    if (!source)
        return false;
    // Editor-only helpers (scene camera etc.) are never saved, so never copied.
    return (source->getEntityFlags() & EEntityFlags::HideInEditor) != EEntityFlags::HideInEditor;
}

MSpatialEntity* MEntityDuplicator::duplicate(MSpatialEntity* source)
{
    if (!canDuplicate(source))
        return nullptr;

    auto* sceneManager = MSceneManager::getSceneManagerInstance();
    MScene* scene = sceneManager ? sceneManager->getActiveScene() : nullptr;
    if (!scene)
    {
        MWARN("Duplicate:: no active scene");
        return nullptr;
    }

    // Serialise the subtree exactly as a scene save would...
    pugi::xml_document doc;
    const pugi::xml_node node = source->serialiseEntity(doc);
    if (!node)
    {
        MERROR(SString("Duplicate:: failed to serialise ") + source->getName());
        return nullptr;
    }

    // ...and load it back. deserialiseEntity registers every entity in the
    // subtree with the active scene; the new root lands at the end of the
    // scene root list and is moved into place below.
    MSpatialEntity* clone = MSpatialEntity::deserialiseEntity(node);
    if (!clone)
    {
        MERROR(SString("Duplicate:: failed to rebuild ") + source->getName());
        return nullptr;
    }

    if (MSpatialEntity* parent = source->getParent())
    {
        auto& siblings = parent->getChildren();
        const SString name = makeUniqueSiblingName(source->getName(), siblings);
        parent->insertChildAt(clone, indexOf(siblings, source) + 1);   // updates transforms
        clone->setName(name);
    }
    else
    {
        auto& roots = scene->getRootEntities();
        const SString name = makeUniqueSiblingName(source->getName(), roots);
        scene->insertRootEntityAt(clone, indexOf(roots, source) + 1);
        clone->setName(name);
        clone->updateTransforms();
    }

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
