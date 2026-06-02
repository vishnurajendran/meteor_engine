//
// Created by ssj5v on 16-05-2025.
//

#include "material_properties_controls.h"
#include <cctype>
#include <string>
#include <vector>
#include "asset_reference_controls.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/utils/logger.h"
#include "imgui.h"

// ── Key prettifier ───────────────────────────────────────────────────────────
// Strips leading underscores, inserts spaces before camelCase transitions,
// and capitalizes the first letter.
//
//   _emissionColor   ->  Emission Color
//   _useAlbedoTex    ->  Use Albedo Tex
//   _metallic        ->  Metallic
//   _normalMap       ->  Normal Map

static SString prettifyPropertyKey(const SString& key)
{
    std::string s = key.str();

    // Strip leading underscores
    while (!s.empty() && s[0] == '_')
        s = s.substr(1);

    if (s.empty()) return key;

    // Capitalize first letter
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));

    // Insert a space before each uppercase letter that follows a lowercase
    std::string result;
    result += s[0];
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (std::isupper(static_cast<unsigned char>(s[i])) &&
            std::islower(static_cast<unsigned char>(s[i - 1])))
            result += ' ';
        result += s[i];
    }

    return SString(result.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────

void MMaterialPropertyControl::draw(MMaterialAsset* asset)
{
    if (!asset) return;

    MMaterial* target = asset->getMaterial();
    if (!target) return;

    if (!ImGui::CollapsingHeader("Material Properties"))
        return;

    constexpr float LABEL_COL_W = 120.0f;
    if (!ImGui::BeginTable("##mat_props", 2,
                           ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        return;

    ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
    ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

    const auto& props = target->getProperties();
    const auto& order = target->getPropertyOrder();
    if (props.empty())
    {
        ImGui::EndTable();
        return;
    }

    // Iterate in the order the shader declared the properties, not the
    // arbitrary hash-bucket order of the unordered_map.  Each value is
    // copied so draw helpers can modify it without invalidating the map.
    for (const auto& key : order)
    {
        auto it = props.find(key);
        if (it == props.end()) continue;
        auto val = it->second;
        drawProperty(key, val, target);
    }

    ImGui::EndTable();
}

void MMaterialPropertyControl::drawProperty(const SString& label,
                                             SShaderPropertyValue& propertyValue,
                                             MMaterial* target)
{
    const auto type = propertyValue.getType();
    if (type != Int && type != Float &&
        type != UniformVec2 && type != UniformVec3 && type != UniformVec4 &&
        type != Color && type != Texture && type != Bool)
        return;

    ImGui::TableNextRow();

    // Label column -- show the prettified name
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(prettifyPropertyKey(label).c_str());

    // Widget column
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);

    switch (type)
    {
        case Int:           drawIntParameter(label, propertyValue, target);      break;
        case Bool:          drawBoolParameter(label, propertyValue, target);     break;
        case Float:         drawFloatParameter(label, propertyValue, target);    break;
        case UniformVec2:   drawVec2Parameter(label, propertyValue, target);     break;
        case UniformVec3:   drawVec3Parameter(label, propertyValue, target);     break;
        case UniformVec4:   drawVec4Parameter(label, propertyValue, target);     break;
        case Color:         drawColorParameter(label, propertyValue, target);    break;
        case Texture:       drawTextureParameter(label, propertyValue, target);  break;
        default:            ImGui::TextDisabled("(unsupported type)");           break;
    }
}

bool MMaterialPropertyControl::canAcceptTextureAsset(MAsset* asset)
{
    return dynamic_cast<MTextureAsset*>(asset) != nullptr;
}

void MMaterialPropertyControl::drawFloatParameter(SString label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id  = STR("##_FLOAT_") + label + "_" + target->getGUID();
    auto val = value.getFloatVal();
    if (ImGui::DragFloat(id.c_str(), &val, 0.01f))
    {
        value.setFloatVal(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawIntParameter(SString label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id  = STR("##_INT_") + label + "_" + target->getGUID();
    auto val = value.getIntVal();
    if (ImGui::DragInt(id.c_str(), &val))
    {
        value.setIntVal(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawVec2Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id  = STR("##_F2_") + label + "_" + target->getGUID();
    auto val = value.getVec2Val();
    if (ImGui::DragFloat2(id.c_str(), glm::value_ptr(val), 0.01f))
    {
        value.setVec2Val(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawBoolParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    // Use ## prefix so the checkbox has no visible text -- the label is
    // already rendered in the table's label column by drawProperty().
    auto id  = STR("##_BOOL_") + label + "_" + target->getGUID();
    auto val = value.getBoolValue();
    if (ImGui::Checkbox(id.c_str(), &val))
    {
        value.setBoolValue(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawVec3Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id  = STR("##_F3_") + label + "_" + target->getGUID();
    auto val = value.getVec3Val();
    if (ImGui::DragFloat3(id.c_str(), glm::value_ptr(val), 0.01f))
    {
        value.setVec3Val(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawVec4Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id  = STR("##_F4_") + label + "_" + target->getGUID();
    auto val = value.getVec4Val();
    if (ImGui::DragFloat4(id.c_str(), glm::value_ptr(val), 0.01f))
    {
        value.setVec4Val(val);
        target->setProperty(label, value);
    }
}

void MMaterialPropertyControl::drawColorParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto       id   = STR("##_COL_") + label + "_" + target->getGUID();
    auto       popId = STR("##_COLPOP_") + label + "_" + target->getGUID();
    glm::vec4  val  = value.getVec4Val();

    const ImVec2 swatchSize = { ImGui::GetContentRegionAvail().x, 20.0f };
    if (ImGui::ColorButton(id.c_str(),
                           ImVec4(val.r, val.g, val.b, val.a),
                           ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreview,
                           swatchSize))
    {
        ImGui::OpenPopup(popId.c_str());
    }

    if (ImGui::BeginPopup(popId.c_str()))
    {
        if (ImGui::ColorPicker4(id.c_str(), glm::value_ptr(val),
                                ImGuiColorEditFlags_Float |
                                ImGuiColorEditFlags_DisplayRGB |
                                ImGuiColorEditFlags_DisplayHex))
        {
            value.setColVal(val);
            target->setProperty(label, value);
        }
        ImGui::EndPopup();
    }
}

void MMaterialPropertyControl::drawTextureParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    const SString controlRefId = label;

    MAssetReferenceControl* texRefControl = nullptr;
    if (textureReferences.contains(controlRefId))
    {
        texRefControl = textureReferences[controlRefId];

        // Sync the control to match the material's current property value.
        const auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
        {
            auto current = texRefControl->getAssetReference();
            if (!current || current->getPath() != assetPath)
            {
                auto texAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                                     ->getAsset<MAsset>(assetPath);
                if (texAsset)
                    texRefControl->setAssetReference(texAsset);
            }
        }
        else
        {
            // Property has no texture assigned -- clear any stale reference
            // left over from a previously inspected material.
            texRefControl->setAssetReference(TAssetHandle<MAsset>());
        }
    }
    else
    {
        texRefControl = new MAssetReferenceControl();
        textureReferences[controlRefId] = texRefControl;
        texRefControl->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> asset)
        { return dynamic_cast<MTextureAsset*>(asset.get()) != nullptr; };

        const auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
        {
            auto texAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                                 ->getAsset<MAsset>(assetPath);
            if (texAsset)
                texRefControl->setAssetReference(texAsset);
        }
    }

    if (texRefControl->drawCompactControl(label))
    {
        auto asset = texRefControl->getAssetReference();
        value.setTextureReference(asset ? asset->getPath() : SString(""));
        target->setProperty(label, SShaderPropertyValue(value));
    }
}

void MMaterialPropertyControl::clearTextureReferences()
{
    for (auto& [id, ctrl] : textureReferences)
        delete ctrl;
    textureReferences.clear();
    lastPropertyCount = -1;
}