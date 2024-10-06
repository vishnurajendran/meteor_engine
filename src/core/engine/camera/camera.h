//
// Created by ssj5v on 05-10-2024.
//

#ifndef CAMERA_H
#define CAMERA_H
#include "core/engine/entities/spatial/spatial.h"


class MCameraEntity : public MSpatialEntity {
private:
    int priority = 0;
    bool isOrthoGraphic = false;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float fov = 60;
public:
    MCameraEntity();
    ~MCameraEntity() override = default;
    void setPriority(const int& priority);
    int getPriority() const;
    void setOrthographic(const bool& orthographic);
    void setClipPlanes(float nearClip, float farFar);
    [[nodiscard]] std::pair<float, float> getClipPlanes() const;
    SMatrix4 getProjectionMatrix(const SVector2& resolution) const;
    SMatrix4 getViewMatrix() const;
    void setFov(const float& fov);
    float getFov() const;
};

#endif //CAMERA_H
