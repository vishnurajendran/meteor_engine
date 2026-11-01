//
// Created by ssj5v on 27-01-2025.
//
#include "lighting_system_manager.h"
#include <GL/glew.h>
#include "../../../../../engine/lighting/dynamiclights/dynamic_light.h"
#include "../../../../../engine/lighting/light_entity.h"
#include "dynamic_light_datatype.h"
#include "light_shader_constants.h"
#include "core/utils/frustum.h"
#include <algorithm>

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
            lightScene.setLights(dynamicLights);   // rebuilt on next query
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
        lightScene.setLights(dynamicLights);   // rebuilt on next query
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
    //reset lights data
    for (auto & data : dynLightsData.data)
    {
        data.type = EDynamicLightDataType::None;
    }

    const auto& lights = lightScene.queryLights(bounds, MAX_DYN_LIGHTS);

    // queryLights() rebuilds the BVH first if any light changed.
    // FIX 2: Track active lights to compact the array without gaps
    int activeLightCount = 0;

    for (int i = 0; i < (int)lights.size(); i++)
    {
        if (!lights[i]->isEnabledInHierarchy() || !lights[i]->getEnabled())
            continue;

        lights[i]->prepareLightRender();
        auto data = lights[i]->getLightData();

        // Copy the shadowIndex that MShadowStage stamped onto the light entity
        // so the shader knows which shadow map slot to sample.
        data.shadowIndex  = lights[i]->getShadowIndex();
        data.smoothShadow = lights[i]->getSmoothShadow() ? 1 : 0;

        // Pack data sequentially
        dynLightsData.data[activeLightCount] = data;
        activeLightCount++;
    }

    // Set the buffer count to ONLY the active, packed lights
    dynLightsData.lightCount = activeLightCount;

    uploadDynamicLights();
}

void MLightSystemManager::prepareViewDynamicLights(const SFrustum* frustum, const SVector3& cameraPosition)
{
    for (auto& data : dynLightsData.data)
        data.type = EDynamicLightDataType::None;

    int count = 0;
    if (frustum)
    {
        // BVH query: skips whole groups of off-screen lights, rebuilds first
        // if any light was added, removed, moved or resized. Nearest first.
        for (MDynamicLight* light : lightScene.queryLightsInFrustum(*frustum, cameraPosition))
        {
            if (count >= MAX_DYN_LIGHTS) break;
            if (!light->isEnabledInHierarchy() || !light->getEnabled()) continue;

            light->prepareLightRender();
            auto data         = light->getLightData();
            data.shadowIndex  = light->getShadowIndex();
            data.smoothShadow = light->getSmoothShadow() ? 1 : 0;
            dynLightsData.data[count++] = data;
        }
    }
    dynLightsData.lightCount = count;

    uploadDynamicLights();
}

void MLightSystemManager::uploadDynamicLights()
{
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
    lightScene.markDirty();
}