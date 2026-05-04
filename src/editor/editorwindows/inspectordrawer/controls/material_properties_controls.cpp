//
// Created by ssj5v on 16-05-2025.
//

#include "material_properties_controls.h"

#include <vector>

#include "asset_reference_controls.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"
#include "glm/ext/vector_common.hpp"
#include "imgui.h"


void MMaterialPropertyControl::draw(MMaterialAsset* asset)
{
    if (!asset) return;

    // Resolve fresh every call — never store the raw MMaterial* which can
    // become dangling between frames if the asset is hot-reloaded.
    MMaterial* target = asset->getMaterial();
    if (!target) return;

    if (!ImGui::CollapsingHeader("Material Properties"))
        return;

    constexpr float LABEL_COL_W = 120.0f;
    if (!ImGui::BeginTable("##mat_props", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("label",  ImGuiTableColumnFlags_WidthFixed,   LABEL_COL_W);
    ImGui::TableSetupColumn("widget", ImGuiTableColumnFlags_WidthStretch);

    // Snapshot so setProperty() calls inside draw helpers don't invalidate the iterator.
    std::vector<std::pair<SString, SShaderPropertyValue>> snapshot(
        target->getProperties().begin(), target->getProperties().end());

    for (auto& [key, val] : snapshot)
        drawProperty(key, val, target);

    ImGui::EndTable();
}

void MMaterialPropertyControl::drawProperty(const SString& label,
                                             SShaderPropertyValue& propertyValue,
                                             MMaterial* target)
{
    // Skip entirely for types we don't render - avoids an empty table row.
    const auto type = propertyValue.getType();
    if (type != Int && type != Float &&
        type != UniformVec2 && type != UniformVec3 && type != UniformVec4 &&
        type != Color && type != Texture)
        return;

    // TableNextRow only after we know we have something to draw.
    ImGui::TableNextRow();

    // Label column
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());

    // Widget column - always fill it so ImGui table state stays consistent.
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);

    switch (type)
    {
        case Int:           drawIntParameter(label, propertyValue, target);      break;
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
    // Key by property label only — stable across material rebuilds.
    // target->getGUID() changes every time MMaterial is reconstructed
    // (deferred load, hot reload), which would cause the cache to miss
    // every time and show "(none)" for all texture slots.
    const SString controlRefId = label;

    MAssetReferenceControl* texRefControl = nullptr;
    if (textureReferences.contains(controlRefId))
    {
        texRefControl = textureReferences[controlRefId];
        // Sync the control if the material's property changed externally
        // (e.g. loaded from disk). Only update if the stored ID differs
        // from what the property says.
        const auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
        {
            auto* current = texRefControl->getAssetReference();
            if (!current || current->getPath() != assetPath)
            {
                auto* texAsset = MAssetManager::getInstance()->getAsset<MAsset>(assetPath);
                if (texAsset)
                    texRefControl->setAssetReference(texAsset);
            }
        }
    }
    else
    {
        texRefControl = new MAssetReferenceControl();
        textureReferences[controlRefId] = texRefControl;
        texRefControl->canAcceptAssetFuncCallback = [](MAsset* asset)
        { return dynamic_cast<MTextureAsset*>(asset) != nullptr; };

        const auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
        {
            auto* texAsset = MAssetManager::getInstance()->getAsset<MAsset>(assetPath);
            if (texAsset)
                texRefControl->setAssetReference(texAsset);
        }
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
void MMaterialPropertyControl::clearTextureReferences()
{
    for (auto& [id, ctrl] : textureReferences)
        delete ctrl;
    textureReferences.clear();
    lastPropertyCount = -1;
}