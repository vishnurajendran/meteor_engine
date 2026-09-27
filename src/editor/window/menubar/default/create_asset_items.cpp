// create_asset_items.cpp

#include "create_asset_items.h"

#include <cstring>
#include <map>
#include <vector>
#include <string>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "editor/editorassetmanager/builtin_asset_templates.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/window/menubar/menubartree.h"
#include "imgui.h"

// -------------------------------------------------------------------------------
//  Shared helpers
// -------------------------------------------------------------------------------

static MEditorAssetManager* editorAssets()
{
    return dynamic_cast<MEditorAssetManager*>(
        MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
}

// Creates through the template registry. Returns the new asset path, or an
// empty string with `outError` set.
static SString createFromTemplate(const char* templateId, const char* directory, const char* name,
                                  const std::map<SString, SString>& params, std::string& outError)
{
    auto* editorAM = editorAssets();
    if (!editorAM)
    {
        outError = "Editor asset manager is not available.";
        return {};
    }

    const SString created = editorAM->createAssetFromTemplate(
        SString(templateId), SString(directory), SString(name), params);

    if (created.empty())
        outError = "Could not create the asset. See the console for details.";
    else
        outError.clear();
    return created;
}

static void drawError(const std::string& error)
{
    if (error.empty()) return;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.f));
    ImGui::TextWrapped("%s", error.c_str());
    ImGui::PopStyleColor();
}

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
    auto* editorAM = editorAssets();
    if (!editorAM) return list;

    SAssetDirectoryNode* root = editorAM->getAssetRootNode();
    if (!root) return list;

    // BFS over the asset tree, collect all shader assets.
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
    lastError.clear();

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
        // Each entry is "ShaderName  (path/to/shader.mesl)".
        const char* preview = cachedShaders[selectedShader].label.c_str();
        if (ImGui::BeginCombo("##cn_shader", preview))
        {
            for (int i = 0; i < (int)cachedShaders.size(); ++i)
            {
                const bool sel = (i == selectedShader);
                if (ImGui::Selectable(cachedShaders[i].label.c_str(), sel))
                    selectedShader = i;
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
    ImGui::TextUnformatted("A numeric suffix is added if the name is taken.");
    ImGui::PopStyleColor();

    drawError(lastError);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = matName[0] != '\0'
                        && directory[0] != '\0'
                        && !cachedShaders.empty();

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        const std::string& chosenPath = cachedShaders[selectedShader].path;

        // The "material" template reads the shader's declared property
        // defaults and writes them in declaration order.
        const SString created = createFromTemplate(
            SBuiltInTemplateIds::Material, directory, matName,
            {
                { SBuiltInTemplateIds::MaterialParamShader, SString(chosenPath) },
                { SBuiltInTemplateIds::MaterialParamMode,   shadingMode == 1 ? "unlit" : "lit" },
            },
            lastError);

        if (!created.empty())
        {
            MLOG(SString::format("Created Material {0}", created));
            showDialog = false;
            ImGui::CloseCurrentPopup();
        }
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

// Every template that produces a shader file, in registration order.
static std::vector<SShaderTemplateEntry> buildShaderTemplateList()
{
    std::vector<SShaderTemplateEntry> list;
    auto* editorAM = editorAssets();
    if (!editorAM) return list;

    for (const auto& t : editorAM->getTemplateRegistry().getAll())
    {
        if (t.extension != SString(SEditorPaths::EXTENSION_SHADER))
            continue;

        SShaderTemplateEntry e;
        e.id        = t.id.str();
        e.label     = t.displayName.str();
        e.extension = t.extension.str();
        list.push_back(std::move(e));
    }
    return list;
}

void MCreateShaderItem::onSelect()
{
    std::strncpy(shaderName, "NewShader", sizeof(shaderName));
    selectedTemplate = 0;
    lastError.clear();
    cachedTemplates = buildShaderTemplateList();
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

    if (cachedTemplates.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("No shader templates registered.");
        ImGui::PopStyleColor();
    }
    else
    {
        if (ImGui::BeginCombo("##cs_tmpl", cachedTemplates[selectedTemplate].label.c_str()))
        {
            for (int i = 0; i < (int)cachedTemplates.size(); ++i)
            {
                const bool sel = (i == selectedTemplate);
                if (ImGui::Selectable(cachedTemplates[i].label.c_str(), sel))
                    selectedTemplate = i;
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::Text("Output: %s%s%s", directory, shaderName, SEditorPaths::EXTENSION_SHADER);
    ImGui::TextUnformatted("A numeric suffix is added if the name is taken.");
    ImGui::PopStyleColor();

    drawError(lastError);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = shaderName[0] != '\0' && directory[0] != '\0' && !cachedTemplates.empty();

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        const SString created = createFromTemplate(
            cachedTemplates[selectedTemplate].id.c_str(), directory, shaderName, {}, lastError);

        if (!created.empty())
        {
            MLOG(SString::format("Created Shader {0}", created));
            showDialog = false;
            ImGui::CloseCurrentPopup();
        }
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
    lastError.clear();
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
    ImGui::TextUnformatted("A numeric suffix is added if the name is taken.");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
    ImGui::TextWrapped("Creates a skybox asset using the default engine skybox faces.");
    ImGui::PopStyleColor();

    drawError(lastError);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = skyboxName[0] != '\0' && directory[0] != '\0';

    ImGui::BeginDisabled(!canCreate);
    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        const SString created = createFromTemplate(
            SBuiltInTemplateIds::Skybox, directory, skyboxName, {}, lastError);

        if (!created.empty())
        {
            MLOG(SString::format("Created Skybox {0}", created));
            showDialog = false;
            ImGui::CloseCurrentPopup();
        }
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