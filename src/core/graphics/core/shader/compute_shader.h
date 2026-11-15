//
// compute_shader.h
//
// MComputeShader - a .mesl with a #COMPUTE_PASS (GL 4.3+). Never drawn
// with: code binds it, sets its buffers and uniforms, and dispatches.
//
//   meteor_assets/engine_assets/shaders/internal/skinning.mesl
//
#pragma once
#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include "shader.h"

class MComputeShader : public MShaderBase {
    DEFINE_OBJECT_SUBCLASS(MComputeShader)
public:
    MComputeShader(const SString& computeProg,
                   const std::unordered_map<SString, SShaderPropertyValue>& properties,
                   bool compileOnFirstUse = true);

    [[nodiscard]] bool isCompute() const override { return true; }

    // glUseProgram, compiling first if needed. False if the program is unusable.
    bool bind();

    // Uniform setters for the bound program. Missing uniforms are ignored.
    void setUInt(const SString& name, unsigned int value) const  { setUniform1ui(name, value); }
    void setInt(const SString& name, int value) const            { setUniform1i(name, value); }
    void setFloat(const SString& name, float value) const        { setUniform1f(name, value); }

    // Points the shader storage block `blockName` at SSBO binding point
    // `bindingPoint` (glShaderStorageBlockBinding). False if the program is
    // unusable or has no such block.
    bool setStorageBlockBinding(const SString& blockName, unsigned int bindingPoint);

    // glDispatchCompute on the bound program.
    static void dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1);

protected:
    void compile() override;

private:
    SString computeShaderSource;
};

#endif // COMPUTE_SHADER_H
