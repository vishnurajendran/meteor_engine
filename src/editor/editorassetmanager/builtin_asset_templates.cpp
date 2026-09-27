//
// builtin_asset_templates.cpp
//
// Intended location: src/editor/editorassetmanager/builtin_asset_templates.cpp
//

#include "builtin_asset_templates.h"

#include <cstring>
#include <map>
#include <sstream>

#include "asset_template_registry.h"
#include "editorassetmanager.h"   // SEditorPaths
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/shader/shader_utils.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "pugixml.hpp"

namespace
{

// ---------------------------------------------------------------------------
// Shader-declared defaults
//
// The live MShader's property values can be changed by materials that share
// it, so defaults are read from the shader file instead. Only the
// <properties> block is parsed: the rest of a .mesl file is GLSL and is not
// valid XML until MShaderAsset preprocesses it.
// ---------------------------------------------------------------------------

std::map<SString, SString> readDeclaredDefaults(const SString& shaderPath)
{
    std::map<SString, SString> defaults;

    SString text;
    if (!MAssetSources::getActive()->readText(shaderPath, text))
        return defaults;

    const std::string& s = text.str();
    const size_t begin = s.find("<properties");
    const size_t end   = s.find("</properties>");
    if (begin == std::string::npos || end == std::string::npos || end < begin)
        return defaults;

    const std::string block = s.substr(begin, end + std::strlen("</properties>") - begin);
    pugi::xml_document doc;
    if (!doc.load_buffer(block.data(), block.size()))
        return defaults;

    for (auto prop : doc.child("properties").children("property"))
    {
        const auto key = prop.attribute("key");
        const auto def = prop.attribute("default");
        if (key && def)
            defaults[SString(key.value())] = SString(def.value());
    }
    return defaults;
}

const char* fallbackDefault(SShaderPropertyType type)
{
    switch (type)
    {
        case SShaderPropertyType::Float:       return "0";
        case SShaderPropertyType::Int:         return "0";
        case SShaderPropertyType::Bool:        return "False";
        case SShaderPropertyType::UniformVec2: return "(0,0)";
        case SShaderPropertyType::UniformVec3: return "(0,0,0)";
        case SShaderPropertyType::UniformVec4: return "(0,0,0,1)";
        case SShaderPropertyType::Color:       return "(1,1,1,1)";
        case SShaderPropertyType::Texture:     return "";
        default:                               return "0";
    }
}

// ---------------------------------------------------------------------------
// Material generator
//
// Moved from MMaterialAsset::createNewMaterial so the player build no longer
// carries editor-only creation code. Writes the shader's properties in the
// shader's declaration order, using the defaults declared in the shader
// file (e.g. toon _useOutline="True"), falling back to per-type defaults.
// Empty values are only kept for textures, matching what
// MMaterialAsset::loadFromSource accepts.
// ---------------------------------------------------------------------------

bool generateMaterial(const STemplateContext& ctx, SString& outContent)
{
    const SString shaderPath = ctx.param(SBuiltInTemplateIds::MaterialParamShader);
    const SString mode       = ctx.param(SBuiltInTemplateIds::MaterialParamMode, "lit");

    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!assetManager)
    {
        MERROR("MaterialTemplate:: asset manager not available");
        return false;
    }

    const auto shaderAsset = assetManager->getAsset<MShaderAsset>(shaderPath);
    if (!shaderAsset)
    {
        MERROR(SString::format("MaterialTemplate:: invalid shader asset {0}", shaderPath));
        return false;
    }

    MShader* shader = shaderAsset->getShader();
    if (!shader)
    {
        MERROR(SString::format("MaterialTemplate:: shader has errors {0}", shaderPath));
        return false;
    }

    pugi::xml_document doc;
    auto root = doc.append_child("material");
    root.append_attribute("name").set_value(ctx.name.c_str());
    root.append_child("shaderPathField").text().set(shaderPath.c_str());
    root.append_child("shadingModeStr").text().set(mode == "unlit" ? "unlit" : "lit");

    const auto declared = readDeclaredDefaults(shaderPath);

    auto xmlProps = root.append_child("properties");
    const auto& props = shader->getProperties();
    for (const auto& key : shader->getPropertyOrder())
    {
        auto it = props.find(key);
        if (it == props.end()) continue;
        const auto type = it->second.getType();

        SString value = fallbackDefault(type);
        if (auto d = declared.find(key); d != declared.end())
        {
            if (!d->second.empty() || type == SShaderPropertyType::Texture)
                value = d->second;
        }

        auto child = xmlProps.append_child("property");
        child.append_attribute("key").set_value(key.c_str());
        child.append_attribute("type").set_value(MShaderUtility::getTypeStr(type).c_str());
        child.append_attribute("value").set_value(value.c_str());
    }

    std::ostringstream oss;
    doc.save(oss);
    outContent = SString(oss.str());
    return true;
}

SAssetTemplate fileTemplate(const char* id, const char* displayName, const char* menuPath,
                            const char* extension, const char* defaultName, const char* file)
{
    SAssetTemplate t;
    t.id           = id;
    t.displayName  = displayName;
    t.menuPath     = menuPath;
    t.extension    = extension;
    t.defaultName  = defaultName;
    t.templateFile = file;
    return t;
}

} // namespace

void registerBuiltInAssetTemplates(MAssetTemplateRegistry& registry)
{
    using namespace SBuiltInTemplateIds;

    registry.registerTemplate(fileTemplate(ShaderLit,        "Lit Shader",          "Shader/Lit",
                                           SEditorPaths::EXTENSION_SHADER, "New_Shader",
                                           SEditorPaths::TEMPLATE_SHADER_LIT_FILE));
    registry.registerTemplate(fileTemplate(ShaderUnlit,      "Unlit Shader",        "Shader/Unlit",
                                           SEditorPaths::EXTENSION_SHADER, "New_Shader",
                                           SEditorPaths::TEMPLATE_SHADER_UNLIT_FILE));
    registry.registerTemplate(fileTemplate(ShaderUnlitColor, "Unlit Color Shader",  "Shader/Unlit Color",
                                           SEditorPaths::EXTENSION_SHADER, "New_Shader",
                                           SEditorPaths::TEMPLATE_SHADER_UNLIT_COLOR_FILE));
    registry.registerTemplate(fileTemplate(ShaderToon,       "Toon Shader",         "Shader/Toon",
                                           SEditorPaths::EXTENSION_SHADER, "New_Shader",
                                           SEditorPaths::TEMPLATE_SHADER_TOONLIT_FILE));
    registry.registerTemplate(fileTemplate(Skybox,           "Skybox",              "Skybox",
                                           SEditorPaths::EXTENSION_SKYBOX, "New_Skybox",
                                           SEditorPaths::TEMPLATE_SKYBOX_FILE));
    registry.registerTemplate(fileTemplate(LuaScript,        "Lua Script",          "Script/Lua",
                                           ".lua", "NewScript",
                                           "script.lua.template"));

    SAssetTemplate material;
    material.id          = Material;
    material.displayName = "Material";
    material.menuPath    = "Material";
    material.extension   = ".material";
    material.defaultName = "New_Material";
    material.generator   = generateMaterial;

    STemplateParam shaderParam;
    shaderParam.key            = MaterialParamShader;
    shaderParam.label          = "Shader";
    shaderParam.kind           = ETemplateParamKind::AssetRef;
    shaderParam.assetExtension = "shader";
    material.params.push_back(shaderParam);

    STemplateParam modeParam;
    modeParam.key          = MaterialParamMode;
    modeParam.label        = "Shading";
    modeParam.kind         = ETemplateParamKind::Enum;
    modeParam.options      = { "lit", "unlit" };
    modeParam.defaultValue = "lit";
    material.params.push_back(modeParam);

    registry.registerTemplate(material);
}