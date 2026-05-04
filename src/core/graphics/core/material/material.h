//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H
#include <map>
#include "core/graphics/core/shader/shader.h"
#include "core/object/object.h"

class MShader;

class MMaterial : public MObject {
public:
    enum class ShadingMode { Lit, Unlit };

private:
    MShader*    shader      = nullptr;
    ShadingMode shadingMode = ShadingMode::Lit;
    std::map<SString, SShaderPropertyValue> properties;

public:
    // shader may be nullptr — callers must guard with if (!shader) before passing.
    explicit MMaterial(MShader* shader, ShadingMode mode = ShadingMode::Lit);
    ~MMaterial() override = default;

    void bindMaterial() const;
    [[nodiscard]] const std::map<SString, SShaderPropertyValue>& getProperties() const;
    void setProperty(const SString& name, const SShaderPropertyValue& value);
    MMaterial* clone() const;

    [[nodiscard]] MShader*    getShader()     const { return shader; }

    // Update shader and mode in place — used by hot reload so existing
    // pointers to this MMaterial remain valid after a material file changes.
    void updateShader(MShader* newShader, ShadingMode newMode)
    {
        shader      = newShader;
        shadingMode = newMode;
        // Rebuild properties from the new shader defaults, then re-apply
        // any overrides that were set on this instance.
        if (newShader)
        {
            auto overrides = properties; // save current overrides
            properties     = newShader->getProperties(); // reset to defaults
            for (auto& [k, v] : overrides)
                properties[k] = v; // re-apply overrides
        }
    }
    [[nodiscard]] ShadingMode getShadingMode()const { return shadingMode; }
    [[nodiscard]] bool        isValid()       const { return shader != nullptr; }

    // Used by SRenderItem and the opaque stage to decide the render path.
    [[nodiscard]] bool isUnlit() const { return shadingMode == ShadingMode::Unlit; }
};

#endif //MATERIAL_H