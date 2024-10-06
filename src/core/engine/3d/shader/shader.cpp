//
// Created by ssj5v on 03-10-2024.
//

#include <GL/glew.h>
#include <utility>
#include "shader.h"
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
    if(valid) {
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

bool MShader::getShaderCompilaionStatus(const GLuint& shaderId) {
    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
}

SString MShader::getShaderInfoLog(const GLuint &shaderId) {
    GLint length = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);

    // Dynamically allocate memory for infoLog
    std::vector<char> infoLog(length + 1);
    glGetShaderInfoLog(shaderId, length, &length, infoLog.data());

    return {infoLog.data()};
}

void MShader::compile() {
    if(valid) {
        MWARN(STR("Shader already compiled"));
        return;
    }

    valid = false;
    // Compile vertex shader
    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    auto vertBuff = vertexShaderSource.c_str();
    GLint vertLength = vertexShaderSource.length();
    glShaderSource(vertex, 1, &vertBuff, &vertLength);
    glCompileShader(vertex);

    if(!getShaderCompilaionStatus(vertex)) {
        MERROR(STR("Error compiling vertex shader: ") + getShaderInfoLog(vertex));
        glDeleteShader(vertex);
        return;
    }

    // Compile fragment shader
    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragBuff = fragmentShaderSource.c_str();
    GLint fragLength = fragmentShaderSource.length();
    glShaderSource(fragment, 1, &fragBuff, &fragLength);
    glCompileShader(fragment);

    if(!getShaderCompilaionStatus(fragment)) {
        MERROR(STR("Error compiling fragment shader: ") + getShaderInfoLog(fragment));
        glDeleteShader(fragment);
        glDeleteShader(vertex);
        return;
    }

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);

    // Check if linking was successful
    GLint programSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &programSuccess);
    if (!programSuccess) {
        MERROR(STR("Error linking shader program"));
        glDeleteProgram(shaderProgram);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return;
    }

    // Clean up shaders (they are no longer needed after linking)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    valid = true;
}

void MShader::bind() {
    if(!valid && compileOnFirstUse) {
        compile();
    }

    if(!valid) {
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
        default:
            break;
    }
}

void MShader::setUniform1i(const SString &name, int value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniform1i(location, value);
    }
}

void MShader::setUniform1f(const SString &name, float value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniform1f(location, value);
    }
}

void MShader::setUniform2f(const SString &name, const SVector2 &value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniform2f(location, value.x, value.y);
    }
}

void MShader::setUniform3f(const SString &name, const SVector3 &value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniform3f(location, value.x, value.y, value.z);
    }
}

void MShader::setUniform4f(const SString &name, const SVector4 &value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
}

void MShader::setUniformMat4(const SString &name, const SMatrix4 &value) const {
    auto location = getUniformLocation(name);
    if (location == -1) {
        MWARN(STR("Uniform '") + name + STR("' doesn't exist or isn't used."));
    } else {
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
    }
}
