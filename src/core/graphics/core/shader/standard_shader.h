//
// standard_shader.h
//
// MStandardShader - a .mesl with vertex and fragment passes. What materials,
// render stages and post effects draw with.
//
#pragma once
#ifndef STANDARD_SHADER_H
#define STANDARD_SHADER_H

#include "shader.h"

class MStandardShader : public MShaderBase {
    DEFINE_OBJECT_SUBCLASS(MStandardShader)
public:
    MStandardShader(const SString& vertProg, const SString& fragProg,
                    const std::unordered_map<SString, SShaderPropertyValue>& properties,
                    const bool& compileOnFirstUse = true);

    [[nodiscard]] bool isCompute() const override { return false; }

    void bind();
    void setPropertyValue(const SString& name, const SShaderPropertyValue& value);

    // Binds a texture asset to `index` and points sampler `name` at it.
    // Public so materials can give each texture its own unit.
    // An empty reference (or a missing asset) binds a 1x1 white texture, so
    // "texture × colour" falls back to the colour.
    void setTexture(const SString &name,const SString& textureAssetPath, const unsigned int& index=0) const;
    void setTextureWhite(const SString &name, const unsigned int& index=0) const;

protected:
    void compile() override;

private:
    SString vertexShaderSource;
    SString fragmentShaderSource;
};

#endif // STANDARD_SHADER_H
