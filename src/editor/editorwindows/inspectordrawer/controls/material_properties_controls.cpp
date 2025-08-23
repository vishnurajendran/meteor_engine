//
// Created by ssj5v on 16-05-2025.
//

#include "material_properties_controls.h"

#include "asset_reference_controls.h"
#include "core/engine/3d/material/material.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/logger.h"
#include "glm/ext/vector_common.hpp"
#include "imgui.h"


void MMaterialPropertyControl::draw(MMaterial* target)
{
    if (ImGui::CollapsingHeader("Material Properties"))
    {
        int id = 0;
        for (const auto& kv : target->getProperties())
        {
            auto property = kv.second;
            drawProperty(kv.first, property, target);
        }
    }
}

bool MMaterialPropertyControl::canAcceptTextureAsset(MAsset* asset)
{
    MLOG("SADGE");
    return dynamic_cast<MTextureAsset*>(asset) != nullptr;
}

void MMaterialPropertyControl::drawLabel(SString label, SString id)
{
    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::PopID();
}

void MMaterialPropertyControl::drawProperty(const SString& label, SShaderPropertyValue& propertyValue, MMaterial* target)
{
    if (propertyValue.getType() == SShaderPropertyType::Texture)
    {
        drawTextureParameter(label, propertyValue, target);
        return;
    }

    auto id = STR("##_CHILD_") + label;
    if (ImGui::BeginChild(id.c_str(), ImVec2(ImGui::GetWindowSize().x,0), true,
            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders))
    {
        switch (propertyValue.getType())
        {
        case Int:
            drawIntParameter(label, propertyValue, target);
            drawLabel(label,STR("ID_TEXT_")+label);
            break;
        case Float:
            drawFloatParameter(label, propertyValue, target);
            drawLabel(label,STR("ID_TEXT_")+label);
            break;
        case UniformVec2:
            drawVec2Parameter(label, propertyValue, target);
            drawLabel(label,STR("ID_TEXT_")+label);
            break;
        case UniformVec3:
            drawVec3Parameter(label, propertyValue, target);
            drawLabel(label,STR("ID_TEXT_")+label);
            break;
        case UniformVec4:
            drawVec4Parameter(label, propertyValue, target);
            drawLabel(label,STR("ID_TEXT_")+label);
            break;
        case Color:
            drawLabel(label,STR("ID_TEXT_")+label);
            drawColorParameter(label, propertyValue, target);
            break;
        default:
            break;
        }
    }
    ImGui::EndChild();
}

void MMaterialPropertyControl::drawFloatParameter(SString label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_FLOAT_") + label + "_" + target->getGUID();
    auto val = value.getFloatVal();
    if (ImGui::InputFloat(id.c_str(), &val))
    {
        value.setFloatVal(val);
        target->setProperty(label, value);
    }
}


void MMaterialPropertyControl::drawIntParameter(SString label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_INT_") + label + "_" + target->getGUID();
    auto val = value.getIntVal();
    ImGui::SetNextItemWidth(glm::min(ImGui::GetContentRegionAvail().x, 120.0f));
    if (ImGui::InputInt(id.c_str(), &val))
    {
        value.setIntVal(val);
        target->setProperty(label, value);
    }
}
void MMaterialPropertyControl::drawVec2Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_F2_") + label + "_" + target->getGUID();
    auto val = value.getVec2Val();
    ImGui::SetNextItemWidth(glm::min(ImGui::GetContentRegionAvail().x, 120.0f));
    if (ImGui::InputFloat2(id.c_str(), glm::value_ptr(val)))
    {
        value.setVec2Val(val);
        target->setProperty(label, value);
    }
}
void MMaterialPropertyControl::drawVec3Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_F3_") + label + "_" + target->getGUID();
    auto val = value.getVec3Val();
    ImGui::SetNextItemWidth(glm::min(ImGui::GetContentRegionAvail().x, 120.0f));
    if (ImGui::InputFloat3(id.c_str(), glm::value_ptr(val)))
    {
        value.setVec3Val(val);
        target->setProperty(label, value);
    }
}
void MMaterialPropertyControl::drawVec4Parameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_F4_") + label + "_" + target->getGUID();
    auto val = value.getVec4Val();
    ImGui::SetNextItemWidth(glm::min(ImGui::GetContentRegionAvail().x, 120.0f));
    if (ImGui::InputFloat4(id.c_str(), glm::value_ptr(val)))
    {
        value.setVec4Val(val);
        target->setProperty(label, value);
    }

}

void MMaterialPropertyControl::drawColorParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    auto id = STR("##_COL_") + label + "_" + target->getGUID();
    auto val = value.getVec4Val();
    ImGui::SetNextItemWidth(glm::min(ImGui::GetContentRegionAvail().x, 120.0f));
    if (ImGui::ColorPicker4(id.c_str(), glm::value_ptr(val)))
    {
        value.setColVal(val);
        target->setProperty(label, value);
    }
}
void MMaterialPropertyControl::drawTextureParameter(const SString& label, SShaderPropertyValue& value, MMaterial* target)
{
    MAssetReferenceControl* texRefControl = nullptr;
    auto controlRefId = target->getGUID()+"_tex_" + label;
    if (textureReferences.contains(controlRefId))
    {
        texRefControl = textureReferences[controlRefId];
    }
    else
    {
        texRefControl = new MAssetReferenceControl();
        textureReferences[controlRefId] = texRefControl;
        texRefControl->canAcceptAssetFuncCallback = canAcceptTextureAsset;
        auto assetPath = value.getTexAssetReference();
        if (!assetPath.empty())
        {
            texRefControl->setAssetReference(MAssetManager::getInstance()->getAsset<MAsset>(assetPath));
        }
    }

    if (texRefControl->drawControl(label))
    {
        auto asset = texRefControl->getAssetReference();
        if (asset == nullptr)
            value.setTextureReference("");
        else
            value.setTextureReference(asset->getPath());
        const SShaderPropertyValue newProperty(value);
        target->setProperty(label, newProperty);
    }
}
