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
const SString MShaderAsset::SHDR_ATTRIB_BASE_SOURCE = "base";
const SString MShaderAsset::SHDR_PROPERTY_PARENT_NODE = "properties";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_NODE = "property";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_KEY = "key";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_TYPE = "type";
const SString MShaderAsset::SHDR_PROPERTY_CHILD_ATTRIB_VALUE = "default";
const SString MShaderAsset::SHDR_VERT_PROGRAM_BEGIN = "#VERTEX_PASS";
const SString MShaderAsset::SHDR_VERT_PROGRAM_END = "#END_VERTEX_PASS";
const SString MShaderAsset::SHDR_FRAG_PROGRAM_BEGIN = "#FRAGMENT_PASS";
const SString MShaderAsset::SHDR_FRAG_PROGRAM_END = "#END_FRAGMENT_PASS";
const SString MShaderAsset::SHDR_CDATA_BEGIN = "<![CDATA[";
const SString MShaderAsset::SHDR_CDATA_END = "]]>";

const SString MShaderAsset::SHDR_VERTNODE = "vert";
const SString MShaderAsset::SHDR_FRAGNODE= "frag";

const SString MShaderAsset::SHDR_EXTENSIONS = "#extension GL_ARB_shading_language_include : require\n"
                                              "#extension GL_NV_uniform_buffer_std430_layout : require\n";
const SString MShaderAsset::SHDR_DEFINE_COMPILE_VERT =  "#define COMPILE_VERTEX\n";
const SString MShaderAsset::SHDR_DEFINE_COMPILE_FRAG =  "#define COMPILE_FRAGMENT\n";

const SString MShaderAsset::SHDR_FALLBACK_MISSING_VERT_PASS = "#NO_VERTEX_PASS_DEFINED\n";
const SString MShaderAsset::SHDR_FALLBACK_MISSING_FRAG_PASS = "#NO_FRAGMENT_PASS_DEFINED\n";

void MShaderAsset::loadShader(const SString& path)
{
    valid = false;
    SString data;
    auto fileReadRes = FileIO::readFile(path, data);
    if (!fileReadRes)
    {
        MERROR("MShaderAsset::loadShader(): Failed to read file");
        return;
    };

    bool hasVertPass = data.find(SHDR_VERT_PROGRAM_BEGIN) != std::string::npos &&
        data.find(SHDR_VERT_PROGRAM_END) != std::string::npos;

    bool hasFragPass = data.find(SHDR_FRAG_PROGRAM_BEGIN) != std::string::npos &&
        data.find(SHDR_FRAG_PROGRAM_END) != std::string::npos;

    // replace files correctly
    data.replace(SHDR_VERT_PROGRAM_BEGIN, STR("<" + SHDR_VERTNODE.str() + ">") + SHDR_CDATA_BEGIN);
    data.replace(SHDR_FRAG_PROGRAM_BEGIN, STR("<" + SHDR_FRAGNODE.str() + ">") + SHDR_CDATA_BEGIN);
    data.replace(SHDR_VERT_PROGRAM_END, SHDR_CDATA_END + STR("</" + SHDR_VERTNODE.str() + ">"));
    data.replace(SHDR_FRAG_PROGRAM_END, SHDR_CDATA_END + STR("</" + SHDR_FRAGNODE.str() + ">"));

    pugi::xml_document document;
    auto parseRes = document.load_string(data.c_str(), pugi::parse_default);
    if (parseRes.status != pugi::status_ok)
    {
        MERROR(STR("MShaderAsset::loadShader(): Failed to parse shader file - ") + parseRes.description() + "\n");
        return;
    };

    pugi::xml_node rootNode = document.child(SHDR_ROOTNODE.c_str());
    if (!rootNode)
    {
        MERROR("MShaderAsset::loadShader(): shader file structure incorrect, missing shader-tree");
        return;
    }

    auto baseSourcePath = STR(rootNode.attribute(SHDR_ATTRIB_BASE_SOURCE.c_str()).value());

    bool isSubShader = true;
    if (baseSourcePath.empty())
    {
        //Shader is not using a base shader file, compile as if it is an independent shader
        isSubShader = false;
    }

    if (isSubShader)
    {
        auto baseSource = STR("");
        FileIO::readFile(baseSourcePath, baseSource);
        valid = loadAsSubShader(rootNode, baseSource, hasVertPass, hasFragPass);
    }
    else
    {
        valid = loadAsIndependantShader(rootNode, hasVertPass, hasFragPass);
    }
}

bool MShaderAsset::loadAsSubShader(const pugi::xml_node& rootNode, const SString& baseSource, const bool& hasVertPass,
                                   const bool& hasFragPass)
{
    auto nameAndVersiontring = getShaderNameAndVersion(rootNode);
    name = nameAndVersiontring.first;
    auto versionStr = nameAndVersiontring.second + "\n";
    auto extension = SHDR_EXTENSIONS;

    SString vertexSource = versionStr + extension + SHDR_DEFINE_COMPILE_VERT + baseSource;
    SString fragmentSource = versionStr + extension + SHDR_DEFINE_COMPILE_FRAG + baseSource;

    if (!hasVertPass)
    {
        vertexSource += SHDR_FALLBACK_MISSING_VERT_PASS;
    }
    else
        vertexSource += rootNode.child(SHDR_VERTNODE.c_str()).text().get();

    if (!hasFragPass)
    {
        fragmentSource += SHDR_FALLBACK_MISSING_FRAG_PASS;
    }
    else
        fragmentSource += rootNode.child(SHDR_FRAGNODE.c_str()).text().get();

    const auto properties = getShaderProperties(rootNode);
    MLOG(STR("MShaderAsset::Creating Shader Object (") + name + ")");
    shader = new MShader(vertexSource, fragmentSource, properties);
    shader->setName(name);
    return true;
}

bool MShaderAsset::loadAsIndependantShader(const pugi::xml_node& rootNode, const bool& hasVertPass,const bool& hasFragPass)
{
    auto nameAndVersiontring = getShaderNameAndVersion(rootNode);
    name = nameAndVersiontring.first;
    auto versionStr = nameAndVersiontring.second + "\n";
    auto extension = SHDR_EXTENSIONS;

    SString vertexSource = versionStr + extension + SHDR_DEFINE_COMPILE_VERT;
    SString fragmentSource = versionStr + extension + SHDR_DEFINE_COMPILE_FRAG;

    if (!hasVertPass)
    {
        MERROR("MShaderAsset::loadAsIndependantShader(): shader file structure incorrect, Vertex Program Missing");
        return false;
    }

    if (!hasFragPass)
    {
        MERROR("MShaderAsset::loadAsIndependantShader(): shader file structure incorrect, Fragment Program Missing");
        return false;
    }

    vertexSource += rootNode.child(SHDR_VERTNODE.c_str()).text().get();
    fragmentSource += rootNode.child(SHDR_FRAGNODE.c_str()).text().get();
    const auto properties = getShaderProperties(rootNode);
    MLOG(STR("MShaderAsset::Creating Shader Object (") + name + ")");
    shader = new MShader(vertexSource, fragmentSource, properties);
    shader->setName(name);
    return true;
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
    if (str == "tex2D") return Texture;
    if (str == "tex") return Texture;
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

        case Texture: {
           value.setTextureReference(str);
        }
        break;
    }
}
