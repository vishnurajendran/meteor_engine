//
// Created by ssj5v on 05-10-2024.
//

#include "MMaterialAsset.h"

#include <algorithm>
#include <sstream>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/shader/shader_utils.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "material.h"
#include "pugixml.hpp"
#include "core/memory/mnew.h"

MMaterialAsset::MMaterialAsset(const SString& path) : MAsset(path)
{
    valid = loadFromSource();
}

MMaterialAsset::~MMaterialAsset()
{
    delete original;
}

void MMaterialAsset::syncFromFields()
{
    shadingMode = (shadingModeStr.get() == "lit") ? MMaterial::ShadingMode::Lit : MMaterial::ShadingMode::Unlit;
}

void MMaterialAsset::applyRenderSettings(MMaterial* material) const
{
    if (!material) return;
    material->setRenderMode(MMaterial::renderModeFromString(renderModeStr.get()));
    material->setBlendMode (MMaterial::blendModeFromString (blendModeStr.get()));
    material->setDoubleSided(doubleSided.get());
    material->setWritesDepth(writeDepth.get());
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

    // "guid:<id>" or a bare path.
    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAssetFromReference<MShaderAsset>(sp.c_str());
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
        original = mnew<MMaterial>(shader, shadingMode);
        for (auto& [k, v] : properties)
            original->setProperty(k, v);
    }
    applyRenderSettings(original);

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

bool MMaterialAsset::dependsOn(const SString& assetPath) const
{
    // The shader may be referenced by GUID, so compare against the resolved
    // asset's path. Only runs on hot-reload, so the extra lookup is fine.
    const SString ref = getShaderPath();
    if (ref == assetPath)
        return true;

    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    if (!assetManager) return false;

    const auto shaderAsset = assetManager->getAssetFromReference<MAsset>(ref);
    return shaderAsset && shaderAsset->getPath() == assetPath;
}

void MMaterialAsset::deferredAssetLoad(bool forced)
{
    if (original && !forced) return;
    buildMaterialAsset();
}

bool MMaterialAsset::loadFromSource()
{
    std::vector<uint8_t> bytes;
    if (!MAssetSources::getActive()->readBytes(path, bytes))
    {
        MERROR("MMaterialAsset::loadFromSource -- cannot read " + path);
        return false;
    }

    pugi::xml_document doc;
    if (doc.load_buffer(bytes.data(), bytes.size()).status != pugi::status_ok)
    {
        MERROR("MMaterialAsset::loadFromSource -- XML parse failed: " + path);
        return false;
    }

    auto root = doc.child("material");
    if (!root) { MERROR("MMaterialAsset::loadFromSource -- missing <material>: " + path); return false; }

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

    // Older files have none of these - they load as opaque, alpha, one-sided.
    if (auto n = root.child("renderModeStr"); n) renderModeStr = std::string(n.text().as_string());
    else                                          renderModeStr = std::string("opaque");
    if (auto n = root.child("blendModeStr"); n)  blendModeStr  = std::string(n.text().as_string());
    else                                          blendModeStr  = std::string("alpha");
    if (auto n = root.child("doubleSided"); n)   doubleSided   = n.text().as_bool();
    else                                          doubleSided   = false;
    if (auto n = root.child("writeDepth"); n)    writeDepth    = n.text().as_bool();
    else                                          writeDepth    = false;

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

        // Skip non-texture properties with empty values -- parseValue would
        // pass the empty string to stoi/stof and crash.  This guards against
        // material files written with missing or cleared values.
        SString valueStr = child.attribute("value").value();
        if (valueStr.empty() && type != SShaderPropertyType::Texture)
            continue;

        SShaderPropertyValue val;
        MShaderUtility::parseValue(valueStr.c_str(), val, type);
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

    // The inspector edits the live material, so take the render settings
    // from it rather than from what was last loaded.
    shadingMode    = original->getShadingMode();
    shadingModeStr = std::string(shadingMode == MMaterial::ShadingMode::Lit ? "lit" : "unlit");
    renderModeStr  = std::string(MMaterial::renderModeToString(original->getRenderMode()));
    blendModeStr   = std::string(MMaterial::blendModeToString(original->getBlendMode()));
    doubleSided    = original->isDoubleSided();
    writeDepth     = original->getWritesDepth();

    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();

    // Upgrade the shader reference to "guid:<id>". Unresolvable references are
    // kept unchanged so a missing shader isn't silently erased.
    if (assetManager)
        shaderPathField = assetManager->toCanonicalReference<MShaderAsset>(getShaderPath()).str();

    pugi::xml_document doc;
    auto root = doc.append_child("material");
    root.append_attribute("name").set_value(name.c_str());

    root.append_child("shaderPathField").text().set(shaderPathField.get().c_str());
    root.append_child("shadingModeStr").text().set(shadingModeStr.get().c_str());
    root.append_child("renderModeStr").text().set(renderModeStr.get().c_str());
    root.append_child("blendModeStr").text().set(blendModeStr.get().c_str());
    root.append_child("doubleSided").text().set(doubleSided.get());
    root.append_child("writeDepth").text().set(writeDepth.get());

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

        // Texture values are asset references — write them as "guid:<id>".
        if (val.getType() == SShaderPropertyType::Texture && !vls.empty() && assetManager)
            vls = assetManager->toCanonicalReference<MTextureAsset>(vls);

        auto prop = propsNode.append_child("property");
        prop.append_attribute("key").set_value(key.c_str());
        prop.append_attribute("type").set_value(ts.c_str());
        prop.append_attribute("value").set_value(vls.c_str());
    }

    auto target = MAssetSources::getWritable();
    if (!target)
    {
        MERROR("MMaterialAsset::save -- active asset source is read-only: " + path);
        return false;
    }

    std::ostringstream oss;
    doc.save(oss);

    if (!target->writeText(path, SString(oss.str())))
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
