//
// Created by ssj5v on 27-01-2025.
//

#include "lighting_system_manager.h"
#include "light_entity.h"

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
            addLightToOthers(light);
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
        removeLightFromOthers(light);
        break;
    }
}

void MLightSystemManager::prepareLights()
{
    if (ambientLightInstance != nullptr)
        ambientLightInstance->prepareLightRender();

    if (directionalLightInstance != nullptr)
        directionalLightInstance->prepareLightRender();

    for (const auto& light : otherLights)
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

void MLightSystemManager::addLightToOthers(MLightEntity* light)
{
    auto it = std::find(otherLights.begin(), otherLights.end(), light);
    if (it != otherLights.end())
    {
        return;
    }
    otherLights.push_back(light);
}

void MLightSystemManager::removeLightFromOthers(MLightEntity* light)
{
    auto it = std::find(otherLights.begin(), otherLights.end(), light);
    if (it == otherLights.end())
    {
       return;
    }
    otherLights.erase(it);
}
