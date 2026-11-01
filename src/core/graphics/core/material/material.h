//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef MATERIAL_H
#define MATERIAL_H
#include <string>
#include <unordered_map>
#include <vector>
#include "core/graphics/core/shader/shader.h"
#include "core/object/object.h"

class MShader;

class MMaterial : public MObject {
    DEFINE_OBJECT_SUBCLASS(MMaterial)
public:
    enum class ShadingMode { Lit, Unlit };

    // Which pass draws the material.
    //   Opaque      - prepass + opaque + deferred lighting; casts shadows.
    //   Transparent - MTransparentStage: forward lit, sorted back to front,
    //                 blended into BUFFER_SCENE; no depth write, no shadows.
    enum class RenderMode { Opaque, Transparent };

    // How a transparent material combines with what is behind it.
    //   Alpha    - colour * a + background * (1 - a)
    //   Additive - colour * a + background          (glows, fire, sparks)
    enum class BlendMode { Alpha, Additive };

private:
    MShader*    shader      = nullptr;
    ShadingMode shadingMode = ShadingMode::Lit;
    RenderMode  renderMode  = RenderMode::Opaque;
    BlendMode   blendMode   = BlendMode::Alpha;
    bool        doubleSided = false;   // draw back faces too (opaque and transparent)
    bool        writeDepth  = false;   // transparent only: also write BUFFER_DEPTH
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

            propertyOrder = newShader->getPropertyOrder();

            for (auto& [k, v] : overrides)
            {
                if (!properties.contains(k))
                    propertyOrder.push_back(k);
                properties[k] = v;
            }
        }
    }

    [[nodiscard]] ShadingMode getShadingMode()const { return shadingMode; }
    void setShadingMode(ShadingMode mode) { shadingMode = mode; }

    [[nodiscard]] RenderMode getRenderMode() const { return renderMode; }
    void setRenderMode(RenderMode mode) { renderMode = mode; }
    [[nodiscard]] bool isTransparent() const { return renderMode == RenderMode::Transparent; }

    [[nodiscard]] BlendMode getBlendMode() const { return blendMode; }
    void setBlendMode(BlendMode mode) { blendMode = mode; }

    [[nodiscard]] bool isDoubleSided() const { return doubleSided; }
    void setDoubleSided(bool value) { doubleSided = value; }

    // Transparent materials only. When on, the mesh writes its depth into
    // BUFFER_DEPTH as it is drawn, so effects in the RS_PostProcessing slot
    // (DOF, fog) treat it as a surface. Effects in RS_PostOpaque (SSAO) run
    // before transparents and never see it. Use for mostly solid
    // transparents (thick glass, ice); leave off for smoke and thin glass.
    [[nodiscard]] bool getWritesDepth() const { return writeDepth; }
    void setWritesDepth(bool value) { writeDepth = value; }

    // String forms used in .material files.
    static const char* renderModeToString(RenderMode mode) { return mode == RenderMode::Transparent ? "transparent" : "opaque"; }
    static RenderMode  renderModeFromString(const std::string& s) { return s == "transparent" ? RenderMode::Transparent : RenderMode::Opaque; }
    static const char* blendModeToString(BlendMode mode) { return mode == BlendMode::Additive ? "additive" : "alpha"; }
    static BlendMode   blendModeFromString(const std::string& s) { return s == "additive" ? BlendMode::Additive : BlendMode::Alpha; }
    [[nodiscard]] bool        isValid()       const { return shader != nullptr; }
    [[nodiscard]] bool isUnlit() const { return shadingMode == ShadingMode::Unlit; }
};

#endif //MATERIAL_H