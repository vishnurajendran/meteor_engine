// create_asset_items.cpp

#include "create_asset_items.h"

#include <vector>
#include <string>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/window/menubar/menubartree.h"
#include "imgui.h"

// -------------------------------------------------------------------------------
//  Material
// -------------------------------------------------------------------------------

bool MCreateMaterialItem::registered = []() {
    MMenubarTreeNode::registerItem(new MCreateMaterialItem());
    return true;
}();

// Built once when the dialog opens from all currently loaded MShaderAssets.
static std::vector<SShaderEntry> buildShaderList()
{
    std::vector<SShaderEntry> list;
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
    if (!editorAM) return list;

    SAssetDirectoryNode* root = editorAM->getAssetRootNode();
    if (!root) return list;

    // BFS over the asset tree, collect all .mesl files.
    std::queue<SAssetDirectoryNode*> q;
    q.push(root);
    while (!q.empty())
    {
        auto* node = q.front(); q.pop();
        if (!node) continue;

        if (!node->isDirectory && node->assetReference)
        {
            // Check if it's a shader asset by dynamic_cast.
            if (dynamic_cast<MShaderAsset*>(node->assetReference))
            {
                // ignore all internal shaders, since they are not meant to be use to render
                // meshes or game geometry
                if (node->assetReference->getPath().contains("/internal/"))
                    continue;

                SShaderEntry e;
                e.path  = node->assetReference->getPath().str();
                e.label = node->assetReference->getName().str()
                          + "  (" + node->assetReference->getPath().str() + ")";
                list.push_back(std::move(e));
            }
        }

        for (auto* child : node->getChildrenNodes())
            q.push(child);
    }

    return list;
}

void MCreateMaterialItem::onSelect()
{
    std::strncpy(matName,  "NewMaterial", sizeof(matName));
    shadingMode    = 0;
    selectedShader = 0;

    // Rebuild the shader list from the live asset tree each time the dialog
    // opens so newly imported shaders appear without restarting the editor.
    cachedShaders  = buildShaderList();

    showDialog = true;
}

void MCreateMaterialItem::drawPopup()
{
    if (!showDialog) return;

    ImGui::OpenPopup("##create_material_dlg");

    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("##create_material_dlg", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoTitleBar))
        return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.f));
    ImGui::TextUnformatted("Create New Material");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    constexpr float LW = 90.f;

    ImGui::Text("Name");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cn_name", matName, sizeof(matName));

    ImGui::Text("Directory");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cn_dir", directory, sizeof(directory));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Relative to the project root, e.g. assets/materials/");

    ImGui::Text("Shader");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);

    if (cachedShaders.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("No .mesl shaders found in asset tree.");
        ImGui::PopStyleColor();
    }
    else
    {
        // Build a null-terminated label array for ImGui::Combo.
        // Each entry is "ShaderName  (path/to/shader.mesl)".
        const char* preview = cachedShaders[selectedShader].label.c_str();
        if (ImGui::BeginCombo("##cn_shader", preview))
        {
            for (int i = 0; i < (int)cachedShaders.size(); ++i)
            {
                const bool sel = (i == selectedShader);
                if (ImGui::Selectable(cachedShaders[i].label.c_str(), sel))
                {
                    selectedShader = i;
                    MLOG(SString::format("Selected Item {0}", cachedShaders[selectedShader].label));
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Text("Mode");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::Combo("##cn_mode", &shadingMode, "Lit\0Unlit\0");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::Text("Output: %s%s.material", directory, matName);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = matName[0] != '\0'
                        && directory[0] != '\0'
                        && !cachedShaders.empty();

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        auto mode = (shadingMode == 1)
                    ? MMaterial::ShadingMode::Unlit
                    : MMaterial::ShadingMode::Lit;

        const std::string& chosenPath = cachedShaders[selectedShader].path;
        if (MMaterialAsset::createNewMaterial(
            SString(directory),
            SString(matName),
            SString(chosenPath.c_str()),
            mode))
        {
           MLOG(SString::format("Created Material {0}", matName));
        }
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    
}

// -------------------------------------------------------------------------------
//  Shader
// -------------------------------------------------------------------------------

bool MCreateShaderItem::registered = []() {
    MMenubarTreeNode::registerItem(new MCreateShaderItem());
    return true;
}();

// Labels and enum values shown in the template combo.
static constexpr const char* k_templateLabels[] = {
    "Lit (Standard)",
    "Unlit",
    "Unlit Color",
    "Toon",
};
static constexpr EShaderTemplate k_templateValues[] = {
    EShaderTemplate::Lit,
    EShaderTemplate::Unlit,
    EShaderTemplate::UnlitColor,
    EShaderTemplate::Toon,
};
static constexpr int k_templateCount = sizeof(k_templateValues) / sizeof(k_templateValues[0]);

void MCreateShaderItem::onSelect()
{
    std::strncpy(shaderName, "NewShader", sizeof(shaderName));
    selectedTemplate = 0;
    showDialog = true;
}

void MCreateShaderItem::drawPopup()
{
    if (!showDialog) return;

    ImGui::OpenPopup("##create_shader_dlg");

    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("##create_shader_dlg", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoTitleBar))
        return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.5f, 0.15f, 1.f));
    ImGui::TextUnformatted("Create New Shader");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    constexpr float LW = 90.f;

    ImGui::Text("Name");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cs_name", shaderName, sizeof(shaderName));

    ImGui::Text("Directory");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cs_dir", directory, sizeof(directory));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Relative to the project root, e.g. assets/shaders/");

    ImGui::Text("Template");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::Combo("##cs_tmpl", &selectedTemplate, k_templateLabels, k_templateCount);

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::Text("Output: %s%s.mesl", directory, shaderName);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = shaderName[0] != '\0' && directory[0] != '\0';

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(
            MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
        if (editorAM)
        {
            EShaderTemplate tmpl = k_templateValues[selectedTemplate];
            if (editorAM->createShaderAsset(SString(directory), SString(shaderName), tmpl))
                MLOG(SString::format("Created Shader {0}", shaderName));
        }
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

// -------------------------------------------------------------------------------
//  Skybox
// -------------------------------------------------------------------------------

bool MCreateSkyboxItem::registered = []() {
    MMenubarTreeNode::registerItem(new MCreateSkyboxItem());
    return true;
}();

void MCreateSkyboxItem::onSelect()
{
    std::strncpy(skyboxName, "NewSkybox", sizeof(skyboxName));
    showDialog = true;
}

void MCreateSkyboxItem::drawPopup()
{
    if (!showDialog) return;

    ImGui::OpenPopup("##create_skybox_dlg");

    ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("##create_skybox_dlg", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize |
                                ImGuiWindowFlags_NoTitleBar))
        return;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.7f, 0.9f, 1.f));
    ImGui::TextUnformatted("Create New Skybox");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    constexpr float LW = 90.f;

    ImGui::Text("Name");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cb_name", skyboxName, sizeof(skyboxName));

    ImGui::Text("Directory");
    ImGui::SameLine(LW);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##cb_dir", directory, sizeof(directory));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Relative to the project root, e.g. assets/skybox/");

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::Text("Output: %s%s%s", directory, skyboxName, SEditorPaths::EXTENSION_SKYBOX);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::TextWrapped("Creates a skybox asset with six empty face slots. ");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = skyboxName[0] != '\0' && directory[0] != '\0';

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(
            MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
        if (editorAM)
        {
            if (editorAM->createSkyboxAsset(SString(directory), SString(skyboxName)))
                MLOG(SString::format("Created Skybox {0}", skyboxName));
        }
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
    {
        showDialog = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}