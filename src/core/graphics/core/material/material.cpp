//
// Created by ssj5v on 05-10-2024.
//

#include "material.h"
#include <map>
#include "core/graphics/core/shader/shader.h"
#include "core/utils/logger.h"

MMaterial::MMaterial(MShader* shader, ShadingMode mode) : MObject()
{
    name         = "Material";
    this->shader = shader;
    shadingMode  = mode;

    if (!shader)
    {
        MERROR("MMaterial: constructed with null shader — properties will be empty");
        return;
    }

    properties = shader->getProperties();
}

void MMaterial::bindMaterial() const
{
    if (!shader)
    {
        MERROR("MMaterial::bindMaterial: shader is null");
        return;
    }

    shader->bind();
    for (auto kv : properties)
        shader->setPropertyValue(kv.first, kv.second);
}

void MMaterial::setProperty(const SString& name, const SShaderPropertyValue& value)
{
    properties[name] = value;
}

const std::map<SString, SShaderPropertyValue>& MMaterial::getProperties() const
{
    return properties;
}

MMaterial* MMaterial::clone() const
{
    auto* inst = new MMaterial(shader, shadingMode);
    inst->properties = properties;
    return inst;
}