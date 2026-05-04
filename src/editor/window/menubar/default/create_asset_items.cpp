// create_asset_items.cpp

#include "create_asset_items.h"

#include <vector>
#include <string>

#include "imgui.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/window/menubar/menubartree.h"
#include "core/utils/logger.h"

bool MCreateMaterialItem::registered = []() {
    MMenubarTreeNode::registerItem(new MCreateMaterialItem());
    return true;
}();

// Built once when the dialog opens from all currently loaded MShaderAssets.
static std::vector<SShaderEntry> buildShaderList()
{
    std::vector<SShaderEntry> list;
    auto* editorAM = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
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