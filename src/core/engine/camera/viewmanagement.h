//
// Created by ssj5v on 05-10-2024.
//

#ifndef VIEWMANAGEMENT_H
#define VIEWMANAGEMENT_H
#include <vector>

#include "camera_spatial_entity.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "core/engine/subsystem/subsystem_registry.h"   // so users can reach it
#include "core/object/object.h"

/// Every camera during engine runtime, sorted by priority.
///
/// An engine subsystem, registered under its own type by each application
/// (before the scene manager - the editor's empty scene creates a camera):
///
///   auto* views = MEngineSubsystemRegistry::getSubsystem<MViewManagement>();
///
/// Cameras add themselves on construction and remove themselves on
/// destruction.
class MViewManagement : public MObject, public IEngineSubSystem {
    DEFINE_OBJECT_SUBCLASS(MViewManagement)
public:
    void init() override {}
    void cleanup() override { cameras.clear(); }   // cameras are owned by their scenes

    void addCamera(MCameraEntity* camera);
    void removeCamera(MCameraEntity* camera);
    std::vector<MCameraEntity*>& getCameras();
    MCameraEntity* getFirstActiveCamera();
    void updateCameraOrder();

private:
    std::vector<MCameraEntity*> cameras;
};



#endif //VIEWMANAGEMENT_H
