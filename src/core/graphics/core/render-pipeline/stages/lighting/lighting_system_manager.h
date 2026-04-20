//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHTING_MANAGER_H
#define LIGHTING_MANAGER_H
#include "../../../../../engine/lighting/dynamic_light_shader_container.h"
#include "../../../../../object/object.h"
#include "../../../../../utils/glmhelper.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
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

    // Shadow toggle and soft shadow mode — read by MShadowStage and MLightingStage each frame.
    bool directionalShadowEnabled = true;
    bool smoothShadows            = false;
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