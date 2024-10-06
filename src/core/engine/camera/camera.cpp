//
// Created by ssj5v on 05-10-2024.
//

#include "camera.h"
#include "viewmanagement.h"

MCameraEntity::MCameraEntity() : MSpatialEntity() {
    name = "camera";
    MViewManagement::addCamera(this);
}

void MCameraEntity::setPriority(const int &priority) {
    this->priority = priority;
    MViewManagement::updateCameraOrder();
}

int MCameraEntity::getPriority() const {
    return this->priority;
}

void MCameraEntity::setOrthographic(const bool &orthographic) {
    isOrthoGraphic = orthographic;
}

void MCameraEntity::setClipPlanes(float nearClip, float farClip) {
    nearPlane = nearClip;
    farPlane = farClip;
}

std::pair<float, float> MCameraEntity::getClipPlanes() const {
    return {nearPlane, farPlane};
}

SMatrix4 MCameraEntity::getProjectionMatrix(const SVector2 &resolution) const {

    SMatrix4 projectionMatrix;

    if (!isOrthoGraphic) {
        projectionMatrix =  glm::perspective(glm::radians(fov), resolution.x / resolution.y, nearPlane, farPlane);
    }
    else {
        const float left = 0.0f;
        const float right = resolution.x;
        const float bottom = 0.0f;
        const float top = resolution.y;
        const float zNear = nearPlane;
        const float zFar = farPlane;
        projectionMatrix = glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    return projectionMatrix;
}

SMatrix4 MCameraEntity::getViewMatrix() const {
    SQuaternion worldRotation = getWorldRotation();
    SVector3 worldPosition = getWorldPosition();
    SMatrix4 rotationMatrix = glm::mat4_cast(glm::conjugate(worldRotation));
    SMatrix4 translationMatrix = glm::translate(SMatrix4(1.0f), -worldPosition);
    SMatrix4 viewMatrix = rotationMatrix * translationMatrix;
    return viewMatrix;
}

void MCameraEntity::setFov(const float &fov) {
    this->fov = fov;
}

float MCameraEntity::getFov() const {
    return this->fov;
}
