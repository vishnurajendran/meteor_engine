//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHT_ENTITY_H
#define LIGHT_ENTITY_H
#include "light_type.h"
#include "core/engine/entities/spatial/spatial.h"

class MLightEntity : public MSpatialEntity {
public:
    virtual void prepareLightRender() = 0;
    ELightType getLightType() {return lightType;};

    virtual void setColor(const SColor& color)=0;
    virtual SColor getColor() const = 0;

    virtual void setIntensity(const float& intensity)=0;
    virtual float getIntensity() const = 0;
protected:
    ELightType lightType;

    const int LIGHT_INDEX_AMBIENT = 1;
    const int LIGHT_INDEX_DIRECTIONAL = 2;
};

#endif //LIGHT_ENTITY_H
