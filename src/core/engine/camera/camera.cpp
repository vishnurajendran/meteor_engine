//
// camera.cpp
//
#include "camera.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/texture/textureasset.h"
#include "viewmanagement.h"

IMPLEMENT_CLASS(MCameraEntity)

MCameraEntity::MCameraEntity()
{
    name = "camera";
    MViewManagement::addCamera(this);
}

MCameraEntity::~MCameraEntity()
{
    MViewManagement::removeCamera(this);
}

// Setters / getters delegate to DECLARE_FIELD members
// The fields themselves are the source of truth; no separate private members.

void MCameraEntity::setPriority(const int& p)    { priority = p; MViewManagement::updateCameraOrder(); }
int  MCameraEntity::getPriority() const           { return priority.get(); }

void MCameraEntity::setOrthographic(const bool& o) { isOrthographic = o; }
bool MCameraEntity::getOrthographic() const         { return isOrthographic.get(); }

void MCameraEntity::setClipPlanes(float nearClip, float farClip)
{
    nearPlane = nearClip;
    farPlane  = farClip;
}

std::pair<float, float> MCameraEntity::getClipPlanes() const
{
    return { nearPlane.get(), farPlane.get() };
}

void  MCameraEntity::setFov(const float& f) { fov = f; }
float MCameraEntity::getFov() const          { return fov.get(); }

// Matrices
SMatrix4 MCameraEntity::getProjectionMatrix(const SVector2& resolution) const
{
    if (!isOrthographic.get())
    {
        return glm::perspective(glm::radians(fov.get()),
                                resolution.x / resolution.y,
                                nearPlane.get(), farPlane.get());
    }
    return glm::ortho(0.0f, resolution.x, 0.0f, resolution.y,
                      nearPlane.get(), farPlane.get());
}

SMatrix4 MCameraEntity::getViewMatrix() const
{
    SQuaternion worldRotation = getWorldRotation();
    SVector3    worldPosition = getWorldPosition();
    return glm::mat4_cast(glm::conjugate(worldRotation))
         * glm::translate(SMatrix4(1.0f), -worldPosition);
}

//  Gizmo 

void MCameraEntity::onDrawGizmo(SVector2 renderResolution)
{
    const auto texture = MAssetManager::getInstance()
        ->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/camera.png");
    MGizmos::drawTextureRect(getWorldPosition(), SVector2(0.5f, 0.5f), texture->getTexture());
    MGizmos::drawWireFrustum(getViewMatrix(), getProjectionMatrix(renderResolution),
                             SColor(1, 1, 1, 1), 1.0f);
}