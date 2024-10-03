//
// Created by ssj5v on 03-10-2024.
//

#include "shader.h"
#include <GL/glew.h>
#include "core/utils/logger.h"


MShader::MShader(SString vertProg, SString fragProg, bool compileOnFirstUse) {
    this->vertexShaderSource = vertProg;
    this->fragmentShaderSource = fragProg;
    this->compileOnFirstUse = compileOnFirstUse;
}

GLint MShader::getUniformLocation(const SString& name) {
    return glGetUniformLocation(shaderProgram, &name);
}

bool MShader::getShaderCompilaionStatus(const GLuint& shaderId) {
    GLint success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    return success == GL_TRUE;
}

SString MShader::getShaderInfoLog(const GLuint &shaderId) {
    GLint length = 0;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
    char infoLog[length + 1];
    glGetShaderInfoLog(shaderId, length, 1, infoLog);
    return SString(infoLog);
}

void MShader::compile() {

    if(valid) {
        MWARN(STR("Shader already compiled"));
        return;
    }

    // compile vertex
    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, vertexShaderSource.c_str(), vertexShaderSource.length());
    glCompileShader(vertex);
    if(!getShaderCompilaionStatus(vertex)) {
        MERROR(STR("Error compiling vertex shader: ") + getShaderInfoLog(vertex));
        glDeleteShader(vertex);
        return;
    }

    // compile fragment
    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, fragmentShaderSource.c_str(), fragmentShaderSource.length());
    glCompileShader(fragment);
    if(!getShaderCompilaionStatus(fragment)) {
        MERROR(STR("Error compiling fragment shader: ") + getShaderInfoLog(fragment));
        glDeleteShader(fragment);
        return;
    }

    valid = true;
}

void MShader::bind() {
    if(!valid && compileOnFirstUse) {
        compile();
    }

    // if not valid, don't bind
    if(!valid) {
        MERROR(STR("Shader cannot be bound! Reason: Error during compilation and/or not compiled."));
        return;
    }

    glUseProgram(shaderProgram);
}

void MShader::setUniform1i(const SString &name, int value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniform1i(location, value);
    }
}

void MShader::setUniform1f(const SString &name, float value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniform1f(location, value);
    }
}

void MShader::setUniform2f(const SString &name, const SVector2 &value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniform2f(location, value.x, value.y);
    }
}

void MShader::setUniform3f(const SString &name, const SVector3 &value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniform3f(location, value.x, value.y, value.z);
    }
}

void MShader::setUniform4f(const SString &name, const SVector4 &value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }
}

void MShader::setUniformMat4(const SString &name, const SMatrix4 &value) {
    auto location = getUniformLocation(name);
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name.c_str() << "' doesn't exist or isn't used." << std::endl;
    } else {
        // Set the integer uniform
        glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
    }
}
