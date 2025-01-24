//
// Created by ssj5v on 03-10-2024.
//

#pragma once
#ifndef SHADER_H
#define SHADER_H
#include <map>

#include "SFML/OpenGL.hpp"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

class MTexture;

enum SShaderPropertyType {
    NoVal=0,
    Int,
    Float,
    UniformVec2,
    UniformVec3,
    UniformVec4,
    Matrix4,
    Texture2D,
};

struct SShaderPropertyValue {
private:
    SShaderPropertyType type = NoVal;
    int intVal = 0;
    float floatVal = 0.0f;
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
    [[nodiscard]] SMatrix4 getMat4Val() const { return mat4Val; }
    [[nodiscard]] SString getTexAssetReference() const { return textureAssetReference; }

    void setIntVal(int val) { intVal = val; type = SShaderPropertyType::Int; }
    void setFloatVal(float val) { floatVal = val; type = SShaderPropertyType::Float; }
    void setVec2Val(glm::vec2 val) { vec2Val = val; type = SShaderPropertyType::UniformVec2; }
    void setVec3Val(glm::vec3 val) { vec3Val = val; type = SShaderPropertyType::UniformVec3; }
    void setVec4Val(glm::vec4 val) { vec4Val = val; type = SShaderPropertyType::UniformVec4; }
    void setMat4Val(glm::mat4 val) { mat4Val = val; type = SShaderPropertyType::Matrix4; }
    void setTextureReference(SString texturePath) { textureAssetReference = texturePath; type = SShaderPropertyType::Texture2D; }
    void print();
};

class MShader : public MObject {
private:
    GLuint shaderProgram;
    GLint getUniformLocation(const SString& name) const;
    SString vertexShaderSource;
    SString fragmentShaderSource;
    bool valid = false;
    bool compileOnFirstUse;
    std::map<SString, SShaderPropertyValue> properties;
private:
    void compile();
public:
    MShader(const SString& vertProg, const SString& fragProg, const std::map<SString, SShaderPropertyValue>& properties, const bool& compileOnFirstUse=true);
    ~MShader() override;
    [[nodiscard]] std::map<SString, SShaderPropertyValue>  getProperties() const;
    void bind();
    void setPropertyValue(const SString& name, const SShaderPropertyValue& value);
private:
    void setUniform1i(const SString &name, int value) const;
    void setUniform1f(const SString &name, float value) const;
    void setUniform2f(const SString &name, const SVector2 &value) const;
    void setUniform3f(const SString &name, const SVector3 &value) const;
    void setUniform4f(const SString &name, const SVector4 &value) const;
    void setUniformMat4(const SString &name, const SMatrix4 &value) const;
    void setTexture(const SString &name,const SString& textureAssetPath, const unsigned int& index=0) const;
};



#endif //SHADER_H
