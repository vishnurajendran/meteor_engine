#include <GL/glew.h>
#include "standard_shader.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/color_space.h"
#include "core/utils/logger.h"
#include "default_textures.h"
#include "shadercompiler.h"

MStandardShader::MStandardShader(const SString& vertProg, const SString& fragProg,
                                 const std::unordered_map<SString, SShaderPropertyValue>& props,
                                 const bool& compileFirstUse)
    : MShaderBase(props, compileFirstUse)
    , vertexShaderSource(vertProg)
    , fragmentShaderSource(fragProg)
{
    if (!compileFirstUse)
        compile();
}

void MStandardShader::compile() {
    if(compiled) {
        MWARN(STR("Shader already compiled"));
        return;
    }
    compiled = MShaderCompiler::compileShader(getName(), vertexShaderSource, fragmentShaderSource, shaderProgram);
}

void MStandardShader::bind() {
    if(!compiled && compileOnFirstUse) {
        compile();
    }

    if(!compiled) {
        MERROR(STR("Shader cannot be bound! Reason: Error during compilation and/or not compiled."));
        return;
    }

    glUseProgram(shaderProgram);
}

void MStandardShader::setPropertyValue(const SString &name, const SShaderPropertyValue &value) {
    if(properties.contains(name)) {
        properties[name] = value;
    }

    switch (value.getType()) {
        case Int:
            setUniform1i(name, value.getIntVal());
            break;
    case Bool:
        setUniform1i(name, value.getBoolValue() ? 1 : 0);
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
        case Color:
            // Colours are picked in sRGB; shaders work in linear.
            setUniform4f(name, MColorSpace::srgbToLinear(value.getColor()));
            break;
        case UniformVec4:
            setUniform4f(name, value.getVec4Val());
            break;
        case Matrix4:
            setUniformMat4(name, value.getMat4Val());
            break;
        case Texture:
            setTexture(name, value.getTexAssetReference());   // empty -> white
            break;
        default:
            break;
    }
}

void MStandardShader::setTexture(const SString &name, const SString& textureAssetPath, const unsigned int& index) const {
    auto location = getUniformLocation(name);
    if (location == -1) return;

    // Nothing assigned, or the asset can't be loaded: sample white instead
    // of whatever an earlier draw left bound on this unit.
    if (textureAssetPath.empty()) {
        setTextureWhite(name, index);
        return;
    }

    // textureAssetPath is a reference string — "guid:<id>" or a bare path.
    auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAssetFromReference<MTextureAsset>(textureAssetPath);
    if(!asset || !asset->getTexture()) {
        MERROR("Shader: Invalid Texture Asset " + textureAssetPath);
        setTextureWhite(name, index);
        return;
    }
    asset->bindForRendering(index);
    glUniform1i(location, static_cast<GLint>(index));
}

void MStandardShader::setTextureWhite(const SString &name, const unsigned int& index) const {
    auto location = getUniformLocation(name);
    if (location == -1) return;
    MDefaultTextures::bindWhite(index);
    glUniform1i(location, static_cast<GLint>(index));
}
