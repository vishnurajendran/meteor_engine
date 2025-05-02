//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHTING_MANAGER_H
#define LIGHTING_MANAGER_H
#include "core/object/object.h"
#include "core/utils/glmhelper.h"
#include "dynamic_light_shader_container.h"
#include "dynamiclights/lightbvh/lightscene.h"


class MLightEntity;

class MLightSystemManager : MObject {
public:
    void registerLight(MLightEntity* light);
    void unregisterLight(MLightEntity* light);
    void prepareLights();
    void prepareDynamicLights(const AABB& bounds);
    void requestLightSceneRebuild();
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
