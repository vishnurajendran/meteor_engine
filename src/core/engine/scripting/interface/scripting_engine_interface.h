//
// Created by ssj5v on 04-06-2026.
//

#ifndef SCRIPTING_ENGINE_INTERFACE_H
#define SCRIPTING_ENGINE_INTERFACE_H
#include "core/engine/subsystem/subsystem_interface.h"

class MSpatialEntity;
class MUIWidget;
class IScriptAsset;
class IScriptInstance;
class IScriptingEngineSubsystem : public IEngineSubSystem
{
public:
    IScriptingEngineSubsystem() = default;
    virtual ~IScriptingEngineSubsystem() override = default;

    virtual IScriptInstance* createScriptInstance(IScriptAsset* asset,  MSpatialEntity* owningEntity)  = 0;

    // A behaviour owned by a UI widget instead of an entity (see MUIWidget).
    // The script must extend WidgetBehaviour; nullptr (with the error logged)
    // otherwise. Created in the current domain, like entity instances.
    virtual IScriptInstance* createWidgetScriptInstance(IScriptAsset* asset, const MUIWidget& owner) = 0;
    virtual void releaseScriptInstance(IScriptInstance* scriptInstance) = 0;
};

#endif //SCRIPTING_ENGINE_INTERFACE_H
