//
// Created by ssj5v on 16-05-2025.
//

#include "material_properties_controls.h"

#include "asset_reference_controls.h"
#include "core/graphics/core/material/material.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"
#include "glm/ext/vector_common.hpp"
#include "imgui.h"


void MMaterialPropertyControl::draw(MMaterial* target)
{
    if (!ImGui::CollapsingHeader("Material Properties"))
        return;

    // Two-column table: fixed label col + stretching widget col.
    // Same pattern as drawLightIntensityAndColor in the spatial inspector.
    constexpr float LABEL_COL_W = 120.0f;
    if (!ImGui::BeginTable("##mat_props", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
    ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

    for (const auto& kv : target->getProperties())
    {
        auto property = kv.second;
        drawProperty(kv.first, property, target);
    }

    ImGui::EndTable();
}

void MMaterialPropertyControl::drawProperty(const SString& label,
                                             SShaderPropertyValue& propertyValue,
                                             MMaterial* target)
{
    // Skip entirely for types we don't render — avoids an empty table row.
    const auto type = propertyValue.getType();
    if (type != Int && type != Float &&
        type != UniformVec2 && type != UniformVec3 && type != UniformVec4 &&
        type != Color && type != Texture)
        return;

    ImGui::TableNextRow();

    // Label column
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());

    // Widget column
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);

    switch (propertyValue.getType())
    {
        case Int:           drawIntParameter(label, propertyValue, target);      break;
        case Float:         drawFloatParameter(label, propertyValue, target);    break;
        case UniformVec2:   drawVec2Parameter(label, propertyValue, target);     break;
        case UniformVec3:   drawVec3Parameter(label, propertyValue, target);     break;
        case UniformVec4:   drawVec4Parameter(label, propertyValue, target);     break;
        case Color:         drawColorParameter(label, propertyValue, target);    break;
        case Texture:       drawTextureParameter(label, propertyValue, target);  break;
        default:            return;
    }
}

bool MMaterialPropertyControl::canAcceptTextureAsset(MAsset* asset)
{
    return dynamic_cast<MTextureAsset*>(asset) != nullptr;
}

// drawLabel removed — labels are now rendered by drawProperty via the table layout.

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
    // Compact swatch — same pattern as the light inspector color picker.
    // Clicking opens a popup with the full picker; panel stays compact otherwise.
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
    auto controlRefId = target->getGUID() + "_tex_" + label;

    MAssetReferenceControl* texRefControl = nullptr;
    if (textureReferences.contains(controlRefId))
    {
        texRefControl = textureReferences[controlRefId];
    }
    else
    {
        texRefControl = new MAssetReferenceControl();
        textureReferences[controlRefId] = texRefControl;
        texRefControl->canAcceptAssetFuncCallback = canAcceptTextureAsset;

        const auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
            texRefControl->setAssetReference(MAssetManager::getInstance()->getAsset<MAsset>(assetPath));
    }

    // Use the compact single-row control so it fits inside the table without
    // breaking the row height.  The full drawControl (85px) was only needed
    // for the standalone Static Mesh inspector panels.
    if (texRefControl->drawCompactControl(label))
    {
        auto* asset = texRefControl->getAssetReference();
        value.setTextureReference(asset ? asset->getPath() : SString(""));
        target->setProperty(label, SShaderPropertyValue(value));
    }
}