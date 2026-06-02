//
// Created by ssj5v on 05-10-2024.
//

#include "MMaterialAsset.h"

#include <algorithm>
#include <sstream>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/shader/shader_utils.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "material.h"
#include "pugixml.hpp"

MMaterialAsset::MMaterialAsset(const SString& path) : MAsset(path)
{
    valid = loadFromFile(path);
}

MMaterialAsset::~MMaterialAsset()
{
    delete original;
}

void MMaterialAsset::syncFromFields()
{
    shadingMode = (shadingModeStr.get() == "lit") ? MMaterial::ShadingMode::Lit : MMaterial::ShadingMode::Unlit;
}

MMaterial* MMaterialAsset::getMaterial()
{
    if (!original && valid)
        buildMaterialAsset();
    return original;
}

void MMaterialAsset::buildMaterialAsset()
{
    const std::string& sp = shaderPathField.get();
    if (sp.empty())
    {
        MERROR("MMaterialAsset::buildMaterialAsset(): shaderPath field is empty");
        return;
    }

    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MShaderAsset>(sp.c_str());
    if (!shaderAsset)
    {
        MERROR("MMaterialAsset::buildMaterialAsset(): shader asset not found: " + SString(sp.c_str()));
        return;
    }

    MShader* shader = shaderAsset->getShader();
    if (!shader)
    {
        MERROR("MMaterialAsset::buildMaterialAsset(): shader compiled with errors: " + SString(sp.c_str()));
        return;
    }

    if (original)
    {
        original->updateShader(shader, shadingMode);
        for (auto& [k, v] : properties)
            original->setProperty(k, v);
    }
    else
    {
        original = new MMaterial(shader, shadingMode);
        for (auto& [k, v] : properties)
            original->setProperty(k, v);
    }

    // Apply the XML file's property order to the material so the inspector
    // displays properties in the order the user authored them.
    // Any shader properties not present in the file are appended at the end.
    std::vector<SString> finalOrder = loadOrder;
    for (const auto& [key, _] : original->getProperties())
    {
        if (std::find(finalOrder.begin(), finalOrder.end(), key) == finalOrder.end())
            finalOrder.push_back(key);
    }
    original->setPropertyOrder(finalOrder);
}

void MMaterialAsset::deferredAssetLoad(bool forced)
{
    if (original && !forced) return;
    buildMaterialAsset();
}

bool MMaterialAsset::loadFromFile(const SString& path)
{
    SString data;
    if (!FileIO::readFile(path, data))
    {
        MERROR("MMaterialAsset::loadFromFile -- cannot read " + path);
        return false;
    }

    pugi::xml_document doc;
    if (doc.load_string(data.c_str()).status != pugi::status_ok)
    {
        MERROR("MMaterialAsset::loadFromFile -- XML parse failed: " + path);
        return false;
    }

    auto root = doc.child("material");
    if (!root) { MERROR("MMaterialAsset::loadFromFile -- missing <material>: " + path); return false; }

    if (root.attribute("name"))
        name = root.attribute("name").value();

    if (auto n = root.child("shaderPathField"); n)
        shaderPathField = std::string(n.text().as_string());
    else if (root.child("shader") && root.child("shader").attribute("path"))
        shaderPathField = std::string(root.child("shader").attribute("path").value());

    if (auto n = root.child("shadingModeStr"); n)
        shadingModeStr = std::string(n.text().as_string());
    else if (root.attribute("mode"))
        shadingModeStr = std::string(root.attribute("mode").value());
    else
        shadingModeStr = std::string("lit");

    syncFromFields();

    properties.clear();
    loadOrder.clear();

    if (!root.child("properties")) return true;

    for (auto child : root.child("properties"))
    {
        if (SString(child.name()) != "property") continue;
        if (!child.attribute("key") || !child.attribute("type") || !child.attribute("value")) continue;

        SString key = child.attribute("key").value();
        if (properties.contains(key)) continue;

        auto type = MShaderUtility::parsePropertyType(child.attribute("type").value());
        SShaderPropertyValue val;
        MShaderUtility::parseValue(child.attribute("value").value(), val, type);
        properties[key] = val;
        loadOrder.push_back(key);
    }

    return true;
}

bool MMaterialAsset::save()
{
    if (!original)
    {
        MWARN("MMaterialAsset::save -- material not built yet");
        return false;
    }

    shadingModeStr = std::string(shadingMode == MMaterial::ShadingMode::Lit ? "lit" : "unlit");

    pugi::xml_document doc;
    auto root = doc.append_child("material");
    root.append_attribute("name").set_value(name.c_str());

    root.append_child("shaderPathField").text().set(shaderPathField.get().c_str());
    root.append_child("shadingModeStr").text().set(shadingModeStr.get().c_str());

    // Write properties in the material's display order so the saved file
    // preserves the authored sequence.
    auto propsNode = root.append_child("properties");
    const auto& props = original->getProperties();
    for (const auto& key : original->getPropertyOrder())
    {
        auto it = props.find(key);
        if (it == props.end()) continue;
        const auto& val = it->second;

        SString ts  = MShaderUtility::getTypeStr(val.getType());
        SString vls = MShaderUtility::getValueStr(val);
        if (ts.empty()) continue;

        auto prop = propsNode.append_child("property");
        prop.append_attribute("key").set_value(key.c_str());
        prop.append_attribute("type").set_value(ts.c_str());
        prop.append_attribute("value").set_value(vls.c_str());
    }

    std::ostringstream oss;
    doc.save(oss);

    auto data = SString(oss.str().c_str());
    if (!FileIO::writeFile(path, data))
    {
        MERROR("MMaterialAsset::save -- failed to write " + path);
        return false;
    }

    // Update loadOrder to match what was just written so subsequent
    // builds keep the same order without re-reading the file.
    loadOrder = original->getPropertyOrder();

    MLOG("MMaterialAsset::save -- saved " + path);
    return true;
}

bool MMaterialAsset::createNewMaterial(const SString& directory,
                                        const SString& materialName,
                                        const SString& shaderPath,
                                        MMaterial::ShadingMode mode)
{
    SString filePath = directory;
    if (!filePath.empty() && filePath.str().back() != '/')
        filePath += "/";
    filePath += materialName + ".material";

    if (FileIO::hasFile(filePath))
    {
        MWARN("MMaterialAsset::createNewMaterial - file already exists: " + filePath);
        return false;
    }

    const char* modeStr = (mode == MMaterial::ShadingMode::Lit) ? "lit" : "unlit";

    pugi::xml_document doc;
    auto root = doc.append_child("material");
    root.append_attribute("name").set_value(materialName.c_str());

    root.append_child("shaderPathField").text().set(shaderPath.c_str());
    root.append_child("shadingModeStr").text().set(modeStr);

    MLOG(SString::format("Creating material with shader {0}", shaderPath));
    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAsset<MShaderAsset>(shaderPath);
    if (!shaderAsset)
    {
        MERROR(SString::format("Invalid Shader Asset {0}", shaderPath));
        return false;
    }
    if (!shaderAsset->getShader())
    {
        MERROR(SString::format("Invalid Shader {0}", shaderPath));
        return false;
    }

    // Write properties with safe defaults for each type.
    // The shader's getProperties() returns mutable state contaminated by
    // whichever material was last bound during rendering -- getValueStr()
    // on those values can produce strings that parseValue() cannot parse
    // (e.g. empty strings passed to stoi).  We write the keys and types
    // from the shader but use known-safe default values.
    auto xmlProps = root.append_child("properties");
    for (auto& [key, val] : shaderAsset->getShader()->getProperties())
    {
        const char* defaultValue = "";
        switch (val.getType())
        {
            case SShaderPropertyType::Float:       defaultValue = "0";             break;
            case SShaderPropertyType::Int:         defaultValue = "0";             break;
            case SShaderPropertyType::Bool:        defaultValue = "False";         break;
            case SShaderPropertyType::UniformVec2: defaultValue = "(0,0)";         break;
            case SShaderPropertyType::UniformVec3: defaultValue = "(0,0,0)";       break;
            case SShaderPropertyType::UniformVec4: defaultValue = "(0,0,0,1)";     break;
            case SShaderPropertyType::Color:       defaultValue = "(1,1,1,1)";     break;
            case SShaderPropertyType::Texture:     defaultValue = "";              break;
            default:                               defaultValue = "0";             break;
        }
        
        auto child = xmlProps.append_child("property");
        child.append_attribute("key").set_value(key.c_str());
        child.append_attribute("type").set_value(
            MShaderUtility::getTypeStr(val.getType()).c_str());
        child.append_attribute("value").set_value(defaultValue);
    }

    std::ostringstream oss;
    doc.save(oss);

    auto data = SString(oss.str().c_str());
    if (!FileIO::writeFile(filePath, data))
    {
        MERROR("MMaterialAsset::createNewMaterial - failed to write " + filePath);
        return false;
    }

    MLOG("MMaterialAsset::createNewMaterial - created " + filePath);
    return true;
}