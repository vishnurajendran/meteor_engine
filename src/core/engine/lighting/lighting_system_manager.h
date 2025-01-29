//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHTING_MANAGER_H
#define LIGHTING_MANAGER_H
#include "core/object/object.h"

class MLightEntity;

class MLightSystemManager : MObject {
public:
    void registerLight(MLightEntity* light);
    void unregisterLight(MLightEntity* light);
    void prepareLights();
public:
    static MLightSystemManager* getInstance();
private:
    MLightEntity* ambientLightInstance = nullptr;
    MLightEntity* directionalLightInstance = nullptr;
    std::vector<MLightEntity*> otherLights;

    void addLightToOthers(MLightEntity* light);
    void removeLightFromOthers(MLightEntity* light);

    static MLightSystemManager* lightingManagerInstance ;
};

#endif //LIGHTING_MANAGER_H
