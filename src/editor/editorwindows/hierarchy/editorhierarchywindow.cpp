#include "editorhierarchywindow.h"
#include "imgui.h" // must precede ImGuizmo.h and any header that pulls it in

#include <algorithm>

#include "core/engine/camera/camera_spatial_entity.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"
#include "core/engine/skybox/procedural_sky/procedural_sky.h"
#include "core/engine/skybox/skybox.h"
#include "editor/app/editorapplication.h"

// ─── Palette ──────────────────────────────────────────────────────────────────
static constexpr ImVec4 COL_SELECTED     = {0.172f, 0.365f, 0.808f, 0.70f};
static constexpr ImVec4 COL_SEL_HOVER    = {0.216f, 0.431f, 0.902f, 0.80f};
static constexpr ImVec4 COL_SEL_ACTIVE   = {0.216f, 0.431f, 0.902f, 0.90f};
static constexpr ImVec4 COL_HOVER        = {1.000f, 1.000f, 1.000f, 0.07f};
static constexpr ImVec4 COL_TRANSPARENT  = {0.000f, 0.000f, 0.000f, 0.00f};
static constexpr ImU32  COL_DROP_OUTLINE = IM_COL32(82, 130, 255, 255);
static constexpr ImU32  COL_INSERT_LINE  = IM_COL32(82, 160, 255, 255);
static constexpr ImU32  COL_SCENE_HEADER = IM_COL32(50, 50, 55, 255);
static constexpr ImU32  COL_TOOLBAR_BG   = IM_COL32(45, 45, 45, 255);
static constexpr ImU32  COL_SEPARATOR    = IM_COL32(55, 55, 55, 255);
static constexpr ImVec4 COL_TEXT_MATCH   = {1.0f, 0.85f, 0.3f, 1.0f};
static constexpr ImU32  COL_TREE_LINE    = IM_COL32(70, 70, 70, 140);

MEditorHierarchyWindow::MEditorHierarchyWindow() : MEditorHierarchyWindow(300, 600) {}

MEditorHierarchyWindow::MEditorHierarchyWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Hierarchy";

    // mesh
    typeToIcon[MStaticMesh::staticTypeInfo()] = sf::Texture("meteor_assets/icons/static_mesh.png");

    // lights
    typeToIcon[MDirectionalLight::staticTypeInfo()] = sf::Texture("meteor_assets/icons/sun.png");
    typeToIcon[MPointLight::staticTypeInfo()] = sf::Texture("meteor_assets/icons/point-light.png");
    typeToIcon[MSpotLight::staticTypeInfo()] = sf::Texture("meteor_assets/icons/spot-light.png");
    typeToIcon[MAmbientLightEntity::staticTypeInfo()] = sf::Texture("meteor_assets/icons/ambient-light.png");

    // skybox
    typeToIcon[MSkyboxEntity::staticTypeInfo()] = sf::Texture("meteor_assets/icons/sky.png");
    typeToIcon[MProceduralSkyboxEntity::staticTypeInfo()] = sf::Texture("meteor_assets/icons/sky.png");

    // camera
    typeToIcon[MCameraEntity::staticTypeInfo()] = sf::Texture("meteor_assets/icons/camera.png");


    sceneTex.loadFromFile("meteor_assets/icons/scene.png");
    entityTex.loadFromFile("meteor_assets/icons/spatial.png");

    float dpi    = DPIHelper::GetDPIScaleFactor();
    sceneTexSize  = sf::Vector2f(sceneTex.getSize().x  * dpi, sceneTex.getSize().y  * dpi);
    entityTexSize = sf::Vector2f(entityTex.getSize().x * dpi, entityTex.getSize().y * dpi);
}


void MEditorHierarchyWindow::onGui(float deltaTime)
{
    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();

    drawToolbar();

    const float statusH = 22.0f;

    // Tighten indent and row padding for the whole tree.
    // ImGui defaults (IndentSpacing=21, FramePadding.y=3) are sized for
    // general-purpose UIs; a hierarchy panel needs compact rows.
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(2.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4.0f, 1.0f));

    ImGui::BeginChild("##hier_tree",
                      ImVec2(0, ImGui::GetContentRegionAvail().y - statusH),
                      false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    if (scene)
        drawSceneRoot(scene);
    else
        ImGui::TextDisabled("  No active scene.");

    // Click on empty space -> deselect
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        MEditorApplication::SelectedObject = nullptr;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(3); // IndentSpacing, FramePadding, ItemSpacing

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 wpos    = ImGui::GetWindowPos();
    ImVec2 wsize   = ImGui::GetWindowSize();
    float  barY    = wpos.y + wsize.y - statusH;
    dl->AddRectFilled({wpos.x, barY}, {wpos.x + wsize.x, barY + statusH}, COL_TOOLBAR_BG);
    dl->AddLine({wpos.x, barY}, {wpos.x + wsize.x, barY}, COL_SEPARATOR);

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - statusH + 4.0f);
    ImGui::SetCursorPosX(8.0f);

    if (scene)
    {
        int n = countVisibleEntities(scene);
        auto* sel = MEditorApplication::SelectedObject;
        if (sel) ImGui::Text("%d entities  |  %s", n, sel->getName().c_str());
        else     ImGui::Text("%d entities", n);
    }
}


void MEditorHierarchyWindow::drawToolbar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(COL_TOOLBAR_BG));
    ImGui::BeginChild("##hier_toolbar", ImVec2(0, 32), false);
    ImGui::SetCursorPosY(6.0f);

    if (ImGui::Button("  +  "))
        ImGui::OpenPopup("##create_menu");

    if (ImGui::BeginPopup("##create_menu"))
    {
        ImGui::TextDisabled("Create");
        ImGui::Separator();
        if (ImGui::MenuItem("Empty Entity"))
        {
            auto* s = MSceneManager::getSceneManagerInstance()->getActiveScene();
            if (s) MSpatialEntity::createInstance("New Entity");
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0, 8);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 4.0f);
    ImGui::InputTextWithHint("##search", "  Search entities...",
                              searchBuffer, sizeof(searchBuffer));

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void MEditorHierarchyWindow::drawSceneRoot(MScene* scene)
{
    // Tinted background for the scene node row
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(
        p,
        {p.x + ImGui::GetContentRegionAvail().x, p.y + ImGui::GetFrameHeightWithSpacing()},
        COL_SCENE_HEADER);

    bool open = ImGui::TreeNodeEx(
        ("##scene_" + scene->getGUID().str()).c_str(),
        ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_DefaultOpen   |
        ImGuiTreeNodeFlags_OpenOnArrow);

    ImGui::SameLine(0, 4);
    ImGui::Image(sceneTex, sceneTexSize);
    ImGui::SameLine(0, 5);

    // Show * indicator when scene has unsaved changes.
    if (scene->isDirty())
    {
        SString dirtyName = scene->getName() + STR(" *");
        ImGui::TextUnformatted(dirtyName.c_str());
    }
    else
    {
        ImGui::TextUnformatted(scene->getName().c_str());
    }
    ImGui::Spacing();

    if (open)
    {
        for (auto* root : scene->getRootEntities())
            drawEntityRow(root);
        ImGui::TreePop();
    }
}

void MEditorHierarchyWindow::drawEntityRow(MSpatialEntity* entity)
{
    if (!entity) return;

    if ((entity->getEntityFlags() & EEntityFlags::HideInEditor) == EEntityFlags::HideInEditor)
        return;

    const bool searching  = strlen(searchBuffer) > 0;
    if (searching && !subtreeMatchesSearch(entity)) return;

    const bool isSelected = (MEditorApplication::SelectedObject == entity);
    const bool isLeaf     = entity->getChildren().empty();
    const bool isRenaming = (renamingEntity == entity);

    ImGui::PushID(entity->getGUID().c_str());

    // Inject row colour before the tree node so ImGui picks it up at draw time
    ImGui::PushStyleColor(ImGuiCol_Header,
                          isSelected ? COL_SELECTED    : COL_TRANSPARENT);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                          isSelected ? COL_SEL_HOVER   : COL_HOVER);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                          isSelected ? COL_SEL_ACTIVE  : COL_HOVER);

    ImGuiTreeNodeFlags nodeFlags =
          ImGuiTreeNodeFlags_SpanFullWidth  // highlight + hit area covers full width
        | ImGuiTreeNodeFlags_OpenOnArrow    // only the arrow toggles expand
        | ImGuiTreeNodeFlags_AllowOverlap;  // SameLine widgets can overlap the node
    // FramePadding is controlled globally via PushStyleVar in onGui()

    if (isSelected)  nodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (isLeaf)      nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (searching)   nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool nodeOpen = ImGui::TreeNodeEx(("##n_" + entity->getGUID().str()).c_str(), nodeFlags);

    // Pop colours immediately — they must not bleed into sibling widgets
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
    {
        MEditorApplication::SelectedObject        = entity;
         // clear asset selection
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        renamingEntity     = entity;
        pendingFocusRename = true;
        std::strncpy(renameBuffer, entity->getName().c_str(), sizeof(renameBuffer) - 1);
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        rightClickedEntity = entity;
        MEditorApplication::SelectedObject = entity;
         // clear asset selection
        ImGui::OpenPopup("##entity_ctx");
    }
    openContextMenu(entity);

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        draggedEntity = entity;
        ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &draggedEntity, sizeof(draggedEntity));

        ImGui::Image(typeToIcon.contains(entity->typeInfo()) ?
            typeToIcon[entity->typeInfo()] : entityTex, entityTexSize);

        ImGui::SameLine(0, 6);
        ImGui::TextUnformatted(entity->getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = { ImGui::GetWindowPos().x + ImGui::GetWindowWidth(),
                           ImGui::GetItemRectMax().y };
        float itemH  = itemMax.y - itemMin.y;
        float mouseY = ImGui::GetMousePos().y;
        float relY   = (itemH > 0.f) ? (mouseY - itemMin.y) / itemH : 0.5f;

        // Zone: top 25% = insert before, center 50% = reparent, bottom 25% = insert after
        enum class EDropZone { Before, Reparent, After };
        EDropZone zone = EDropZone::Reparent;
        if (relY < 0.25f)      zone = EDropZone::Before;
        else if (relY > 0.75f) zone = EDropZone::After;

        // Visual feedback — insertion line or reparent outline
        if (zone == EDropZone::Before)
        {
            ImGui::GetWindowDrawList()->AddLine(
                { itemMin.x, itemMin.y }, { itemMax.x, itemMin.y },
                COL_INSERT_LINE, 2.0f);
        }
        else if (zone == EDropZone::After)
        {
            ImGui::GetWindowDrawList()->AddLine(
                { itemMin.x, itemMax.y }, { itemMax.x, itemMax.y },
                COL_INSERT_LINE, 2.0f);
        }
        else
        {
            ImGui::GetWindowDrawList()->AddRect(
                itemMin, itemMax, COL_DROP_OUTLINE, 3.0f, 0, 1.5f);
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY"))
        {
            if (auto* src = *static_cast<MSpatialEntity**>(payload->Data); src && src != entity)
            {
                if (zone == EDropZone::Reparent)
                {
                    src->setParent(entity);
                }
                else
                {
                    // Insert before or after entity among its siblings.
                    auto* targetParent = entity->getParent();
                    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();

                    if (targetParent)
                    {
                        auto& siblings = targetParent->getChildren();
                        auto it = std::find(siblings.begin(), siblings.end(), entity);
                        int idx = (it != siblings.end()) ? (int)(it - siblings.begin()) : (int)siblings.size();
                        if (zone == EDropZone::After) ++idx;

                        // Adjust if src is already a sibling before the target.
                        if (src->getParent() == targetParent)
                        {
                            auto srcIt = std::find(siblings.begin(), siblings.end(), src);
                            int srcIdx = (srcIt != siblings.end()) ? (int)(srcIt - siblings.begin()) : -1;
                            if (srcIdx >= 0 && srcIdx < idx) --idx;
                        }

                        targetParent->insertChildAt(src, idx);
                    }
                    else if (scene)
                    {
                        auto& roots = scene->getRootEntities();
                        auto it = std::find(roots.begin(), roots.end(), entity);
                        int idx = (it != roots.end()) ? (int)(it - roots.begin()) : (int)roots.size();
                        if (zone == EDropZone::After) ++idx;

                        if (!src->getParent())
                        {
                            auto srcIt = std::find(roots.begin(), roots.end(), src);
                            int srcIdx = (srcIt != roots.end()) ? (int)(srcIt - roots.begin()) : -1;
                            if (srcIdx >= 0 && srcIdx < idx) --idx;
                        }

                        scene->insertRootEntityAt(src, idx);
                    }
                }
            }
            draggedEntity = dropTargetEntity = nullptr;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine(0, 4);

    if (typeToIcon.contains(entity->typeInfo()))
    {
        ImGui::Image(typeToIcon[entity->typeInfo()], entityTexSize);
    }
    else
    {
        ImGui::Image(entityTex, entityTexSize);
    }

    ImGui::SameLine(0, 5);

    if (isRenaming)
    {
        if (pendingFocusRename)
        {
            ImGui::SetKeyboardFocusHere();
            pendingFocusRename = false;
        }

        ImGui::SetNextItemWidth(
            std::max(80.0f, ImGui::GetContentRegionAvail().x - 8.0f));

        bool confirmed = ImGui::InputText(
            "##rename", renameBuffer, sizeof(renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_EscapeClearsAll  |
            ImGuiInputTextFlags_AutoSelectAll);

        // Commit when Enter is pressed or the field loses focus
        if (confirmed)
        {
            entity->setName(SString(renameBuffer));
            renamingEntity = nullptr;
        }
        else if (!ImGui::IsItemActive() && !ImGui::IsItemFocused())
        {
            entity->setName(SString(renameBuffer));
            renamingEntity = nullptr;
        }
    }
    else if (searching && matchesSearch(entity))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT_MATCH);
        ImGui::TextUnformatted(entity->getName().c_str());
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::TextUnformatted(entity->getName().c_str());
    }

    // Children - ImGui's indent stack handles depth automatically
    if (nodeOpen && !isLeaf)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float indentSpacing = ImGui::GetStyle().IndentSpacing;

        // The cursor is now at the first child's position (after the indent).
        // The vertical guide line sits centered in the indent gutter.
        ImVec2 childStart = ImGui::GetCursorScreenPos();
        float  lineX      = childStart.x - indentSpacing * 0.5f;
        float  lineTopY   = childStart.y;

        for (auto* child : entity->getChildren())
            drawEntityRow(child);

        // The cursor is now just past the last child row.
        float lineBottomY = ImGui::GetCursorScreenPos().y
                          - ImGui::GetStyle().ItemSpacing.y;

        // Draw the vertical connector through the indent gutter.
        if (lineBottomY > lineTopY)
            dl->AddLine({ lineX, lineTopY }, { lineX, lineBottomY },
                        COL_TREE_LINE, 1.0f);

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void MEditorHierarchyWindow::openContextMenu(MSpatialEntity* entity)
{
    if (!ImGui::BeginPopup("##entity_ctx"))
        return;

    auto* target = rightClickedEntity ? rightClickedEntity : entity;
    ImGui::TextDisabled("%s", target ? target->getName().c_str() : "Entity");
    ImGui::Separator();

    if (ImGui::MenuItem("Rename"))
    {
        renamingEntity = target;
        pendingFocusRename = true;
        std::strncpy(renameBuffer, target->getName().c_str(), sizeof(renameBuffer) - 1);
    }

    if (ImGui::MenuItem("Duplicate"))
    {
        // TODO: Add duplication logic here later
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Create Empty Child"))
    {
        auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
        if (scene && target)
        {
            auto* child = MSpatialEntity::createInstance("New Entity");
            if (child)
                child->setParent(target);
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Delete", "Del"))
    {
        if (target)
        {
            if (MEditorApplication::SelectedObject == target)
                MEditorApplication::SelectedObject = nullptr;
            target->destroy();
            rightClickedEntity = nullptr;
        }
    }
    ImGui::EndPopup();
}
void MEditorHierarchyWindow::handleDragDrop(MSpatialEntity* entity)
{

}

bool MEditorHierarchyWindow::matchesSearch(MSpatialEntity* entity) const
{
    if (!strlen(searchBuffer)) return true;
    std::string name   = entity->getName().str();
    std::string needle = searchBuffer;
    return std::search(name.begin(), name.end(),
                       needle.begin(), needle.end(),
                       [](char a, char b){ return std::tolower(a) == std::tolower(b); })
           != name.end();
}

bool MEditorHierarchyWindow::subtreeMatchesSearch(MSpatialEntity* entity) const
{
    if (matchesSearch(entity)) return true;
    for (auto* child : entity->getChildren())
        if (subtreeMatchesSearch(child)) return true;
    return false;
}

int MEditorHierarchyWindow::countVisibleEntities(MScene* scene) const
{
    std::function<int(MSpatialEntity*)> count = [&](MSpatialEntity* e) -> int {
        if (!e) return 0;
        if ((e->getEntityFlags() & EEntityFlags::HideInEditor) == EEntityFlags::HideInEditor)
            return 0;
        int n = 1;
        for (auto* c : e->getChildren()) n += count(c);
        return n;
    };
    int total = 0;
    for (auto* root : scene->getRootEntities()) total += count(root);
    return total;
}