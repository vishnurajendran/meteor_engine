//
// Created by ssj5v on 21-05-2026.
//

#ifndef CONTACT_LISTENER_H
#define CONTACT_LISTENER_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include "core/engine/physics/callback/physics_callback_reciever.h"
#include "core/engine/physics/data/collision_data.h"

// A single contact event queued during physicsSystem.Update() and dispatched
// on the main thread by MJoltPhysicsEngine::tick() after Update() returns.
struct SPendingContactEvent
{
    enum class EType
    {
        CollisionStart, CollisionStay, CollisionEnd,
        TriggerStart,   TriggerStay,   TriggerEnd
    };

    EType                     type;
    IPhysicsCallbackReceiver* receiver = nullptr;
    SCollisionData            collisionData;   // valid for Collision* events
    SOverlapData              overlapData;     // valid for Trigger* events
};

class MContactListener : public JPH::ContactListener
{
public:
    // Called by MJoltPhysicsEngine::init() once the registry is live.
    // Raw pointers are safe here — the engine owns the registry and outlives the listener.
    void init(const std::unordered_map<uint32_t, IPhysicsCallbackReceiver*>* reg,
              std::shared_mutex* regMutex)
    {
        receiverRegistry      = reg;
        receiverRegistryMutex = regMutex;
    }

    // -----------------------------------------------------------------------
    // JPH::ContactListener overrides — called from Jolt worker threads
    // -----------------------------------------------------------------------

    JPH::ValidateResult OnContactValidate(const JPH::Body&, const JPH::Body&,
                                          JPH::RVec3Arg,
                                          const JPH::CollideShapeResult&) override
    {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2,
                        const JPH::ContactManifold& inManifold,
                        JPH::ContactSettings&) override
    {
        const bool isTrigger = inBody1.IsSensor() || inBody2.IsSensor();

        if (isTrigger)
        {
            // Track this pair so OnContactRemoved knows to fire TriggerEnd.
            // TRADEOFF — keyed by body index pair, not SubShapeIDPair. For compound
            // shapes with multiple sub-shapes the last add wins; upgrade the key to
            // SubShapeIDPair.GetHash() if compound shape support is needed.
            const uint64_t key = makePairKey(inBody1.GetID(), inBody2.GetID());
            {
                std::lock_guard lock(stateMutex);
                activeTriggerPairs.insert(key);
            }
            queueTriggerContact(inBody1, inBody2, inManifold,
                                SPendingContactEvent::EType::TriggerStart);
        }
        else
        {
            queueCollisionContact(inBody1, inBody2, inManifold,
                                  SPendingContactEvent::EType::CollisionStart);
        }
    }

    void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2,
                            const JPH::ContactManifold& inManifold,
                            JPH::ContactSettings&) override
    {
        const bool isTrigger = inBody1.IsSensor() || inBody2.IsSensor();

        if (isTrigger)
            queueTriggerContact(inBody1, inBody2, inManifold,
                                SPendingContactEvent::EType::TriggerStay);
        else
            queueCollisionContact(inBody1, inBody2, inManifold,
                                  SPendingContactEvent::EType::CollisionStay);
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        // No Body references or manifold available here — only IDs.
        const uint32_t key1 = inSubShapePair.GetBody1ID().GetIndex();
        const uint32_t key2 = inSubShapePair.GetBody2ID().GetIndex();

        IPhysicsCallbackReceiver* recv1 = lookupReceiver(key1);
        IPhysicsCallbackReceiver* recv2 = lookupReceiver(key2);

        if (!recv1 && !recv2) return;

        const uint64_t pairKey = makePairKey(inSubShapePair.GetBody1ID(),
                                             inSubShapePair.GetBody2ID());

        bool wasTrigger = false;
        {
            std::lock_guard lock(stateMutex);
            wasTrigger = activeTriggerPairs.erase(pairKey) > 0;
        }

        if (wasTrigger)
        {
            if (recv1)
            {
                SOverlapData d;
                d.otherEntity = recv2 ? recv2->getEntity() : nullptr;
                pushEvent({ SPendingContactEvent::EType::TriggerEnd, recv1, {}, d });
            }
            if (recv2)
            {
                SOverlapData d;
                d.otherEntity = recv1 ? recv1->getEntity() : nullptr;
                pushEvent({ SPendingContactEvent::EType::TriggerEnd, recv2, {}, d });
            }
        }
        else
        {
            if (recv1)
            {
                SCollisionData d;
                d.otherEntity = recv2 ? recv2->getEntity() : nullptr;
                pushEvent({ SPendingContactEvent::EType::CollisionEnd, recv1, d, {} });
            }
            if (recv2)
            {
                SCollisionData d;
                d.otherEntity = recv1 ? recv1->getEntity() : nullptr;
                pushEvent({ SPendingContactEvent::EType::CollisionEnd, recv2, d, {} });
            }
        }
    }

    // -----------------------------------------------------------------------
    // Called by MJoltPhysicsEngine::tick() on the main thread after Update()
    // -----------------------------------------------------------------------

    void drainPendingEvents(std::vector<SPendingContactEvent>& out)
    {
        std::lock_guard lock(stateMutex);
        out = std::move(pendingEvents);
        pendingEvents.clear();
    }

private:
    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    IPhysicsCallbackReceiver* lookupReceiver(uint32_t bodyIndex) const
    {
        if (!receiverRegistry) return nullptr;
        std::shared_lock lock(*receiverRegistryMutex);
        const auto it = receiverRegistry->find(bodyIndex);
        return (it != receiverRegistry->end()) ? it->second : nullptr;
    }

    // Order-independent key for a body pair — prevents duplicate entries for
    // (A,B) vs (B,A).
    static uint64_t makePairKey(JPH::BodyID id1, JPH::BodyID id2)
    {
        uint32_t a = id1.GetIndex();
        uint32_t b = id2.GetIndex();
        if (a > b) std::swap(a, b);
        return (static_cast<uint64_t>(a) << 32) | b;
    }

    void queueCollisionContact(const JPH::Body& body1, const JPH::Body& body2,
                               const JPH::ContactManifold& manifold,
                               SPendingContactEvent::EType type)
    {
        IPhysicsCallbackReceiver* recv1 = lookupReceiver(body1.GetID().GetIndex());
        IPhysicsCallbackReceiver* recv2 = lookupReceiver(body2.GetID().GetIndex());

        if (!recv1 && !recv2) return;

        // mWorldSpaceNormal points from body2 toward body1 in Jolt.
        const JPH::Vec3 n = manifold.mWorldSpaceNormal;

        const bool hasContacts = manifold.mRelativeContactPointsOn1.size() > 0;

        if (recv1)
        {
            SCollisionData d;
            d.otherEntity  = recv2 ? recv2->getEntity() : nullptr;
            d.contactNormal = { n.GetX(), n.GetY(), n.GetZ() };
            d.penetration   = manifold.mPenetrationDepth;
            if (hasContacts)
            {
                JPH::Vec3 cp = manifold.GetWorldSpaceContactPointOn1(0);
                d.contactPoint = { cp.GetX(), cp.GetY(), cp.GetZ() };
            }
            pushEvent({ type, recv1, d, {} });
        }

        if (recv2)
        {
            SCollisionData d;
            d.otherEntity   = recv1 ? recv1->getEntity() : nullptr;
            d.contactNormal = { -n.GetX(), -n.GetY(), -n.GetZ() }; // flipped for body2
            d.penetration   = manifold.mPenetrationDepth;
            if (hasContacts)
            {
                JPH::Vec3 cp = manifold.GetWorldSpaceContactPointOn2(0);
                d.contactPoint = { cp.GetX(), cp.GetY(), cp.GetZ() };
            }
            pushEvent({ type, recv2, d, {} });
        }
    }

    void queueTriggerContact(const JPH::Body& body1, const JPH::Body& body2,
                             const JPH::ContactManifold& manifold,
                             SPendingContactEvent::EType type)
    {
        IPhysicsCallbackReceiver* recv1 = lookupReceiver(body1.GetID().GetIndex());
        IPhysicsCallbackReceiver* recv2 = lookupReceiver(body2.GetID().GetIndex());

        if (!recv1 && !recv2) return;

        const bool hasContacts = manifold.mRelativeContactPointsOn1.size() > 0;

        if (recv1)
        {
            SOverlapData d;
            d.otherEntity = recv2 ? recv2->getEntity() : nullptr;
            if (hasContacts)
            {
                JPH::Vec3 cp = manifold.GetWorldSpaceContactPointOn1(0);
                d.overlapPoint = { cp.GetX(), cp.GetY(), cp.GetZ() };
            }
            pushEvent({ type, recv1, {}, d });
        }

        if (recv2)
        {
            SOverlapData d;
            d.otherEntity = recv1 ? recv1->getEntity() : nullptr;
            if (hasContacts)
            {
                JPH::Vec3 cp = manifold.GetWorldSpaceContactPointOn2(0);
                d.overlapPoint = { cp.GetX(), cp.GetY(), cp.GetZ() };
            }
            pushEvent({ type, recv2, {}, d });
        }
    }

    void pushEvent(SPendingContactEvent event)
    {
        std::lock_guard lock(stateMutex);
        pendingEvents.push_back(std::move(event));
    }

private:
    // Provided by the engine after construction — not owned here.
    const std::unordered_map<uint32_t, IPhysicsCallbackReceiver*>* receiverRegistry      = nullptr;
    std::shared_mutex*                                              receiverRegistryMutex = nullptr;

    // Single mutex covers both the event queue and the trigger pair set.
    // Separating them would require a lock-ordering discipline to avoid deadlocks.
    std::mutex stateMutex;

    std::vector<SPendingContactEvent> pendingEvents;   // drained each tick
    std::unordered_set<uint64_t>      activeTriggerPairs; // body-pair keys for in-flight triggers
};

#endif //CONTACT_LISTENER_H