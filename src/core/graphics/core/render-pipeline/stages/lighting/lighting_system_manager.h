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
struct SFrustum;

// Every light in the world. Owned by the render pipeline manager:
//   MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>()->getLightSystem()
class MLightSystemManager : public MObject {
public:
    void registerLight(MLightEntity* light);
    void unregisterLight(MLightEntity* light);
    void prepareLights();
    void prepareDynamicLights(const AABB& bounds);

    // Uploads one light list for the whole view: every enabled point/spot
    // light whose range touches the frustum (queried through the light BVH),
    // nearest to the camera first, up to MAX_DYN_LIGHTS. Pass nullptr to upload an empty list (e.g. when
    // a pipeline light override is active). Call after the shadow stage has
    // assigned shadow indices for the frame.
    void prepareViewDynamicLights(const SFrustum* frustum, const SVector3& cameraPosition);
    // A light moved or changed range - the BVH rebuilds on its next query.
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
private:
    MLightEntity* ambientLightInstance = nullptr;
    MLightEntity* directionalLightInstance = nullptr;
    std::vector<MDynamicLight*> dynamicLights;


    MLightScene lightScene;
    unsigned int dynLightsBuffer=0;
    SDynamicLightShaderContainer dynLightsData = {};

    void uploadDynamicLights();
    void addLightToDynamicLights(MLightEntity* light);
    void removeLightFromDynamicLights(MLightEntity* light);
};

#endif //LIGHTING_MANAGER_H