//
// light_entity.h
//
#ifndef LIGHT_ENTITY_H
#define LIGHT_ENTITY_H
#include "../../graphics/core/render-pipeline/stages/lighting/light_type.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/utils/color.h"

SCRIPT_BIND_CLASS()
class MLightEntity : public MSpatialEntity
{
    DEFINE_ABSTRACT_SPATIAL_CLASS(MLightEntity)

public:
    virtual void prepareLightRender() = 0;

    ELightType getLightType() const { return lightType; }

    SCRIPT_BIND_FUNC()
    virtual void setColor(const SColor& color)      = 0;
    SCRIPT_BIND_FUNC()
    virtual SColor getColor() const                  = 0;
    SCRIPT_BIND_FUNC()
    virtual void setIntensity(const float& intensity) = 0;
    SCRIPT_BIND_FUNC()
    virtual float getIntensity() const                = 0;

protected:
    ELightType lightType = ELightType::Ambient;
};

#endif // LIGHT_ENTITY_H