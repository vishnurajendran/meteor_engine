//
// sphere_collision_body_entity.cpp
//

#include "sphere_collision_body_entity.h"

#include "core/engine/gizmos/gizmos.h"

IMPLEMENT_SPATIAL_CLASS(MSphereCollisionBody)

MSphereCollisionBody* MSphereCollisionBody::create(ECollisionBodyType type, float r, float m)
{
    auto* e = new MSphereCollisionBody();
    e->bodyType.set(type); e->radius.set(r); e->mass.set(m);
    return e;
}
MSphereCollisionBody* MSphereCollisionBody::createDynamic(float r, float m)
{
    auto* e = create(ECollisionBodyType::DynamicBody, r, m);
    e->affectedByGravity.set(true);
    return e;
}
MSphereCollisionBody* MSphereCollisionBody::createStatic (float r) { return create(ECollisionBodyType::StaticBody,  r, 0.0f); }
MSphereCollisionBody* MSphereCollisionBody::createTrigger(float r)
{
    auto* e = create(ECollisionBodyType::StaticBody, r, 0.0f);
    e->isSensor.set(true);
    return e;
}

void MSphereCollisionBody::createCollisionBody()
{
    const auto& scale = getWorldScale();
    SSphereBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.radius            = radius.get() * std::max({ scale.x, scale.y, scale.z });

    physicsBody = physicsEngine->createSphereCollisionBody(settings);
    if (!physicsBody)
        MERROR("MSphereCollisionBody::createCollisionBody — failed to create sphere collider");
}

void MSphereCollisionBody::releaseBody()
{
    if (!physicsBody) return;
    physicsEngine->releaseSphereCollisionBody(physicsBody);
    physicsBody = nullptr;
}

void MSphereCollisionBody::setupShapeCallbacks()
{
    radius.setOnChangeCallback([this](auto) { syncRadius(); });
}

void MSphereCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody) syncRadius();
}

void MSphereCollisionBody::syncRadius()
{
    if (!physicsBody) return;
    const auto& scale = getRelativeScale();
    physicsBody->setRadius(radius.get() * std::max({ scale.x, scale.y, scale.z }));
}

void MSphereCollisionBody::onDrawGizmo(SVector2)
{
    const auto& scale = getRelativeScale();
    const SColor color = isSensor.get() ? SColor(0.2f, 0.8f, 1.0f, 1.0f) : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    MGizmos::drawWireSphere(getWorldPosition(),
        radius.get() * std::max({ scale.x, scale.y, scale.z }), color, 1.0f, getWorldRotation());
}