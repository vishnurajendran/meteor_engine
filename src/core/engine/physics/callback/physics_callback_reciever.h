//
// Created by ssj5v on 22-05-2026.
//

#ifndef PHYSICS_CALLBACK_RECEIVER_H
#define PHYSICS_CALLBACK_RECEIVER_H
#include "core/engine/physics/data/collision_data.h"
class MSpatialEntity;

// Implemented by any entity that wants physics contact callbacks.
// The contact listener dispatches to registered receivers on the main thread
// after physicsSystem.Update() completes — callbacks are NOT called mid-solve.
class IPhysicsCallbackReceiver
{
public:
    virtual ~IPhysicsCallbackReceiver() = default;

    // Returns the spatial entity this receiver belongs to. Used to populate
    // SCollisionData::otherEntity / SOverlapData::otherEntity on the remote side.
    // Spatial physics entities return 'this'; returns nullptr by default.
    [[nodiscard]] virtual MSpatialEntity* getEntity() { return nullptr; }

    virtual void dispatchCollisionStart(const SCollisionData& data) {}
    virtual void dispatchCollisionStay (const SCollisionData& data) {}
    virtual void dispatchCollisionEnd  (const SCollisionData& data) {}
    virtual void dispatchTriggerStart  (const SOverlapData&   data) {}
    virtual void dispatchTriggerStay   (const SOverlapData&   data) {}
    virtual void dispatchTriggerEnd    (const SOverlapData&   data) {}
};

#endif //PHYSICS_CALLBACK_RECEIVER_H