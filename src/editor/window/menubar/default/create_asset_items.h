// create_asset_items.h
// Suggested location: editor/window/menubar/items/create_asset_items.h
//
// Menubar items under "Assets/Create/Rendering/...".
// Each item follows the same pattern as add_entity.h:
//   onSelect()  - arms a flag to open the dialog
//   drawPopup() - draws the modal every frame (called by drawAllPopups())
//
// All three items create assets through the editor's template registry
// (MEditorAssetManager::createAssetFromTemplate), the same path the asset
// browser's Create menu uses. Template content lives in the templates folder
// and templates.xml; these dialogs only collect the name, folder and params.
//
// To add a new creatable asset type, prefer adding a template (templates.xml,
// or registerBuiltInAssetTemplates for generated ones): it shows up in the
// asset browser's Create menu automatically. A menubar item is only needed
// if it should also appear in the top menu bar.

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
    std::string path;   // shader asset path passed to the material template
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
    std::string               lastError;
    static bool registered;
};

// --- Shader ------------------------------------------------------------------

struct SShaderTemplateEntry
{
    std::string id;     // template id, e.g. "shader.lit"
    std::string label;  // display name, e.g. "Lit Shader"
    std::string extension;
};

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
    int  selectedTemplate = 0; // index into cachedTemplates

    // Every registered template that produces a shader, rebuilt on open so
    // shader templates added through templates.xml appear here too.
    std::vector<SShaderTemplateEntry> cachedTemplates;
    std::string                       lastError;
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

    std::string lastError;
    static bool registered;
};

#endif // CREATE_ASSET_ITEMS_H