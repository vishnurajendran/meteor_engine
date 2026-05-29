//
// cylinder_collision_body_entity.cpp
//

#include "cylinder_collision_body_entity.h"

#include <algorithm>
#include "core/engine/gizmos/gizmos.h"

IMPLEMENT_SPATIAL_CLASS(MCylinderCollisionBody)

MCylinderCollisionBody* MCylinderCollisionBody::create(ECollisionBodyType type, float hh, float r, float m)
{
    auto* e = new MCylinderCollisionBody();
    e->bodyType.set(type); e->halfHeight.set(hh); e->radius.set(r); e->mass.set(m);
    return e;
}
MCylinderCollisionBody* MCylinderCollisionBody::createDynamic(float hh, float r, float m)
{
    auto* e = create(ECollisionBodyType::DynamicBody, hh, r, m);
    e->affectedByGravity.set(true);
    return e;
}
MCylinderCollisionBody* MCylinderCollisionBody::createStatic(float hh, float r)
{
    return create(ECollisionBodyType::StaticBody, hh, r, 0.0f);
}

void MCylinderCollisionBody::axisScaleFactors(float& outScaleR, float& outScaleH, const SVector3& s) const
{
    switch (axis.get())
    {
    case EShapeAxis::X: outScaleR = std::max(s.y, s.z); outScaleH = s.x; break;
    case EShapeAxis::Z: outScaleR = std::max(s.x, s.y); outScaleH = s.z; break;
    default:            outScaleR = std::max(s.x, s.z); outScaleH = s.y; break;
    }
}

void MCylinderCollisionBody::createCollisionBody()
{
    updateTransforms();
    float scaleR, scaleH;
    axisScaleFactors(scaleR, scaleH, getWorldScale());

    SCylinderBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.halfHeight        = halfHeight.get() * scaleH;
    settings.radius            = radius.get()     * scaleR;
    settings.axis              = axis.get();

    physicsBody = physicsEngine->createCylinderCollisionBody(settings);
    if (!physicsBody)
        MERROR("MCylinderCollisionBody::createCollisionBody — failed to create cylinder collider");
}

void MCylinderCollisionBody::releaseBody()
{
    if (!physicsBody) return;
    physicsEngine->releaseCylinderCollisionBody(physicsBody);
    physicsBody = nullptr;
}

void MCylinderCollisionBody::setupShapeCallbacks()
{
    halfHeight.setOnChangeCallback([this](auto) { syncShape(); });
    radius.setOnChangeCallback([this](auto)     { syncShape(); });
    axis.setOnChangeCallback([this](auto v)
    {
        if (physicsBody) { physicsBody->setAxis(v); syncShape(); }
    });
}

void MCylinderCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody) syncShape();
}

void MCylinderCollisionBody::syncShape()
{
    if (!physicsBody) return;
    float scaleR, scaleH;
    axisScaleFactors(scaleR, scaleH, getRelativeScale());
    physicsBody->setRadius(radius.get()         * scaleR);
    physicsBody->setHalfHeight(halfHeight.get() * scaleH);
}

void MCylinderCollisionBody::onDrawGizmo(SVector2)
{
    float scaleR, scaleH;
    axisScaleFactors(scaleR, scaleH, getRelativeScale());

    SVector3 localAxis;
    switch (axis.get())
    {
    case EShapeAxis::X: localAxis = {1,0,0}; break;
    case EShapeAxis::Z: localAxis = {0,0,1}; break;
    default:            localAxis = {0,1,0}; break;
    }

    const SColor color = isSensor.get() ? SColor(0.2f, 0.8f, 1.0f, 1.0f) : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    MGizmos::drawWireCylinder(getWorldPosition(),
        halfHeight.get() * scaleH, radius.get() * scaleR,
        getWorldRotation() * localAxis, color, 1.0f);
}