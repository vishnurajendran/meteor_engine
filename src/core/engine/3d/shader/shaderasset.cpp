//
// Created by ssj5v on 03-10-2024.
//

#include "shaderasset.h"
#include "shader.h"
#include "pugixml.hpp"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "core/utils/serialisation_utils.h"

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

    pugi::xml_node rootNode = document.child("shader");
    if (!rootNode) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect, missing shader-tree");
        return;
    }

    if (!rootNode.attribute("name")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing shader attrib name");
        return;
    }

    name = rootNode.attribute("name").value();

    if (!rootNode.attribute("version")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing shader attrib version");
        return;
    }

    SString version = rootNode.attribute("version").value();
    SString versionStr = STR("#version ") + version + "\n";

    if (!rootNode.child("vert")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing vertex program tree");
        return;
    }

    SString vertexSource = versionStr + rootNode.child("vert").text().get();

    if (!rootNode.child("frag")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing fragment program tree");
        return;
    }

    SString fragmentSource = versionStr + rootNode.child("frag").text().get();
    std::map<SString, SShaderPropertyValue> properties;

    if (!rootNode.child("properties")) {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect - missing fragment program tree");
        return;
    }

    for (auto child: rootNode.child("properties")) {
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
        SShaderPropertyType type = parsePropertyType(child.attribute("type").value());
        SShaderPropertyValue value;
        if (!child.attribute("default")) {
            MWARN("MShaderAsset::loadShader(): property type missing");
            continue;
        } else {
            parseValue(child.attribute("default").value(), value, type);
        }
        properties[key] = value;
    }
    shader = new MShader(vertexSource, fragmentSource, properties);
    valid = true;
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
    }
}
