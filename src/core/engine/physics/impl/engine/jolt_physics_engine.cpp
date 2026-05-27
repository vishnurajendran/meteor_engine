//
// jolt_physics_engine.cpp
//

#include "jolt_physics_engine.h"
#include <Jolt/Jolt.h>
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/RegisterTypes.h"
#include "SFML/Graphics/Glsl.hpp"
#include "core/engine/engine_statics.h"
#include "core/engine/physics/impl/bodies/jolt_box_collision.h"
#include "core/engine/physics/impl/bodies/jolt_capsule_collision.h"
#include "core/engine/physics/impl/bodies/jolt_convexhull_collision.h"
#include "core/engine/physics/impl/bodies/jolt_cylinder_collision.h"
#include "core/engine/physics/impl/bodies/jolt_mesh_collision.h"
#include "core/engine/physics/impl/bodies/jolt_sphere_collision.h"

// Helper — returns the quaternion that rotates Jolt's Y-axis cylinder/capsule
// to align with the requested axis. Matches the logic in the Jolt impl classes.
static JPH::Quat shapeAxisRotation(EShapeAxis axis)
{
    switch (axis)
    {
    case EShapeAxis::X: return JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -0.5f * JPH_PI);
    case EShapeAxis::Z: return JPH::Quat::sRotation(JPH::Vec3::sAxisX(),  0.5f * JPH_PI);
    default:            return JPH::Quat::sIdentity();
    }
}

void MJoltPhysicsEngine::init()
{
    MLOG("MJoltPhysicsEngine:: Initializing Physics engine");
    RegisterDefaultAllocator();
    Factory::sInstance = new Factory();
    RegisterTypes();

    const auto* settings = MEngineStatics::getEngineSettings();

    uint maxTempAllocs = 10 * 1024 * 1024;
    uint maxPhysicsBodies = 1024;
    uint numBodyMutexes = 0;
    uint maxBodyPairs = 1024;
    uint maxContactConstraints = 1024;
    SVector3 gravity= {0.0f,-9.8f, 0.0f};

    if (settings)
    {
        maxTempAllocs = static_cast<uint>(settings->maxPhysicsTempAllocSize.get());
        maxPhysicsBodies = static_cast<uint>(settings->maxPhysicsBodies.get());
        numBodyMutexes = static_cast<uint>(settings->numPhysicsBodyMutexes.get());
        maxBodyPairs = static_cast<uint>(settings->maxPhysicsBodyPairs.get());
        maxContactConstraints = static_cast<uint>(settings->maxPhysicsContactConstraints.get());
        gravity = settings->gravity.get();
    }

    MLOG(SString::format("Using settings: alloc {0}", maxTempAllocs));

    tempAlloc = new TempAllocatorImpl(maxTempAllocs);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    physicsSystem.Init(maxPhysicsBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                       broadPhaseLayerInterface, objVsBroadPhaseLayerFilter, objVsObjLayerFilter);

    physicsSystem.SetGravity({gravity.x, gravity.y, gravity.z});
    physicsSystem.SetBodyActivationListener(&bodyActivationListener);
    physicsSystem.SetContactListener(&contactListener);
    bodyInterface = &physicsSystem.GetBodyInterface();
    contactListener.init(&receiverRegistry, &receiverRegistryMutex);
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

    delete jobSystem;  jobSystem  = nullptr;
    delete tempAlloc;  tempAlloc  = nullptr;
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
    if (!allowTick)
        return;

    physicsSystem.Update(fixedDeltaTime, 1, tempAlloc, jobSystem);
    // Contact callbacks were queued on worker threads during Update() — dispatch
    // them now on the main thread so entity callbacks always run here.
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
// Box
// ---------------------------------------------------------------------------

IBoxCollisionBody* MJoltPhysicsEngine::createBoxCollider(const SBoxPhysicsBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating box collider");

    const auto& aabb = settings.bounds;
    const float halfX = (aabb.max.x - aabb.min.x) * 0.5f;
    const float halfY = (aabb.max.y - aabb.min.y) * 0.5f;
    const float halfZ = (aabb.max.z - aabb.min.z) * 0.5f;

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    BoxShapeSettings boxShapeSettings(Vec3(halfX, halfY, halfZ));
    ShapeSettings::ShapeResult shapeResult = boxShapeSettings.Create();
    if (!shapeResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createBoxCollider - failed to create BoxShape");
        return nullptr;
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltBoxCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseBoxCollider(IBoxCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing box collider");
    const auto* joltBody = dynamic_cast<MJoltBoxCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Sphere
// ---------------------------------------------------------------------------

ISphereCollisionBody* MJoltPhysicsEngine::createSphereCollisionBody(const SSphereBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating sphere collider");
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    SphereShapeSettings sphereShapeSettings(settings.radius);
    ShapeSettings::ShapeResult shapeResult = sphereShapeSettings.Create();
    if (!shapeResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createSphereCollisionBody — failed to create SphereShape");
        return nullptr;
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltSphereCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseSphereCollisionBody(ISphereCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing sphere collider");
    const auto* joltBody = dynamic_cast<MJoltSphereCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Cylinder
// ---------------------------------------------------------------------------

ICylinderCollisionBody* MJoltPhysicsEngine::createCylinderCollisionBody(const SCylinderBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating cylinder collider");

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    CylinderShapeSettings cylinderSettings(settings.halfHeight, settings.radius);
    ShapeSettings::ShapeResult cylinderResult = cylinderSettings.Create();
    if (!cylinderResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createCylinderCollisionBody — failed to create CylinderShape");
        return nullptr;
    }

    ShapeSettings::ShapeResult shapeResult;
    if (settings.axis == EShapeAxis::Y)
    {
        shapeResult = cylinderResult;
    }
    else
    {
        RotatedTranslatedShapeSettings rts(Vec3::sZero(), shapeAxisRotation(settings.axis), cylinderResult.Get());
        shapeResult = rts.Create();
        if (!shapeResult.IsValid())
        {
            MERROR("MJoltPhysicsEngine::createCylinderCollisionBody — failed to create rotated CylinderShape");
            return nullptr;
        }
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

    MLOG("MJoltPhysicsEngine:: Releasing cylinder collider");
    const auto* joltBody = dynamic_cast<MJoltCylinderCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Capsule
// ---------------------------------------------------------------------------

ICapsuleCollisionBody* MJoltPhysicsEngine::createCapsuleCollisionBody(const SCapsuleBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating capsule collider");

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    CapsuleShapeSettings capsuleSettings(settings.halfHeight, settings.radius);
    ShapeSettings::ShapeResult capsuleResult = capsuleSettings.Create();
    if (!capsuleResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createCapsuleCollisionBody — failed to create CapsuleShape");
        return nullptr;
    }

    ShapeSettings::ShapeResult shapeResult;
    if (settings.axis == EShapeAxis::Y)
    {
        shapeResult = capsuleResult;
    }
    else
    {
        RotatedTranslatedShapeSettings rts(Vec3::sZero(), shapeAxisRotation(settings.axis), capsuleResult.Get());
        shapeResult = rts.Create();
        if (!shapeResult.IsValid())
        {
            MERROR("MJoltPhysicsEngine::createCapsuleCollisionBody — failed to create rotated CapsuleShape");
            return nullptr;
        }
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

    MLOG("MJoltPhysicsEngine:: Releasing capsule collider");
    const auto* joltBody = dynamic_cast<MJoltCapsuleCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Mesh
// ---------------------------------------------------------------------------

IMeshCollisionBody* MJoltPhysicsEngine::createMeshCollisionBody(const SMeshBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating mesh collider");

    if (settings.vertices.empty() || settings.indices.empty() || settings.indices.size() % 3 != 0)
    {
        MERROR("MJoltPhysicsEngine::createMeshCollisionBody — invalid mesh data");
        return nullptr;
    }

    // MeshShape does not support Dynamic motion in Jolt — force to Static.
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    if (settings.bodyType == ECollisionBodyType::DynamicBody)
    {
        MWARN("MJoltPhysicsEngine::createMeshCollisionBody — MeshShape does not support Dynamic bodies; forced to Static");
    }
    else if (settings.bodyType == ECollisionBodyType::KinematicBody)
    {
        motionType  = EMotionType::Kinematic;
        objectLayer = Layers::DYNAMIC_BODY;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    // Build unindexed triangle list — Jolt builds a BVH from this at creation time.
    JPH::TriangleList triangles;
    triangles.reserve(settings.indices.size() / 3);

    for (size_t i = 0; i + 2 < settings.indices.size(); i += 3)
    {
        const SVector3& v0 = settings.vertices[settings.indices[i]];
        const SVector3& v1 = settings.vertices[settings.indices[i + 1]];
        const SVector3& v2 = settings.vertices[settings.indices[i + 2]];
        triangles.emplace_back(
            JPH::Float3(v0.x, v0.y, v0.z),
            JPH::Float3(v1.x, v1.y, v1.z),
            JPH::Float3(v2.x, v2.y, v2.z)
        );
    }

    JPH::MeshShapeSettings meshShapeSettings(std::move(triangles));
    ShapeSettings::ShapeResult shapeResult = meshShapeSettings.Create();
    if (!shapeResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createMeshCollisionBody — failed to create MeshShape");
        return nullptr;
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltMeshCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseMeshCollisionBody(IMeshCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing mesh collider");
    const auto* joltBody = dynamic_cast<MJoltMeshCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Convex hull
// ---------------------------------------------------------------------------

IConvexHullCollisionBody* MJoltPhysicsEngine::createConvexHullCollisionBody(const SConvexHullBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating convex hull collider");

    if (settings.points.size() < 4)
    {
        MERROR("MJoltPhysicsEngine::createConvexHullCollisionBody — need at least 4 points to form a convex hull");
        return nullptr;
    }

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    JPH::Array<JPH::Vec3> jphPoints;
    jphPoints.reserve(settings.points.size());
    for (const SVector3& p : settings.points)
        jphPoints.emplace_back(p.x, p.y, p.z);

    JPH::ConvexHullShapeSettings hullSettings(jphPoints);
    ShapeSettings::ShapeResult shapeResult = hullSettings.Create();
    if (!shapeResult.IsValid())
    {
        MERROR("MJoltPhysicsEngine::createConvexHullCollisionBody — failed to create ConvexHullShape");
        return nullptr;
    }

    BodyCreationSettings bodySettings(shapeResult.Get(), position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltConvexHullCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseConvexHullCollisionBody(IConvexHullCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing convex hull collider");
    const auto* joltBody = dynamic_cast<MJoltConvexHullCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

void MJoltPhysicsEngine::registerCallbackReceiver(ICollisionBody* body,
                                                  IPhysicsCallbackReceiver* receiver)
{
    if (!body || !receiver) return;

    const auto* joltBody = dynamic_cast<IJoltCollisionBodyInterface*>(body);
    if (!joltBody) return;

    std::unique_lock lock(receiverRegistryMutex);
    receiverRegistry[joltBody->getJoltBodyIndex()] = receiver;
}

void MJoltPhysicsEngine::unregisterCallbackReceiver(ICollisionBody* body)
{
    if (!body) return;

    const auto* joltBody = dynamic_cast<IJoltCollisionBodyInterface*>(body);
    if (!joltBody) return;

    std::unique_lock lock(receiverRegistryMutex);
    receiverRegistry.erase(joltBody->getJoltBodyIndex());
}

bool MJoltPhysicsEngine::createPhysicsBody(const BodyCreationSettings& bodyCreationSetting,
                                           const EActivation activation, BodyID& container)
{
    if (!bodyInterface) return false;

    container = bodyInterface->CreateAndAddBody(bodyCreationSetting, activation);

    if (container.IsInvalid())
        return false;

    bodies.insert(container);
    return true;
}