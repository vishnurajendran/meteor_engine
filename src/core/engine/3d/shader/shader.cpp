//
// Created by ssj5v on 03-10-2024.
//

#include <GL/glew.h>
#include <utility>
#include "shader.h"

#include "shadercompiler.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"
#include "core/utils/serialisation_utils.h"


void SShaderPropertyValue::print() {

    MLOG("[SShaderPropertyValue]");

    if(type == SShaderPropertyType::Float) {
        MLOG(std::to_string(getFloatVal()));
    }
    else if(type == SShaderPropertyType::Int) {
        MLOG(std::to_string(getIntVal()));
    }
    else if(type == SShaderPropertyType::UniformVec2) {
        MLOG(STR("X: ") + std::to_string(getVec2Val().x) + STR("Y: ") + std::to_string(getVec2Val().y));
    }
    else if(type == SShaderPropertyType::UniformVec3) {
        MLOG(STR("X: ") + std::to_string(getVec3Val().x) + STR("Y: ") + std::to_string(getVec3Val().y) + STR("Z: ") + std::to_string(getVec3Val().z));
    }
    else if(type == SShaderPropertyType::UniformVec4) {
        MLOG(STR("X: ") + std::to_string(getVec4Val().x) + STR("Y: ") + std::to_string(getVec4Val().y) + STR("Z: ") + std::to_string(getVec4Val().z)+ STR("W: ") + std::to_string(getVec4Val().w));
    }
    else if(type == SShaderPropertyType::Matrix4) {
        for(int i = 0; i < 4; i++) {
            SString entry = "";
            for(int j = 0; j < 4; j++) {
                entry += std::to_string(getMat4Val()[i][j]) + "\t";
            }
            MLOG(entry);
        }
    }
}

MShader::MShader(const SString& vertProg, const SString& fragProg, const std::map<SString, SShaderPropertyValue>& properties, const bool& compileOnFirstUse) {
    this->vertexShaderSource = std::move(vertProg);
    this->fragmentShaderSource = std::move(fragProg);
    this->properties = std::move(properties);
    this->compileOnFirstUse = compileOnFirstUse;
    if(!compileOnFirstUse)
        compile();
}

MShader::~MShader() {
    if(compiled) {
        glDeleteProgram(shaderProgram);
    }
}

std::map<SString, SShaderPropertyValue> MShader::getProperties() const {
    return properties;
}

GLint MShader::getUniformLocation(const SString& name) const {
    auto nameBuff = name.c_str();
    return glGetUniformLocation(shaderProgram, nameBuff);
}

void MShader::compile() {
    if(compiled) {
        MWARN(STR("Shader already compiled"));
        return;
    }
    compiled = MShaderCompiler::compileShader(getName(), this->vertexShaderSource, this->fragmentShaderSource, this->shaderProgram);
}

void MShader::bind() {
    if(!compiled && compileOnFirstUse) {
        compile();
    }

    if(!compiled) {
        MERROR(STR("Shader cannot be bound! Reason: Error during compilation and/or not compiled."));
        return;
    }

    glUseProgram(shaderProgram);
}

void MShader::setPropertyValue(const SString &name, const SShaderPropertyValue &value) {
    if(properties.contains(name)) {
        properties[name] = value;
    }

    switch (value.getType()) {
        case Int:
            setUniform1i(name, value.getIntVal());
            break;
        case Float:
            setUniform1f(name, value.getFloatVal());
            break;
        case UniformVec2:
            setUniform2f(name, value.getVec2Val());
            break;
        case UniformVec3:
            setUniform3f(name, value.getVec3Val());
            break;
        case UniformVec4:
            setUniform4f(name, value.getVec4Val());
            break;
        case Matrix4:
            setUniformMat4(name, value.getMat4Val());
            break;
        case Texture:
            setTexture(name, value.getTexAssetReference());
        default:
            break;
    }
}

void MShader::setUniform1i(const SString &name, int value) const {
    auto location = getUniformLocation(name);
    if (location != -1)  {
        glUniform1i(location, value);
    }
}

void MShader::setUniform1f(const SString &name, float value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void MShader::setUniform2f(const SString &name, const SVector2 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, value.x, value.y);
    }
}

void MShader::setUniform3f(const SString &name, const SVector3 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, value.x, value.y, value.z);
    }
}

void MShader::setUniform4f(const SString &name, const SVector4 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
}

void MShader::setUniformMat4(const SString &name, const SMatrix4 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
    }
}

void MShader::setTexture(const SString &name, const SString& textureAssetPath, const unsigned int& index) const {
    auto asset = MAssetManager::getInstance()->getAsset<MTextureAsset>(textureAssetPath);
    if(!asset) {
        MERROR("Shader: Invalid Texture Asset");
        return;
    }
    auto texture = asset->getTexture();
    if(!texture) {
        MERROR("Shader: NULL SHADER");
        return;
    }
    auto location = getUniformLocation(name);
    if (location != -1) {
        texture->bind(location, index);
        glUniform1i(location, static_cast<GLint>(index));
    }
}
