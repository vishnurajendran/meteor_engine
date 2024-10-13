//
// Created by ssj5v on 03-10-2024.
//

#include "shaderasset.h"
#include "shader.h"
#include "pugixml.hpp"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "core/utils/serialisation_utils.h"

const SString MShaderAsset::SHDR_ROOTNODE = "shader";
const SString MShaderAsset::SHDR_VERTNODE = "vert";
const SString MShaderAsset::SHDR_FRAGNODE = "frag";
const SString MShaderAsset::SHDR_INCLNODE = "include";
const SString MShaderAsset::SHDR_INCL_ATTRIB_PATH="header";
const SString MShaderAsset::SHDR_INCL_ATTRIB_INTERNAL="internal";
const SString MShaderAsset::SHDR_PROPERTY_PARENT_NODE = "properties";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_NODE = "property";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_KEY = "key";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_TYPE = "type";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_VALUE = "default";

void MShaderAsset::loadShader(const SString &path) {
    valid = false;
    SString data;
    auto fileReadRes = FileIO::readFile(path, data);
    if (!fileReadRes) {
        MERROR("MShaderAsset::loadShader(): Failed to read file");
        return;
    };
    pugi::xml_document document;
    auto parseRes = document.load_string(data.c_str(), pugi::parse_default);
    if (parseRes.status != pugi::status_ok) {
        MERROR(STR("MShaderAsset::loadShader(): Failed to parse shader file - ") + parseRes.description() + "\n");
        return;
    };

    pugi::xml_node rootNode = document.child(SHDR_ROOTNODE.c_str());
    if (!rootNode) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect, missing shader-tree");
        return;
    }

    auto nameAndVersiontring = getShaderNameAndVersion(rootNode);
    name = nameAndVersiontring.first;
    auto versionStr = nameAndVersiontring.second;

    if (!rootNode.child(SHDR_VERTNODE.c_str())) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing vertex program tree");
        return;
    }
    SString vertPreReq = getPrequisiteCode(rootNode.child(SHDR_VERTNODE.c_str()));
    SString vertexSource = versionStr + vertPreReq+ rootNode.child(SHDR_VERTNODE.c_str()).text().get();
    if (!rootNode.child(SHDR_FRAGNODE.c_str())) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing fragment program tree");
        return;
    }

    const SString fragPreReq = getPrequisiteCode(rootNode.child(SHDR_FRAGNODE.c_str()));
    const SString fragmentSource = versionStr + fragPreReq + rootNode.child(SHDR_FRAGNODE.c_str()).text().get();

    const auto properties = getShaderProperties(rootNode);
    shader = new MShader(vertexSource, fragmentSource, properties);
    valid = true;
}

SString MShaderAsset::getPrequisiteCode(pugi::xml_node node) {
    SString prequisiteCode="";
    for(auto child : node.children()) {
        if(child.name() != SHDR_INCLNODE.str()) {
            continue;
        }

        if(!child.attribute(SHDR_INCL_ATTRIB_PATH.c_str()))
            continue;

        auto fileName = STR(child.attribute(SHDR_INCL_ATTRIB_PATH.c_str()).value());

        if(child.attribute(SHDR_INCL_ATTRIB_INTERNAL.c_str())) {
            auto internalSrc = STR(child.attribute(SHDR_INCL_ATTRIB_INTERNAL.c_str()).value()) == "1";
            fileName = internalSrc ? STR("meteor_assets/shader_utils/")+fileName : fileName;
        }

        if (fileName.empty()) {
            MERROR("MShaderAsset:loadShader(): shader file structure incorrect - missing include name");
            return "";
        }
        SString fileContents="";
        if(FileIO::readFile(fileName, fileContents)) {
            prequisiteCode += fileContents + STR("\n");
        }
        else {
            MERROR(STR("MShaderAsset:loadShader(): Failed to read file ") + fileName + "\n");
            return "";
        }
    }
    return prequisiteCode;
}

std::map<SString, SShaderPropertyValue> MShaderAsset::getShaderProperties(pugi::xml_node node) {
    std::map<SString, SShaderPropertyValue> properties;
    if (!node.child(SHDR_PROPERTY_PARENT_NODE.c_str())) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing fragment program tree");
        return properties;
    }

    for (auto child: node.child(SHDR_PROPERTY_PARENT_NODE.c_str())) {
        if (STR(child.name()) != SHDR_PROPERTY_CHILD_NODE.c_str()) {
            MWARN("MShaderAsset:loadShader(): shader file structure incorrect - missing property name");
            continue;
        }

        if (!child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_KEY.c_str())) {
            MWARN("MShaderAsset::loadShader(): property key missing");
            continue;
        }
        SString key = child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_KEY.c_str()).value();
        if (properties.contains(key)) {
            MWARN("MShaderAsset::loadShader(): property value already exists");
            continue;
        }

        if (!child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_TYPE.c_str())) {
            MWARN("MShaderAsset::loadShader(): property type missing");
            continue;
        }
        SShaderPropertyType type = parsePropertyType(child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_TYPE.c_str()).value());
        SShaderPropertyValue value;
        if (!child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_VALUE.c_str())) {
            MWARN("MShaderAsset::loadShader(): property type missing");
            continue;
        } else {
            parseValue(child.attribute(SHDR_PROPERTY_CHILD_ATTRIB_VALUE.c_str()).value(), value, type);
        }
        properties[key] = value;
    }
    return properties;
}

std::pair<SString, SString> MShaderAsset::getShaderNameAndVersion(pugi::xml_node node) {

    if (!node.attribute("name")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing shader attrib name");
        return std::make_pair("", "");
    }
    auto name = STR(node.attribute("name").value());
    if (!node.attribute("version")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing shader attrib version");
        return std::make_pair("", "");
    }
    auto version = STR(node.attribute("version").value());
    auto versionStr = STR("#version ") + version + "\n";
    return std::make_pair(name, versionStr);
}

MShaderAsset::MShaderAsset(const SString &path) : MAsset(path) {
    name = STR("ShaderAsset");
    loadShader(path);
}

MShaderAsset::~MShaderAsset() {
    if (valid) {
        delete shader;
    }
}

MShader *MShaderAsset::getShader() const {
    return shader;
}

SShaderPropertyType MShaderAsset::parsePropertyType(const SString &str) {
    if (str == "i") return Int;
    if (str == "f") return Float;
    if (str == "u2") return UniformVec2;
    if (str == "u3") return UniformVec3;
    if (str == "u4") return UniformVec4;
    if (str == "m4") return Matrix4;
    if (str == "tex2D") return Texture2D;
    return NoVal;
}

void MShaderAsset::parseValue(const SString &str, SShaderPropertyValue &value, const SShaderPropertyType &type) {
    switch (type) {
        case NoVal: // no value in this property
        case Matrix4: //material serialisation not supported
            break;
        case Int:
            value.setIntVal(std::stoi(str));
            break;

        case Float:
            value.setFloatVal(std::stof(str));
            break;

        case UniformVec2: {
            SVector2 vec2 = SVector2(0);
            if (parseVector2(str, vec2)) {
                value.setVec2Val(vec2);
            }
        }
            break;

        case UniformVec3: {
            SVector3 vec3 = SVector3(0);
            if (parseVector3(str, vec3)) {
                value.setVec3Val(vec3);
            }
        }
            break;

        case UniformVec4: {
            SVector4 vec4 = SVector4(0);
            if (parseVector4(str, vec4)) {
                value.setVec4Val(vec4);
            }
        }
            break;

        case Texture2D: {
           value.setTextureReference(str);
        }
        break;
    }
}
