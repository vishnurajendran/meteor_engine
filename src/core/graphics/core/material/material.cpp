//
// Created by ssj5v on 05-10-2024.
//

#include "material.h"
#include <unordered_map>
#include "core/graphics/core/shader/shader.h"
#include "core/utils/logger.h"
#include "core/memory/mnew.h"

MMaterial::MMaterial(MShader* shader, ShadingMode mode) : MObject()
{
    name         = "Material";
    this->shader = shader;
    shadingMode  = mode;

    if (!shader)
    {
        MERROR("MMaterial: constructed with null shader -- properties will be empty");
        return;
    }

    properties = shader->getProperties();

    // Use the shader's declaration order so the inspector displays
    // properties in the order they appear in the .mesl file.
    propertyOrder = shader->getPropertyOrder();
}

void MMaterial::bindMaterial() const
{
    if (!shader)
    {
        MERROR("MMaterial::bindMaterial: shader is null");
        return;
    }

    shader->bind();

    // Each texture gets its own unit (0, 1, 2, ...). Binding them all to
    // unit 0 made every sampler read the last texture bound.
    unsigned int textureUnit = 0;
    for (const auto& [key, value] : properties)
    {
        if (value.getType() == SShaderPropertyType::Texture)
        {
            // Empty slots bind white (see MShader::setTexture).
            shader->setTexture(key, value.getTexAssetReference(), textureUnit++);
        }
        else
        {
            shader->setPropertyValue(key, value);
        }
    }
}

void MMaterial::setProperty(const SString& name, const SShaderPropertyValue& value)
{
    if (!properties.contains(name))
        propertyOrder.push_back(name);

    properties[name] = value;
}

const std::unordered_map<SString, SShaderPropertyValue>& MMaterial::getProperties() const
{
    return properties;
}

MMaterial* MMaterial::clone() const
{
    auto* inst = mnew<MMaterial>(shader, shadingMode);
    inst->properties    = properties;
    inst->propertyOrder = propertyOrder;
    inst->renderMode    = renderMode;
    inst->blendMode     = blendMode;
    inst->doubleSided   = doubleSided;
    inst->writeDepth    = writeDepth;
    return inst;
}