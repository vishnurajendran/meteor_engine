//
// Created by ssj5v on 27-01-2025.
//
#include "lighting_system_manager.h"
#include <GL/glew.h>
#include "dynamiclights/dynamic_light.h"
#include "dynamiclights/dynamic_light_datatype.h"
#include "light_entity.h"
#include "light_shader_constants.h"

MLightSystemManager* MLightSystemManager::lightingManagerInstance = nullptr;

void MLightSystemManager::registerLight(MLightEntity* light)
{
    if (light == nullptr)
        return;

    switch (light->getLightType())
    {
        case ELightType::Ambient:
            ambientLightInstance = light;
        break;

        case ELightType::Directional:
            directionalLightInstance = light;
        break;

        case ELightType::Point:
        case ELightType::Spot:
            addLightToDynamicLights(dynamic_cast<MDynamicLight*>(light));
            lightScene.build(dynamicLights);
        break;
    }
}

void MLightSystemManager::unregisterLight(MLightEntity* light)
{
    if (light == nullptr)
        return;

    switch (light->getLightType())
    {
    case ELightType::Ambient:
        if (light == ambientLightInstance) ambientLightInstance = nullptr;
        break;

    case ELightType::Directional:
        if (light == directionalLightInstance) directionalLightInstance = nullptr;
        break;

    case ELightType::Point:
    case ELightType::Spot:
        removeLightFromDynamicLights(light);
        lightScene.build(dynamicLights);
        break;
    }
}

void MLightSystemManager::prepareLights()
{
    if (ambientLightInstance != nullptr)
    {
        ambientLightInstance->prepareLightRender();
    }

    if (directionalLightInstance != nullptr)
    {
        directionalLightInstance->prepareLightRender();
    }

    for (const auto& light : dynamicLights)
    {
        if (light != nullptr)
            light->prepareLightRender();
    }
}

MLightSystemManager* MLightSystemManager::getInstance()
{
    if (lightingManagerInstance == nullptr)
    {
        lightingManagerInstance = new MLightSystemManager();
    }

    return lightingManagerInstance;
}

void MLightSystemManager::addLightToDynamicLights(MLightEntity* light)
{
    auto it = std::find(dynamicLights.begin(), dynamicLights.end(), light);
    if (it != dynamicLights.end())
    {
        return;
    }
    dynamicLights.push_back(dynamic_cast<MDynamicLight*>(light));
}

void MLightSystemManager::removeLightFromDynamicLights(MLightEntity* light)
{
    auto it = std::find(dynamicLights.begin(), dynamicLights.end(), light);
    if (it == dynamicLights.end())
    {
       return;
    }
    dynamicLights.erase(it);
}

void MLightSystemManager::prepareDynamicLights(const AABB& bounds)
{
    //rebuild if it was requested.
    if (rebuildRequested)
    {
        rebuildRequested = false;
        lightScene.build(dynamicLights);
    }

    //reset lights data
    for (auto & data : dynLightsData.data)
    {
        data.type = EDynamicLightDataType::None;
    }

    const auto& lights = lightScene.queryLights(bounds, MAX_DYN_LIGHTS);
    dynLightsData.lightCount = lights.size();
    for (int i = 0; i < lights.size(); i++)
    {
        //ignore disabled lights.
        if (!lights[i]->getEnabled())
            continue;

        lights[i]->prepareLightRender();
        const auto& data = lights[i]->getLightData();
        dynLightsData.data[i] = data;
    }

    if (!dynLightsBuffer)
    {
        glGenBuffers(1, &dynLightsBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynLightsBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SDynamicLightShaderContainer), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, LIGHT_INDEX_DYNAMIC, dynLightsBuffer, 0,
                          sizeof(SDynamicLightShaderContainer));
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, dynLightsBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SDynamicLightShaderContainer), &dynLightsData);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MLightSystemManager::requestLightSceneRebuild()
{
    rebuildRequested = true;
}
