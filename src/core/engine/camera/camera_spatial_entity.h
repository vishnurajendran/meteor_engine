//
// camera.h
//
#ifndef CAMERA_H
#define CAMERA_H
#include "core/engine/entities/spatial/spatial.h"

SCRIPT_BIND_CLASS()
class MCameraEntity : public MSpatialEntity
{
    DEFINE_SPATIAL_CLASS(MCameraEntity)

    DECLARE_FIELD(priority,       int,   0)
    DECLARE_FIELD(isOrthographic, bool,  false)
    DECLARE_FIELD(nearPlane,      float, 0.1f)
    DECLARE_FIELD(farPlane,       float, 100.0f)
    DECLARE_FIELD(fov,            float, 60.0f)

public:
    MCameraEntity();
    ~MCameraEntity() override;

    SCRIPT_BIND_FUNC()
    void setPriority(const int& priority);
    SCRIPT_BIND_FUNC()
    int getPriority() const;

    SCRIPT_BIND_FUNC()
    void setOrthographic(const bool& orthographic);
    SCRIPT_BIND_FUNC()
    bool getOrthographic() const;

    SCRIPT_BIND_FUNC()
    void setClipPlanes(float nearClip, float farClip);
    SCRIPT_BIND_FUNC()
    SVector2 getClipPlanes() const;

    SCRIPT_BIND_FUNC()
    SMatrix4 getProjectionMatrix(const SVector2& resolution) const;
    SCRIPT_BIND_FUNC()
    SMatrix4 getViewMatrix() const;

    SCRIPT_BIND_FUNC()
    void setFov(const float& fov);
    SCRIPT_BIND_FUNC()
    float getFov() const;

    void onDrawGizmo(SVector2 renderResolution) override;
};

#endif // CAMERA_H