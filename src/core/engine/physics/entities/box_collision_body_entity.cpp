//
// box_collision_body_entity.cpp
//

#include "box_collision_body_entity.h"

#include "core/engine/gizmos/gizmos.h"

IMPLEMENT_SPATIAL_CLASS(MBoxCollisionBody)

MBoxCollisionBody* MBoxCollisionBody::create(ECollisionBodyType type, SVector3 halfExtents, float mass)
{
    auto* e = new MBoxCollisionBody();
    e->bodyType.set(type);
    e->halfExtentX.set(halfExtents.x);
    e->halfExtentY.set(halfExtents.y);
    e->halfExtentZ.set(halfExtents.z);
    e->mass.set(mass);
    return e;
}

MBoxCollisionBody* MBoxCollisionBody::createDynamic(SVector3 halfExtents, float mass)
{
    auto* e = create(ECollisionBodyType::DynamicBody, halfExtents, mass);
    e->affectedByGravity.set(true);
    return e;
}

MBoxCollisionBody* MBoxCollisionBody::createStatic(SVector3 halfExtents)
{
    return create(ECollisionBodyType::StaticBody, halfExtents, 0.0f);
}

MBoxCollisionBody* MBoxCollisionBody::createTrigger(SVector3 halfExtents)
{
    auto* e = create(ECollisionBodyType::StaticBody, halfExtents, 0.0f);
    e->isSensor.set(true);
    return e;
}

void MBoxCollisionBody::createCollisionBody()
{
    updateTransforms();
    const auto& scale = getWorldScale();

    SBoxPhysicsBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.bounds = {
        .min = { -halfExtentX.get() * scale.x, -halfExtentY.get() * scale.y, -halfExtentZ.get() * scale.z },
        .max = {  halfExtentX.get() * scale.x,  halfExtentY.get() * scale.y,  halfExtentZ.get() * scale.z }
    };

    physicsBody = physicsEngine->createBoxCollider(settings);
    if (!physicsBody)
        MERROR("MBoxCollisionBody::createCollisionBody — failed to create box collider");
}

void MBoxCollisionBody::releaseBody()
{
    if (!physicsBody) return;
    physicsEngine->releaseBoxCollider(physicsBody);
    physicsBody = nullptr;
}

void MBoxCollisionBody::setupShapeCallbacks()
{
    halfExtentX.setOnChangeCallback([this](auto) { syncBounds(); });
    halfExtentY.setOnChangeCallback([this](auto) { syncBounds(); });
    halfExtentZ.setOnChangeCallback([this](auto) { syncBounds(); });
}

void MBoxCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody) syncBounds();
}

void MBoxCollisionBody::syncBounds()
{
    if (!physicsBody) return;
    const auto& scale = getRelativeScale();
    physicsBody->setBounds({
        .min = { -halfExtentX.get() * scale.x, -halfExtentY.get() * scale.y, -halfExtentZ.get() * scale.z },
        .max = {  halfExtentX.get() * scale.x,  halfExtentY.get() * scale.y,  halfExtentZ.get() * scale.z }
    });
}

void MBoxCollisionBody::onDrawGizmo(SVector2)
{
    const auto& scale = getRelativeScale();
    const SColor color = isSensor.get() ? SColor(0.2f, 0.8f, 1.0f, 1.0f) : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    MGizmos::drawWireCube(getWorldPosition(),
        { halfExtentX.get() * scale.x, halfExtentY.get() * scale.y, halfExtentZ.get() * scale.z },
        color, 1.0f, getWorldRotation());
}