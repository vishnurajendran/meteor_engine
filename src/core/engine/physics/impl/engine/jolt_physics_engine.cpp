//
// jolt_physics_engine.cpp
//

#include "jolt_physics_engine.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/RegisterTypes.h>
#include "SFML/Graphics/Glsl.hpp"
#include "core/engine/engine_statics.h"
#include "core/engine/physics/impl/bodies/jolt_box_collision.h"
#include "core/engine/physics/impl/bodies/jolt_sphere_collision.h"
#include "core/engine/physics/impl/bodies/jolt_cylinder_collision.h"
#include "core/engine/physics/impl/bodies/jolt_capsule_collision.h"
#include "core/engine/physics/impl/bodies/jolt_mesh_collision.h"
#include "core/engine/physics/impl/bodies/jolt_convexhull_collision.h"

static JPH::Quat shapeAxisRotation(EShapeAxis axis)
{
    switch (axis)
    {
    case EShapeAxis::X: return JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -0.5f * JPH_PI);
    case EShapeAxis::Z: return JPH::Quat::sRotation(JPH::Vec3::sAxisX(),  0.5f * JPH_PI);
    default:            return JPH::Quat::sIdentity();
    }
}

// ---------------------------------------------------------------------------
// Init / cleanup
// ---------------------------------------------------------------------------

void MJoltPhysicsEngine::init()
{
    MLOG("MJoltPhysicsEngine:: Initializing Physics engine");
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    const auto* settings = MEngineStatics::getEngineSettings();

    unsigned int maxTempAllocs         = 10 * 1024 * 1024;
    unsigned int maxPhysicsBodies      = 1024;
    unsigned int numBodyMutexes        = 0;
    unsigned int maxBodyPairs          = 1024;
    unsigned int maxContactConstraints = 1024;
    SVector3 gravity = { 0.0f, -9.8f, 0.0f };

    if (settings)
    {
        maxTempAllocs         = static_cast<unsigned int>(settings->maxPhysicsTempAllocSize.get());
        maxPhysicsBodies      = static_cast<unsigned int>(settings->maxPhysicsBodies.get());
        numBodyMutexes        = static_cast<unsigned int>(settings->numPhysicsBodyMutexes.get());
        maxBodyPairs          = static_cast<unsigned int>(settings->maxPhysicsBodyPairs.get());
        maxContactConstraints = static_cast<unsigned int>(settings->maxPhysicsContactConstraints.get());
        gravity               = settings->gravity.get();
    }

    MLOG(SString::format("Using settings: alloc {0}", maxTempAllocs));

    tempAlloc = new TempAllocatorImpl(maxTempAllocs);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers,
                                         std::thread::hardware_concurrency() - 1);

    physicsSystem.Init(maxPhysicsBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                       broadPhaseLayerInterface, objVsBroadPhaseLayerFilter, objVsObjLayerFilter);

    physicsSystem.SetGravity({ gravity.x, gravity.y, gravity.z });
    physicsSystem.SetBodyActivationListener(&bodyActivationListener);
    physicsSystem.SetContactListener(&contactListener);
    bodyInterface = &physicsSystem.GetBodyInterface();
    contactListener.init(&receiverRegistry, &receiverRegistryMutex);

    // Wire the collision matrix from the physics layers asset to the pair filter.
    // If no asset is loaded, the pair filter falls back to "everything collides".
    auto* physicsLayers = MEngineStatics::getPhysicsLayers();
    if (physicsLayers)
        objVsObjLayerFilter.setCollisionMasks(physicsLayers->getParsedCollisionMasks());

    MLOG("MJoltPhysicsEngine:: Initialized Physics engine");
    allowTick = true;
}

void MJoltPhysicsEngine::cleanup()
{
    MLOG("MJoltPhysicsEngine:: Cleanup");
    releaseAllBodies();
    UnregisterTypes();

    delete Factory::sInstance;
    Factory::sInstance = nullptr;

    delete jobSystem; jobSystem = nullptr;
    delete tempAlloc; tempAlloc = nullptr;
}

void MJoltPhysicsEngine::releaseAllBodies()
{
    MLOG("MJoltPhysicsEngine:: Releasing all bodies");
    for (const auto& bodyId : bodies)
    {
        bodyInterface->RemoveBody(bodyId);
        bodyInterface->DestroyBody(bodyId);
    }
    bodies.clear();
}

void MJoltPhysicsEngine::tick(float fixedDeltaTime)
{
    if (!allowTick) return;
    physicsSystem.Update(fixedDeltaTime, 1, tempAlloc, jobSystem);
    dispatchPendingEvents();
}

void MJoltPhysicsEngine::dispatchPendingEvents()
{
    std::vector<SPendingContactEvent> events;
    contactListener.drainPendingEvents(events);

    for (const auto& e : events)
    {
        if (!e.receiver) continue;
        switch (e.type)
        {
        case SPendingContactEvent::EType::CollisionStart: e.receiver->dispatchCollisionStart(e.collisionData); break;
        case SPendingContactEvent::EType::CollisionStay:  e.receiver->dispatchCollisionStay (e.collisionData); break;
        case SPendingContactEvent::EType::CollisionEnd:   e.receiver->dispatchCollisionEnd  (e.collisionData); break;
        case SPendingContactEvent::EType::TriggerStart:   e.receiver->dispatchTriggerStart  (e.overlapData);   break;
        case SPendingContactEvent::EType::TriggerStay:    e.receiver->dispatchTriggerStay   (e.overlapData);   break;
        case SPendingContactEvent::EType::TriggerEnd:     e.receiver->dispatchTriggerEnd    (e.overlapData);   break;
        }
    }
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

void MJoltPhysicsEngine::registerCallbackReceiver(ICollisionBody* body,
                                                   IPhysicsCallbackReceiver* receiver,
                                                   unsigned int physicsLayer)
{
    if (!body || !receiver) return;

    const auto* joltBody = dynamic_cast<IJoltCollisionBodyInterface*>(body);
    if (!joltBody) return;

    const uint32_t idx  = joltBody->getJoltBodyIndex();
    const BodyID bodyId = joltBody->getBodyID();

    // Both maps updated together under one lock to keep them consistent.
    {
        std::unique_lock lock(receiverRegistryMutex);
        receiverRegistry[idx] = receiver;
        layerRegistry[idx]    = physicsLayer & 31u;
    }

    // Update the Jolt ObjectLayer to encode the physics layer + motion class.
    // Static bodies use ObjectLayers 0-31, dynamic/kinematic use 32-63.
    // This is what MObjectLayerPairFilterImpl::ShouldCollide reads to check
    // the collision matrix.
    const bool isStatic = bodyInterface->GetMotionType(bodyId) == EMotionType::Static;
    const ObjectLayer newLayer = static_cast<ObjectLayer>((physicsLayer & 31u) + (isStatic ? 0u : 32u));
    bodyInterface->SetObjectLayer(bodyId, newLayer);
    bodyInterface->InvalidateContactCache(bodyId);
}

void MJoltPhysicsEngine::unregisterCallbackReceiver(ICollisionBody* body)
{
    if (!body) return;

    const auto* joltBody = dynamic_cast<IJoltCollisionBodyInterface*>(body);
    if (!joltBody) return;

    const uint32_t idx = joltBody->getJoltBodyIndex();

    std::unique_lock lock(receiverRegistryMutex);
    receiverRegistry.erase(idx);
    layerRegistry.erase(idx);
}

// ---------------------------------------------------------------------------
// Raycast
// ---------------------------------------------------------------------------

bool MJoltPhysicsEngine::rayCast(const SRay& ray, const SLayerFilter& layerFilter,
                                  SRayCastHitResult& outHit)
{
    const JPH::RVec3 origin(ray.origin.x, ray.origin.y, ray.origin.z);
    const JPH::Vec3  scaledDir(ray.direction.x * ray.distance,
                               ray.direction.y * ray.distance,
                               ray.direction.z * ray.distance);
    const JPH::RRayCast joltRay{ origin, scaledDir };

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    physicsSystem.GetNarrowPhaseQuery().CastRay(
        joltRay,
        JPH::RayCastSettings{},
        collector
    );

    if (!collector.HadHit())
        return false;

    collector.Sort();

    for (const JPH::RayCastResult& hit : collector.mHits)
    {
        const uint32_t bodyIdx = hit.mBodyID.GetIndex();

        // ---- Layer filter -------------------------------------------------
        unsigned int entityLayer = 0;
        {
            std::shared_lock lock(receiverRegistryMutex);
            auto it = layerRegistry.find(bodyIdx);
            if (it != layerRegistry.end())
                entityLayer = it->second;
        }

        if (!(layerFilter.getLayerMask() & (1u << entityLayer)))
            continue;

        // ---- Hit point ----------------------------------------------------
        const JPH::RVec3 joltHitPoint = joltRay.GetPointOnRay(hit.mFraction);
        outHit.hitPoint  = SVector3(
            static_cast<float>(joltHitPoint.GetX()),
            static_cast<float>(joltHitPoint.GetY()),
            static_cast<float>(joltHitPoint.GetZ())
        );
        outHit.distance = hit.mFraction * ray.distance;

        // ---- Surface normal -----------------------------------------------
        {
            JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), hit.mBodyID);
            if (lock.Succeeded())
            {
                const JPH::Body& body     = lock.GetBody();
                const JPH::Quat  bodyRot  = body.GetRotation();
                const JPH::RVec3 comPos   = body.GetCenterOfMassPosition();

                const JPH::Vec3 hitFromCom(
                    static_cast<float>(joltHitPoint.GetX() - comPos.GetX()),
                    static_cast<float>(joltHitPoint.GetY() - comPos.GetY()),
                    static_cast<float>(joltHitPoint.GetZ() - comPos.GetZ())
                );
                const JPH::Vec3 localHit    = bodyRot.Conjugated() * hitFromCom;
                const JPH::Vec3 localNormal = body.GetShape()->GetSurfaceNormal(hit.mSubShapeID2, localHit);
                const JPH::Vec3 worldNormal = bodyRot * localNormal;

                outHit.hitNormal = SVector3(worldNormal.GetX(), worldNormal.GetY(), worldNormal.GetZ());
            }
            else
            {
                outHit.hitNormal = SVector3(0.0f, 1.0f, 0.0f);
            }
        }

        // ---- Entity lookup ------------------------------------------------
        outHit.hitEntity = nullptr;
        {
            std::shared_lock lock(receiverRegistryMutex);
            auto it = receiverRegistry.find(bodyIdx);
            if (it != receiverRegistry.end())
                outHit.hitEntity = it->second->getEntity();
        }

        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Factory -- Box
// ---------------------------------------------------------------------------

IBoxCollisionBody* MJoltPhysicsEngine::createBoxCollider(const SBoxPhysicsBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating box collider");

    const auto& aabb = settings.bounds;
    const float halfX = (aabb.max.x - aabb.min.x) * 0.5f;
    const float halfY = (aabb.max.y - aabb.min.y) * 0.5f;
    const float halfZ = (aabb.max.z - aabb.min.z) * 0.5f;

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::DEFAULT_STATIC;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DEFAULT_DYNAMIC; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    BoxShapeSettings boxSettings(Vec3(halfX, halfY, halfZ));
    ShapeSettings::ShapeResult shapeResult = boxSettings.Create();
    if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createBoxCollider -- failed to create BoxShape"); return nullptr; }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltBoxCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseBoxCollider(IBoxCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltBoxCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Factory -- Sphere
// ---------------------------------------------------------------------------

ISphereCollisionBody* MJoltPhysicsEngine::createSphereCollisionBody(const SSphereBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating sphere collider");
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::DEFAULT_STATIC;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DEFAULT_DYNAMIC; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    SphereShapeSettings sphereSettings(settings.radius);
    ShapeSettings::ShapeResult shapeResult = sphereSettings.Create();
    if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createSphereCollisionBody -- failed to create SphereShape"); return nullptr; }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltSphereCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseSphereCollisionBody(ISphereCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltSphereCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Factory -- Cylinder
// ---------------------------------------------------------------------------

ICylinderCollisionBody* MJoltPhysicsEngine::createCylinderCollisionBody(const SCylinderBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating cylinder collider");
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::DEFAULT_STATIC;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DEFAULT_DYNAMIC; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    CylinderShapeSettings cylinderSettings(settings.halfHeight, settings.radius);
    ShapeSettings::ShapeResult cylinderResult = cylinderSettings.Create();
    if (!cylinderResult.IsValid()) { MERROR("MJoltPhysicsEngine::createCylinderCollisionBody -- failed to create CylinderShape"); return nullptr; }

    ShapeSettings::ShapeResult shapeResult;
    if (settings.axis == EShapeAxis::Y) { shapeResult = cylinderResult; }
    else
    {
        RotatedTranslatedShapeSettings rts(Vec3::sZero(), shapeAxisRotation(settings.axis), cylinderResult.Get());
        shapeResult = rts.Create();
        if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createCylinderCollisionBody -- failed to create rotated CylinderShape"); return nullptr; }
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltCylinderCollision(bodyId, *bodyInterface, physicsSystem,
                                          settings.halfHeight, settings.radius, settings.axis);

    return nullptr;
}

void MJoltPhysicsEngine::releaseCylinderCollisionBody(ICylinderCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltCylinderCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Factory -- Capsule
// ---------------------------------------------------------------------------

ICapsuleCollisionBody* MJoltPhysicsEngine::createCapsuleCollisionBody(const SCapsuleBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating capsule collider");
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::DEFAULT_STATIC;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DEFAULT_DYNAMIC; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    CapsuleShapeSettings capsuleSettings(settings.halfHeight, settings.radius);
    ShapeSettings::ShapeResult capsuleResult = capsuleSettings.Create();
    if (!capsuleResult.IsValid()) { MERROR("MJoltPhysicsEngine::createCapsuleCollisionBody -- failed to create CapsuleShape"); return nullptr; }

    ShapeSettings::ShapeResult shapeResult;
    if (settings.axis == EShapeAxis::Y) { shapeResult = capsuleResult; }
    else
    {
        RotatedTranslatedShapeSettings rts(Vec3::sZero(), shapeAxisRotation(settings.axis), capsuleResult.Get());
        shapeResult = rts.Create();
        if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createCapsuleCollisionBody -- failed to create rotated CapsuleShape"); return nullptr; }
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltCapsuleCollision(bodyId, *bodyInterface, physicsSystem,
                                         settings.halfHeight, settings.radius, settings.axis);

    return nullptr;
}

void MJoltPhysicsEngine::releaseCapsuleCollisionBody(ICapsuleCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltCapsuleCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Factory -- Mesh
// ---------------------------------------------------------------------------

IMeshCollisionBody* MJoltPhysicsEngine::createMeshCollisionBody(const SMeshBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating mesh collider");

    if (settings.vertices.empty() || settings.indices.empty() || settings.indices.size() % 3 != 0)
    { MERROR("MJoltPhysicsEngine::createMeshCollisionBody -- invalid mesh data"); return nullptr; }

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    if (settings.bodyType == ECollisionBodyType::DynamicBody)
        MWARN("MJoltPhysicsEngine::createMeshCollisionBody -- MeshShape does not support Dynamic bodies; forced to Static");
    else if (settings.bodyType == ECollisionBodyType::KinematicBody)
    { motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    JPH::TriangleList triangles;
    triangles.reserve(settings.indices.size() / 3);
    for (size_t i = 0; i + 2 < settings.indices.size(); i += 3)
    {
        const SVector3& v0 = settings.vertices[settings.indices[i]];
        const SVector3& v1 = settings.vertices[settings.indices[i + 1]];
        const SVector3& v2 = settings.vertices[settings.indices[i + 2]];
        triangles.emplace_back(JPH::Float3(v0.x, v0.y, v0.z), JPH::Float3(v1.x, v1.y, v1.z), JPH::Float3(v2.x, v2.y, v2.z));
    }

    JPH::MeshShapeSettings meshSettings(std::move(triangles));
    ShapeSettings::ShapeResult shapeResult = meshSettings.Create();
    if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createMeshCollisionBody -- failed to create MeshShape"); return nullptr; }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltMeshCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseMeshCollisionBody(IMeshCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltMeshCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Factory -- Convex Hull
// ---------------------------------------------------------------------------

IConvexHullCollisionBody* MJoltPhysicsEngine::createConvexHullCollisionBody(const SConvexHullBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating convex hull collider");

    if (settings.points.size() < 4)
    { MERROR("MJoltPhysicsEngine::createConvexHullCollisionBody -- need at least 4 points"); return nullptr; }

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::DEFAULT_STATIC;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::DEFAULT_STATIC;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DEFAULT_DYNAMIC; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DEFAULT_DYNAMIC; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    JPH::Array<JPH::Vec3> jphPoints;
    jphPoints.reserve(settings.points.size());
    for (const SVector3& p : settings.points)
        jphPoints.emplace_back(p.x, p.y, p.z);

    JPH::ConvexHullShapeSettings hullSettings(jphPoints);
    ShapeSettings::ShapeResult shapeResult = hullSettings.Create();
    if (!shapeResult.IsValid()) { MERROR("MJoltPhysicsEngine::createConvexHullCollisionBody -- failed to create ConvexHullShape"); return nullptr; }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltConvexHullCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseConvexHullCollisionBody(IConvexHullCollisionBody* body)
{
    if (!body || !bodyInterface) return;
    const auto* joltBody = dynamic_cast<MJoltConvexHullCollision*>(body);
    if (!joltBody) return;
    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);
    delete joltBody;
}

// ---------------------------------------------------------------------------
// Shared helper
// ---------------------------------------------------------------------------

bool MJoltPhysicsEngine::createPhysicsBody(const BodyCreationSettings& bodyCreationSetting,
                                             const EActivation activation, BodyID& container)
{
    if (!bodyInterface) return false;
    container = bodyInterface->CreateAndAddBody(bodyCreationSetting, activation);
    if (container.IsInvalid()) return false;
    bodies.insert(container);
    return true;
}