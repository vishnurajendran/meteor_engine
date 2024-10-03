//
// Created by ssj5v on 03-10-2024.
//

#ifndef SHADER_H
#define SHADER_H
#include "SFML/OpenGL.hpp"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"


class MShader : MObject {
private:
    GLuint shaderProgram;
    GLint getUniformLocation(const SString& name);
    SString vertexShaderSource;
    SString fragmentShaderSource;
    bool valid;
    bool compileOnFirstUse;
private:
    void compile();
    bool getShaderCompilaionStatus(const GLuint& shaderId);
    SString getShaderInfoLog(const GLuint& shaderId);
public:
    MShader(SString vertProg, SString fragProg, bool compileOnFirstUse=true);
    void bind();
    auto setUniform1i(const SString &name, int value) -> void;
    void setUniform1f(const SString &name, float value);
    void setUniform2f(const SString &name, const SVector2 &value);
    void setUniform3f(const SString &name, const SVector3 &value);
    void setUniform4f(const SString &name, const SVector4 &value);
    void setUniformMat4(const SString &name, const SMatrix4 &value);
};



#endif //SHADER_H
