//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHTING_MANAGER_H
#define LIGHTING_MANAGER_H
#include "../../../../../engine/lighting/dynamic_light_shader_container.h"
#include "../../../../../object/object.h"
#include "../../../../../utils/glmhelper.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/utils/aabb.h"
#include "lightbvh/lightscene.h"


class MLightEntity;

class MLightSystemManager : MObject {
public:
    void registerLight(MLightEntity* light);
    void unregisterLight(MLightEntity* light);
    void prepareLights();
    void prepareDynamicLights(const AABB& bounds);
    void requestLightSceneRebuild();

    // ── Directional shadow - queries the directional light entity ─────────
    // These replace the old global booleans. If no directional light exists,
    // they return safe defaults.
    bool isDirectionalShadowEnabled() const
    {
        auto* dl = dynamic_cast<MDirectionalLight*>(directionalLightInstance);
        return dl ? dl->getCastsShadow() : false;
    }

    bool isDirectionalSmoothShadow() const
    {
        auto* dl = dynamic_cast<MDirectionalLight*>(directionalLightInstance);
        return dl ? dl->getSmoothShadow() : false;
    }

    MLightEntity* getDirectionalLight() const { return directionalLightInstance; }
    const std::vector<MDynamicLight*>& getDynamicLights() const { return dynamicLights; }
public:
    static MLightSystemManager* getInstance();
private:
    MLightEntity* ambientLightInstance = nullptr;
    MLightEntity* directionalLightInstance = nullptr;
    std::vector<MDynamicLight*> dynamicLights;


    bool rebuildRequested = false;
    MLightScene lightScene;
    unsigned int dynLightsBuffer=0;
    SDynamicLightShaderContainer dynLightsData = {};

    void addLightToDynamicLights(MLightEntity* light);
    void removeLightFromDynamicLights(MLightEntity* light);

    static MLightSystemManager* lightingManagerInstance ;
};

#endif //LIGHTING_MANAGER_H