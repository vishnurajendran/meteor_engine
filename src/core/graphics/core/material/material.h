//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H
#include <unordered_map>
#include <vector>
#include "core/graphics/core/shader/shader.h"
#include "core/object/object.h"

class MShader;

class MMaterial : public MObject {
    DEFINE_OBJECT_SUBCLASS(MMaterial)
public:
    enum class ShadingMode { Lit, Unlit };

private:
    MShader*    shader      = nullptr;
    ShadingMode shadingMode = ShadingMode::Lit;
    std::unordered_map<SString, SShaderPropertyValue> properties;
    std::vector<SString> propertyOrder;  // tracks key order for UI display

public:
    explicit MMaterial(MShader* shader, ShadingMode mode = ShadingMode::Lit);
    ~MMaterial() override = default;

    void bindMaterial() const;
    [[nodiscard]] const std::unordered_map<SString, SShaderPropertyValue>& getProperties() const;
    void setProperty(const SString& name, const SShaderPropertyValue& value);
    MMaterial* clone() const;

    // Ordered key list for UI display.  Defaults to the order captured from
    // the shader, but MMaterialAsset overrides it with the XML file order
    // after buildMaterialAsset() so the inspector matches the authored file.
    [[nodiscard]] const std::vector<SString>& getPropertyOrder() const { return propertyOrder; }
    void setPropertyOrder(const std::vector<SString>& order) { propertyOrder = order; }

    [[nodiscard]] MShader*    getShader()     const { return shader; }

    void updateShader(MShader* newShader, ShadingMode newMode)
    {
        shader      = newShader;
        shadingMode = newMode;
        if (newShader)
        {
            auto overrides = properties;
            properties     = newShader->getProperties();

            propertyOrder.clear();
            for (auto& [k, _] : properties)
                propertyOrder.push_back(k);

            for (auto& [k, v] : overrides)
            {
                if (!properties.contains(k))
                    propertyOrder.push_back(k);
                properties[k] = v;
            }
        }
    }

    [[nodiscard]] ShadingMode getShadingMode()const { return shadingMode; }
    [[nodiscard]] bool        isValid()       const { return shader != nullptr; }
    [[nodiscard]] bool isUnlit() const { return shadingMode == ShadingMode::Unlit; }
};

#endif //MATERIAL_H