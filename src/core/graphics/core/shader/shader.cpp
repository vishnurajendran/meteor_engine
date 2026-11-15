//
// Created by ssj5v on 03-10-2024.
//

#include <GL/glew.h>
#include <utility>
#include "shader.h"
#include "core/graphics/core/color_space.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"
#include "core/utils/serialisation_utils.h"
#include "default_textures.h"
#include "shadercompiler.h"
#include "core/memory/mnew.h"


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

MShaderBase::MShaderBase(const std::unordered_map<SString, SShaderPropertyValue>& props, bool compileFirstUse)
    : compileOnFirstUse(compileFirstUse), properties(props)
{
    // Default order from the map (arbitrary hash order). MShaderAsset
    // overrides this with the .mesl declaration order via setPropertyOrder().
    for (auto& [key, _] : properties)
        propertyOrder.push_back(key);
}

MShaderBase::~MShaderBase() {
    if(compiled) {
        glDeleteProgram(shaderProgram);
    }
}

std::unordered_map<SString, SShaderPropertyValue> MShaderBase::getProperties() const {
    return properties;
}

bool MShaderBase::ensureCompiled() {
    if (!compiled && compileOnFirstUse) {
        compileOnFirstUse = false;   // one attempt - a broken shader is reported once
        compile();
    }
    return compiled;
}

GLuint MShaderBase::getProgramHandle() {
    return ensureCompiled() ? shaderProgram : 0;
}

GLint MShaderBase::getUniformLocation(const SString& name) const {
    auto nameBuff = name.c_str();
    return glGetUniformLocation(shaderProgram, nameBuff);
}

void MShaderBase::setUniform1ui(const SString &name, unsigned int value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform1ui(location, value);
    }
}

void MShaderBase::setUniform1i(const SString &name, int value) const {
    auto location = getUniformLocation(name);
    if (location != -1)  {
        glUniform1i(location, value);
    }
}

void MShaderBase::setUniform1f(const SString &name, float value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void MShaderBase::setUniform2f(const SString &name, const SVector2 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, value.x, value.y);
    }
}

void MShaderBase::setUniform3f(const SString &name, const SVector3 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, value.x, value.y, value.z);
    }
}

void MShaderBase::setUniform4f(const SString &name, const SVector4 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
}

void MShaderBase::setUniformMat4(const SString &name, const SMatrix4 &value) const {
    auto location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
    }
}
