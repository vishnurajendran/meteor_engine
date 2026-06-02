// create_asset_items.h
// Suggested location: editor/window/menubar/items/create_asset_items.h
//
// Menubar items under "Assets/Create/Rendering/...".
// Each item follows the same pattern as add_entity.h:
//   onSelect()  - arms a flag to open the dialog
//   drawPopup() - draws the modal every frame (called by drawAllPopups())
//
// To add a new creatable asset type:
//   1. Declare a class with DECLARE_CREATE_ASSET_ITEM (or manually).
//   2. Implement onSelect(), drawPopup(), and the static registered bool.
//   3. That's it - the menubar tree picks it up automatically.

#pragma once
#ifndef CREATE_ASSET_ITEMS_H
#define CREATE_ASSET_ITEMS_H

#include <string>
#include <vector>
#include <queue>
#include "editor/editorassetmanager/editor_asset_directory_node.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/window/menubar/menubaritem.h"

// --- Material ----------------------------------------------------------------

struct SShaderEntry
{
    std::string label;  // display name shown in the combo
    std::string path;   // asset path passed to createNewMaterial
};

class MCreateMaterialItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MCreateMaterialItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR; }
    [[nodiscard]] SString getPath()     const override { return "Assets/Create/Rendering/Material"; }

    void onSelect()  override;
    void drawPopup() override;

private:
    bool showDialog  = false;
    char matName  [128] = "NewMaterial";
    char directory [512] = "assets/materials/";
    int  shadingMode     = 0;   // 0=Lit 1=Unlit
    int  selectedShader  = 0;   // index into the shader list built on open

    std::vector<SShaderEntry> cachedShaders;
    static bool registered;
};

// --- Shader ------------------------------------------------------------------

class MCreateShaderItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MCreateShaderItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR; }
    [[nodiscard]] SString getPath()     const override { return "Assets/Create/Rendering/Shader"; }

    void onSelect()  override;
    void drawPopup() override;

private:
    bool showDialog     = false;
    char shaderName[128] = "NewShader";
    char directory [512] = "assets/shaders/";
    int  selectedTemplate = 0; // index into k_templateLabels

    static bool registered;
};

// --- Skybox ------------------------------------------------------------------

class MCreateSkyboxItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MCreateSkyboxItem)
public:
    [[nodiscard]] int     getPriority() const override { return PRIORITY_REGULAR; }
    [[nodiscard]] SString getPath()     const override { return "Assets/Create/Rendering/Skybox"; }

    void onSelect()  override;
    void drawPopup() override;

private:
    bool showDialog    = false;
    char skyboxName[128] = "NewSkybox";
    char directory [512] = "assets/skybox/";

    static bool registered;
};

#endif // CREATE_ASSET_ITEMS_H