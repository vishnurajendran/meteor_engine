//
// Created by ssj5v on 05-10-2024.
//

#include "MMaterialAsset.h"

#include "material.h"
#include "pugixml.hpp"
#include "core/engine/3d/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MMaterialAsset::MMaterialAsset(const SString &path) : MAsset(path){
    valid = loadFromFile(path);
}

MMaterialAsset::~MMaterialAsset() {
    if(original)
        delete original;
}

MMaterial * MMaterialAsset::getInstance() {
    if(!original && valid) {
        // if material is not, built, and we have a valid asset, Build it now
        // there is some dependency with this asset.
        buildMaterialAsset();
    }

    return original->clone();
}

void MMaterialAsset::buildMaterialAsset() {

    auto shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>(shaderPath);
    if(!shaderAsset) {
        MERROR("MMaterialAsset::buildMaterialAsset(): shaderasset not found");
        return;
    }
    original = new MMaterial(shaderAsset->getShader());
    for(auto kv : properties) {
        original->setProperty(kv.first, kv.second);
    }
    MLOG("MMaterialAsset::buildMaterialAsset(): built material asset");
}


void MMaterialAsset::defferedAssetLoad(bool forced) {
    // if not forced, don't rebuild
    if(original && !forced)
        return;
    buildMaterialAsset();
}

bool MMaterialAsset::loadFromFile(const SString &path) {
    SString data;
    if(!FileIO::readFile(path, data)) {
        MERROR("MMaterialAsset::Error reading file");
        return false;
    }
    auto doc = pugi::xml_document();
    auto res = doc.load_string(data.c_str());
    if(res.status != pugi::status_ok) {
        MERROR(STR("MMaterialAsset::Failed parsing file: ") + res.description());
        return false;
    }
    auto root = doc.child("material");
    if(!root) {
        MERROR("MMaterialAsset::Failed parsing root");
    }

    if(root.attribute("name"))
        name = root.attribute("name").value();

    if(!root.child("shader") || !root.child("shader").attribute("path")) {
       MERROR("MMaterialAsset::Failed parsing shader");
        return false;
    }
    shaderPath = root.child("shader").attribute("path").value();

    if(!root.child("properties"))
        return true;

    for(auto child : root.child("properties")) {
        if (STR(child.name()) != "property") {
            MWARN("MShaderAsset:loadShader(): shader file structure incorrect - missing property name");
            continue;
        }

        if (!child.attribute("key")) {
            MWARN("MShaderAsset::loadShader(): property key missing");
            continue;
        }
        SString key = child.attribute("key").value();
        if (properties.contains(key)) {
            MWARN("MShaderAsset::loadShader(): property value already exists");
            continue;
        }

        if (!child.attribute("type")) {
            MWARN("MShaderAsset::loadShader(): property type missing");
            continue;
        }
        SShaderPropertyType type = MShaderAsset::parsePropertyType(child.attribute("type").value());
        SShaderPropertyValue value;
        if (!child.attribute("value")) {
            MWARN("MShaderAsset::loadShader(): property type missing");
            continue;
        } else {
            MShaderAsset::parseValue(child.attribute("value").value(), value, type);
        }
        properties[key] = value;
    }

    return true;
}
