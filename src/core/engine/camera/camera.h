//
// camera.h
//
#ifndef CAMERA_H
#define CAMERA_H
#include "core/engine/entities/spatial/spatial.h"

class MCameraEntity : public MSpatialEntity
{
    DEFINE_CLASS(MCameraEntity)

    DECLARE_FIELD(priority,       int,   0)
    DECLARE_FIELD(isOrthographic, bool,  false)
    DECLARE_FIELD(nearPlane,      float, 0.1f)
    DECLARE_FIELD(farPlane,       float, 100.0f)
    DECLARE_FIELD(fov,            float, 60.0f)

public:
    MCameraEntity();
    ~MCameraEntity() override;

    void setPriority(const int& priority);
    [[nodiscard]] int getPriority() const;

    void setOrthographic(const bool& orthographic);
    [[nodiscard]] bool getOrthographic() const;

    void setClipPlanes(float nearClip, float farClip);
    [[nodiscard]] std::pair<float, float> getClipPlanes() const;

    [[nodiscard]] SMatrix4 getProjectionMatrix(const SVector2& resolution) const;
    [[nodiscard]] SMatrix4 getViewMatrix() const;

    void setFov(const float& fov);
    [[nodiscard]] float getFov() const;

    void onDrawGizmo(SVector2 renderResolution) override;
};

#endif // CAMERA_H