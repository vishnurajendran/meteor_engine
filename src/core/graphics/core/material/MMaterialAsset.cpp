//
// Created by ssj5v on 05-10-2024.
//

#include "MMaterialAsset.h"

#include <sstream>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
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

    auto* shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>(sp.c_str());
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
        // Update in place - do NOT delete and recreate.
        // getMaterial() returns original directly; any inspector panel, render
        // item, or thumbnail renderer holding that pointer must remain valid.
        // Updating the internal shader and properties keeps all existing
        // references alive while reflecting the new file contents.
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
        MERROR("MMaterialAsset::loadFromFile — cannot read " + path);
        return false;
    }

    pugi::xml_document doc;
    if (doc.load_string(data.c_str()).status != pugi::status_ok)
    {
        MERROR("MMaterialAsset::loadFromFile — XML parse failed: " + path);
        return false;
    }

    auto root = doc.child("material");
    if (!root) { MERROR("MMaterialAsset::loadFromFile — missing <material>: " + path); return false; }

    if (root.attribute("name"))
        name = root.attribute("name").value();

    // New format: <shaderPathField>path</shaderPathField>
    // Old format: <shader path="..."/>
    if (auto n = root.child("shaderPathField"); n)
        shaderPathField = std::string(n.text().as_string());
    else if (root.child("shader") && root.child("shader").attribute("path"))
        shaderPathField = std::string(root.child("shader").attribute("path").value());

    // New format: <shadingModeStr>lit</shadingModeStr>
    // Old format: mode="lit" attribute on <material>
    if (auto n = root.child("shadingModeStr"); n)
        shadingModeStr = std::string(n.text().as_string());
    else if (root.attribute("mode"))
        shadingModeStr = std::string(root.attribute("mode").value());
    else
        shadingModeStr = std::string("lit");

    syncFromFields();

    // Load per-property overrides from <properties> block — kept as a raw
    // map because property count/types vary per shader.
    properties.clear();
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
    }

    return true;
}

bool MMaterialAsset::save()
{
    if (!original)
    {
        MWARN("MMaterialAsset::save — material not built yet");
        return false;
    }

    // Sync enum → field string before saving.
    shadingModeStr = std::string(shadingMode == MMaterial::ShadingMode::Lit ? "lit" : "unlit");

    pugi::xml_document doc;
    auto root = doc.append_child("material");
    root.append_attribute("name").set_value(name.c_str());

    // Write fields directly — avoids calling serialiseToNode from a context
    // where the SerializedClassBase framework may interact with MAsset internals.
    root.append_child("shaderPathField").text().set(shaderPathField.get().c_str());
    root.append_child("shadingModeStr").text().set(shadingModeStr.get().c_str());

    // Write current property values.
    auto propsNode = root.append_child("properties");
    for (auto& [key, val] : original->getProperties())
    {
        // Store as SString so the lifetime covers all three set_value calls.
        // const char* ts = getTypeStr(...).c_str() would dangle immediately.
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
        MERROR("MMaterialAsset::save — failed to write " + path);
        return false;
    }

    MLOG("MMaterialAsset::save — saved " + path);
    return true;
}

bool MMaterialAsset::createNewMaterial(const SString& directory,
                                        const SString& materialName,
                                        const SString& shaderPath,
                                        MMaterial::ShadingMode mode)
{
    // Build the file path: directory/materialName.material
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

    // Write as DECLARE_FIELD child nodes (matching the format save() produces).
    root.append_child("shaderPathField").text().set(shaderPath.c_str());
    root.append_child("shadingModeStr").text().set(modeStr);


    MLOG(SString::format("Copying Shader Properties {0}", shaderPath));
    auto shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>(shaderPath);
    if (shaderAsset == nullptr)
    {
        MERROR(SString::format("Invalid Shader Asset {0}", shaderPath));
        return false;
    }
    auto shaderInstance = shaderAsset->getShader();
    if (shaderAsset == nullptr)
    {
        MERROR(SString::format("Invalid Shader {0}", shaderPath));
        return false;
    }

    auto properties = root.append_child("properties");
    for (auto property : shaderInstance->getProperties())
    {
        auto child = properties.append_child("property");
        child.append_attribute("key").set_value(property.first.c_str());
        child.append_attribute("type").set_value(MShaderUtility::getTypeStr(property.second.getType()).c_str());
        child.append_attribute("value").set_value(MShaderUtility::getValueStr(property.second).c_str());
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