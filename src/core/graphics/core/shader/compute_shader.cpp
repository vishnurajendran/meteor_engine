#include <GL/glew.h>
#include "compute_shader.h"

#include "core/utils/logger.h"
#include "shadercompiler.h"

MComputeShader::MComputeShader(const SString& computeProg,
                               const std::unordered_map<SString, SShaderPropertyValue>& props,
                               bool compileFirstUse)
    : MShaderBase(props, compileFirstUse)
    , computeShaderSource(computeProg)
{
    if (!compileFirstUse)
        compile();
}

void MComputeShader::compile() {
    if (compiled) {
        MWARN(STR("Shader already compiled"));
        return;
    }
    compiled = MShaderCompiler::compileComputeShader(getName(), computeShaderSource, shaderProgram);
}

bool MComputeShader::bind() {
    if (!ensureCompiled()) {
        MERROR(STR("Compute shader '") + getName() + "' cannot be bound: it failed to compile or is not compiled.");
        return false;
    }
    glUseProgram(shaderProgram);
    return true;
}

bool MComputeShader::setStorageBlockBinding(const SString& blockName, unsigned int bindingPoint) {
    if (!ensureCompiled())
        return false;
    const GLuint index = glGetProgramResourceIndex(shaderProgram, GL_SHADER_STORAGE_BLOCK, blockName.c_str());
    if (index == GL_INVALID_INDEX)
        return false;
    glShaderStorageBlockBinding(shaderProgram, index, bindingPoint);
    return true;
}

void MComputeShader::dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ) {
    glDispatchCompute(groupsX, groupsY, groupsZ);
}
