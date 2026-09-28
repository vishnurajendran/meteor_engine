//
// entity_duplicator.h
//
// Duplicates a spatial entity (and its whole subtree) by round-tripping it
// through the scene XML format: serialiseEntity() into an in-memory document,
// then deserialiseEntity() to build the copy. Anything that survives a scene
// save/load survives a duplicate, and new entity types need no extra code.
//
// The copy is inserted directly after the original under the same parent (or
// in the scene root), gets a unique sibling name ("Cube" -> "Cube (1)") and is
// registered with the active scene like any loaded entity, so onCreate runs
// immediately and onStart on the next scene update.
//
// Note: the copy reflects the entity's current in-memory state. Duplicating
// during play mode copies runtime values, and the copy is discarded with the
// rest of the play session.
//

#ifndef ENTITY_DUPLICATOR_H
#define ENTITY_DUPLICATOR_H

class MSpatialEntity;
class MObject;

class MEntityDuplicator
{
public:
    // Returns the new entity, or nullptr if `source` can't be duplicated
    // (null, hidden editor-only entity, no active scene, or deserialisation
    // failed).
    static MSpatialEntity* duplicate(MSpatialEntity* source);

    // Duplicates `object` if it's a spatial entity and selects the copy.
    // Returns the copy (or nullptr).
    static MSpatialEntity* duplicateAndSelect(MObject* object);

    [[nodiscard]] static bool canDuplicate(const MSpatialEntity* source);
};

#endif // ENTITY_DUPLICATOR_H
