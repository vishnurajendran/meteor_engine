#pragma once
#ifndef ADD_ENTITY_MENU_H
#define ADD_ENTITY_MENU_H
#include "editor/window/menubar/menubaritem.h"
#include "core/engine/audio/audio_entity/audio_listener_entity.h"
#include "core/engine/audio/audio_entity/audio_source_entity.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "core/engine/input/input.h"
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"
#include "core/engine/physics/entities/box_collision_body_entity.h"
#include "core/engine/physics/entities/sphere_collision_body_entity.h"
#include "core/engine/skybox/procedural_sky/procedural_sky.h"
#include "menu_additem_macros.h"

// -- Empty --------------------------------------------------------------------
class MAddEmptySpatialItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddEmptySpatialItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR - 999; }
    [[nodiscard]] SString getPath()     const override { return "Add/Empty Spatial"; }

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::N, true, true};
    };

    void onSelect() override;
private:
    static bool registered;
};

// -- Static Mesh ---------------------------------------------------------------
// Primitives -------------------------------------------------------------------

#define DECLARE_PRIMITIVE_ITEM(ClassName, ItemName, MeshPath) \
class ClassName : public MMenubarItem { \
public: \
[[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; } \
[[nodiscard]] SString getPath()     const override { return "Add/Meshes/Primitives/" ItemName; } \
void onSelect() override; \
private: \
static bool registered; \
};

DECLARE_PRIMITIVE_ITEM(MAddCubeItem,     "Cube",     "")
DECLARE_PRIMITIVE_ITEM(MAddSphereItem,   "Sphere",   "")
DECLARE_PRIMITIVE_ITEM(MAddCylinderItem, "Cylinder", "")
DECLARE_PRIMITIVE_ITEM(MAddPlaneItem,    "Plane",    "")
DECLARE_PRIMITIVE_ITEM(MAddConeItem,     "Cone",     "")
DECLARE_PRIMITIVE_ITEM(MAddCapsuleItem,  "Capsule",  "")

#undef DECLARE_PRIMITIVE_ITEM

class MAddStaticMeshItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddStaticMeshItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/3D Object/New Static Mesh"; }
    void onSelect() override;
private:
    static bool registered;
};


REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddPointLightItem,
    "Add/Light/Point Light", MMenubarItem::PRIORITY_REGULAR + 3,
    MPointLight,
    "Point Light"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddSpotLightItem,
    "Add/Light/Spot Light", MMenubarItem::PRIORITY_REGULAR + 3,
    MSpotLight,
    "Spot Light"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddDirectionalLightItem,
    "Add/Light/Directional Light",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MDirectionalLight,
    "Directional Light"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddAmbientLightItem,
    "Add/Light/Ambient Light",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MAmbientLightEntity,
    "Ambient Light"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddProceduralSkyboxItem,
    "Add/Sky/Procedural Skybox",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MProceduralSkyboxEntity,
    "Procedural Skybox"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddCameraItem,
    "Add/Camera",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MCameraEntity,
    "Camera"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddAudioListenerItem,
    "Add/Audio/Listener",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MAudioListener,
    "AudioListener"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddAudioSourceItem,
    "Add/Audio/Source",
    MMenubarItem::PRIORITY_REGULAR + 3,
    MAudioSource,
    "AudioSource"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddBoxCollisionBodyItem,
    "Add/Physics/Box Collision Body",
    MMenubarItem::PRIORITY_REGULAR + 4,
    MBoxCollisionBody,
    "Box Collision Body"
)

REGISTER_SPATIAL_ENTITY_CREATE_MENU(
    MAddSphereCollisionBodyItem,
    "Add/Physics/Sphere Collision Body",
    MMenubarItem::PRIORITY_REGULAR + 4,
    MSphereCollisionBody,
    "Sphere Collision Body"
)



#endif //ADD_ENTITY_MENU_H