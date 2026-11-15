//
// Created by ssj5v on 03-10-2024.
//

#pragma once
#ifndef SHADER_H
#define SHADER_H
#include <unordered_map>
#include <vector>
#include "SFML/OpenGL.hpp"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

class MTexture;

enum SShaderPropertyType {
    NoVal=0,
    Int,
    Bool,
    Float,
    UniformVec2,
    UniformVec3,
    UniformVec4,
    Matrix4,
    Texture,
    Color,
};

struct SShaderPropertyValue {
private:
    SShaderPropertyType type = NoVal;
    int intVal = 0;
    float floatVal = 0.0f;
    bool boolValue = false;
    SString textureAssetReference;
    SVector2 vec2Val = glm::vec2(0);
    SVector3 vec3Val = glm::vec3(0);
    SVector4 vec4Val = glm::vec4(0);
    SMatrix4 mat4Val = glm::mat4();
public:
    [[nodiscard]] SShaderPropertyType getType() const { return type; }
    [[nodiscard]] int getIntVal() const { return intVal; }
    [[nodiscard]] float getFloatVal() const { return floatVal; }
    [[nodiscard]] glm::vec2 getVec2Val() const { return vec2Val; }
    [[nodiscard]] glm::vec3 getVec3Val() const { return vec3Val; }
    [[nodiscard]] glm::vec4 getVec4Val() const { return vec4Val; }
    [[nodiscard]] glm::vec4 getColor() const { return vec4Val; }
    [[nodiscard]] SMatrix4 getMat4Val() const { return mat4Val; }
    [[nodiscard]] bool getBoolValue() const { return boolValue; }
    [[nodiscard]] SString getTexAssetReference() const { return textureAssetReference; }

    void setIntVal(int val) { intVal = val; type = SShaderPropertyType::Int; }
    void setFloatVal(float val) { floatVal = val; type = SShaderPropertyType::Float; }
    void setVec2Val(glm::vec2 val) { vec2Val = val; type = SShaderPropertyType::UniformVec2; }
    void setVec3Val(glm::vec3 val) { vec3Val = val; type = SShaderPropertyType::UniformVec3; }
    void setColVal(glm::vec4 val) { vec4Val = val; type = SShaderPropertyType::Color; }
    void setVec4Val(glm::vec4 val) { vec4Val = val; type = SShaderPropertyType::UniformVec4; }
    void setMat4Val(glm::mat4 val) { mat4Val = val; type = SShaderPropertyType::Matrix4; }
    void setBoolValue(bool val) { boolValue = val; type = SShaderPropertyType::Bool; }
    void setTextureReference(SString texturePath) { textureAssetReference = texturePath; type = SShaderPropertyType::Texture; }
    void print();
};

// Common to every shader program loaded from a .mesl: the GL program, its
// compile state and the properties declared in the file. Two kinds:
//
//   MStandardShader  vertex + fragment passes - materials, stages, effects
//   MComputeShader   a #COMPUTE_PASS - dispatched by code (e.g. skinning)
//
// MShaderAsset builds the right one from the file; getShader() /
// getComputeShader() hand out the typed pointer.
class MShaderBase : public MObject {
    DEFINE_OBJECT_SUBCLASS(MShaderBase)
public:
    ~MShaderBase() override;

    [[nodiscard]] virtual bool isCompute() const = 0;

    // The linked GL program, compiling it first if needed. 0 if it failed.
    GLuint getProgramHandle();
    [[nodiscard]] bool isCompiled() const { return compiled; }

    [[nodiscard]] std::unordered_map<SString, SShaderPropertyValue> getProperties() const;
    [[nodiscard]] const std::vector<SString>& getPropertyOrder() const { return propertyOrder; }
    void setPropertyOrder(const std::vector<SString>& order) { propertyOrder = order; }

protected:
    MShaderBase(const std::unordered_map<SString, SShaderPropertyValue>& properties, bool compileOnFirstUse);

    // Builds and links `shaderProgram`; sets `compiled`.
    virtual void compile() = 0;
    // Compiles on first use if allowed. False (with an error) if unusable.
    bool ensureCompiled();

    GLint getUniformLocation(const SString& name) const;
    void setUniform1i(const SString &name, int value) const;
    void setUniform1ui(const SString &name, unsigned int value) const;
    void setUniform1f(const SString &name, float value) const;
    void setUniform2f(const SString &name, const SVector2 &value) const;
    void setUniform3f(const SString &name, const SVector3 &value) const;
    void setUniform4f(const SString &name, const SVector4 &value) const;
    void setUniformMat4(const SString &name, const SMatrix4 &value) const;

    GLuint shaderProgram = 0;
    bool   compiled = false;
    bool   compileOnFirstUse = true;
    std::unordered_map<SString, SShaderPropertyValue> properties;
    std::vector<SString> propertyOrder;  // .mesl declaration order
};

// The concrete kinds - included here so code that includes shader.h keeps
// seeing a complete shader type.
#include "standard_shader.h"
#include "compute_shader.h"

#endif //SHADER_H