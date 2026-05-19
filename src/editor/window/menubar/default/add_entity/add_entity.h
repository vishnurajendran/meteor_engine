#pragma once
#ifndef ADD_ENTITY_MENU_H
#define ADD_ENTITY_MENU_H

#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"

// ── Primitives ────────────────────────────────────────────────────────────────

#define DECLARE_PRIMITIVE_ITEM(ClassName, ItemName, MeshPath) \
class ClassName : public MMenubarItem { \
public: \
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; } \
    [[nodiscard]] SString getPath()     const override { return "Add/3D Object/Primitives/" ItemName; } \
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

// ── Static Mesh ───────────────────────────────────────────────────────────────

class MAddStaticMeshItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddStaticMeshItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/3D Object/New Static Mesh"; }
    void onSelect() override;
private:
    static bool registered;
};

// ── Lights ────────────────────────────────────────────────────────────────────

#define DECLARE_LIGHT_ITEM(ClassName, ItemName) \
class ClassName : public MMenubarItem { \
DEFINE_OBJECT_SUBCLASS(ClassName) \
public: \
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; } \
    [[nodiscard]] SString getPath()     const override { return "Add/Light/" ItemName; } \
    void onSelect() override; \
private: \
    static bool registered; \
};

DECLARE_LIGHT_ITEM(MAddPointLightItem,       "Point Light")
DECLARE_LIGHT_ITEM(MAddSpotLightItem,        "Spot Light")
DECLARE_LIGHT_ITEM(MAddDirectionalLightItem, "Directional Light")
DECLARE_LIGHT_ITEM(MAddAmbientLightItem,     "Ambient Light")

#undef DECLARE_LIGHT_ITEM

// ── Sky ───────────────────────────────────────────────────────────────────────

class MAddProceduralSkyboxItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddProceduralSkyboxItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/Sky/Procedural Skybox"; }
    void onSelect() override;
private:
    static bool registered;
};

// ── Misc ──────────────────────────────────────────────────────────────────────

class MAddCameraItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddCameraItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/Camera"; }
    void onSelect() override;
private:
    static bool registered;
};

class MAddAudioListenerItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddAudioListenerItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/Audio/Listener"; }
    void onSelect() override;
private:
    static bool registered;
};

class MAddAudioSourceItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddAudioSourceItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 3; }
    [[nodiscard]] SString getPath()     const override { return "Add/Audio/Source"; }
    void onSelect() override;
private:
    static bool registered;
};

class MAddEmptySpatialItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAddEmptySpatialItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR + 999; }
    [[nodiscard]] SString getPath()     const override { return "Add/Empty Spatial"; }

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::N, true, true};
    };

    void onSelect() override;
private:
    static bool registered;
};

#endif //ADD_ENTITY_MENU_H