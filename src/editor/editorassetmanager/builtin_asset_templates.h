//
// builtin_asset_templates.h
//
// Registers the templates that ship with the editor:
//
//   shader.lit / shader.unlit / shader.unlit_color / shader.toon   (file)
//   skybox                                                          (file)
//   script.lua                                                      (file)
//   material                                                        (generated)
//
// The shader and skybox entries use the existing template files named in
// SEditorPaths, so current templates keep working unchanged.
//
// Intended location: src/editor/editorassetmanager/builtin_asset_templates.h
//

#ifndef BUILTIN_ASSET_TEMPLATES_H
#define BUILTIN_ASSET_TEMPLATES_H

class MAssetTemplateRegistry;

namespace SBuiltInTemplateIds
{
    inline constexpr const char* ShaderLit        = "shader.lit";
    inline constexpr const char* ShaderUnlit      = "shader.unlit";
    inline constexpr const char* ShaderUnlitColor = "shader.unlit_color";
    inline constexpr const char* ShaderToon       = "shader.toon";
    inline constexpr const char* Skybox           = "skybox";
    inline constexpr const char* LuaScript        = "script.lua";
    inline constexpr const char* Material         = "material";

    // Params of the material template
    inline constexpr const char* MaterialParamShader = "shader";   // shader asset path
    inline constexpr const char* MaterialParamMode   = "mode";     // "lit" | "unlit"
}

void registerBuiltInAssetTemplates(MAssetTemplateRegistry& registry);

#endif // BUILTIN_ASSET_TEMPLATES_H
