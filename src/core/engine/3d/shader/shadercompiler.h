//
// Created by ssj5v on 24-01-2025.
//

#ifndef SHADERENGINE_H
#define SHADERENGINE_H
#include <GL/glew.h>
#include "core/object/object.h"
#include "core/utils/fileio.h"


class MShaderCompiler : public MObject {
public:
    static bool initialiseEngine();
    static bool compileShader(const SString& name, const SString& vertexSource, const SString& fragmentSource, GLuint& shaderProgramHandle);
private:
    static bool initialised;
private:
    static bool getShaderCompilationStatus(const GLuint& shaderId);
    static SString getShaderInfoLog(const GLuint& shaderId);
    static SString getShaderProgramInfoLog(const GLuint& shaderId);
    static void registerGLNamedString(const SString& filePath, const SString& fileName);
};


#endif //SHADERENGINE_H
