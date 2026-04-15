#include "imgui.h"
#include "editorsceneviewwindow.h"
#include "core/engine/camera/camera.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/scene/scenemanager.h"
#include "editor/app/editorapplication.h"

// ─── Style constants ──────────────────────────────────────────────────────────
static constexpr ImU32  OVL_BG          = IM_COL32(22,  22,  22,  210);
static constexpr ImU32  OVL_BORDER      = IM_COL32(70,  70,  70,  200);
static constexpr ImU32  OVL_BTN_ACTIVE  = IM_COL32(44,  93,  206, 230);
static constexpr ImU32  OVL_BTN_HOVER   = IM_COL32(255, 255, 255, 30);
static constexpr ImU32  OVL_DIVIDER     = IM_COL32(80,  80,  80,  200);
static constexpr float  OVL_ROUNDING    = 5.0f;
static constexpr float  OVL_PAD         = 6.0f;

// Fixed icon render size — never derived from tex.getSize() so a failed
// texture load can't collapse the panel height to zero and hide all buttons.
static constexpr float  ICON_SIZE       = 16.0f;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static bool overlayImageButton(const char* id,
                                const sf::Texture& tex,
                                bool active)
{
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(OVL_BTN_HOVER));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::ColorConvertU32ToFloat4(OVL_BTN_HOVER));
    }
    // Always use ICON_SIZE — if the texture failed to load its size is {0,0},
    // which would make an invisible button.
    const bool clicked = ImGui::ImageButton(id, tex, { ICON_SIZE, ICON_SIZE });
    ImGui::PopStyleColor(3);
    return clicked;
}

// Text-label button styled the same as overlayImageButton.
// Used for the gizmo toggle so it needs no icon asset.
static bool overlayTextButton(const char* id, const char* label, bool active)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::ColorConvertU32ToFloat4(OVL_BTN_ACTIVE));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(OVL_BTN_HOVER));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::ColorConvertU32ToFloat4(OVL_BTN_HOVER));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
    }
    const bool clicked = ImGui::Button(id);   // id doubles as label here
    (void)label;                               // kept in signature for clarity
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    return clicked;
}

// Draws a vertical divider line at the current cursor. Call before SameLine.
static void overlayDivider(float iconH)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
        { p.x, p.y + 2.0f }, { p.x, p.y + iconH - 2.0f }, OVL_DIVIDER);
    ImGui::Dummy({ 1.0f, iconH });   // advance cursor by the divider width
    ImGui::SameLine(0, OVL_PAD);
}

static void beginOverlayPanel(const char* id, ImVec2 screenPos, ImVec2 size)
{
    ImGui::SetCursorScreenPos(screenPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(OVL_BG));
    ImGui::PushStyleColor(ImGuiCol_Border,  ImGui::ColorConvertU32ToFloat4(OVL_BORDER));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(OVL_PAD, OVL_PAD));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, OVL_ROUNDING);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
}

static void endOverlayPanel()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// ─── Constructor ──────────────────────────────────────────────────────────────

MEditorSceneViewWindow::MEditorSceneViewWindow() : MEditorSceneViewWindow(700, 300) {}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Scene";
    settings.depthBits = 24;

    renderTexture = sf::RenderTexture({ 1920, 1080 }, settings);
    updateRenderTarget();

    translateIcon.loadFromFile("meteor_assets/icons/gizmo_translate.png");
    rotateIcon.loadFromFile("meteor_assets/icons/gizmo_rotate.png");
    scaleIcon.loadFromFile("meteor_assets/icons/gizmo_scale.png");
    localSpaceIcon.loadFromFile("meteor_assets/icons/gizmo_local.png");
    worldSpaceIcon.loadFromFile("meteor_assets/icons/gizmo_world.png");
    gizmoOnIcon.loadFromFile("meteor_assets/icons/gizmo_visible.png");
    gizmoOffIcon.loadFromFile("meteor_assets/icons/gizmo_hidden.png");

    auto* cam = MEditorApplication::getSceneCamera();
    if (cam)
    {
        SVector3 euler = glm::degrees(glm::eulerAngles(cam->getWorldRotation()));
        cameraPitch    = euler.x;
        cameraYaw      = -euler.y;
    }
}

// ─── onGui ────────────────────────────────────────────────────────────────────

void MEditorSceneViewWindow::onGui()
{
    ImVec2 region = ImGui::GetContentRegionAvail();
    if (region.x <= 0 || region.y <= 0) return;

    if (renderTexture.getSize().x != (unsigned)region.x ||
        renderTexture.getSize().y != (unsigned)region.y)
    {
        renderTexture = sf::RenderTexture(
            sf::Vector2u((unsigned)region.x, (unsigned)region.y), settings);
        updateRenderTarget();
    }

    ImGui::Image(renderTexture, region);
    viewportMin  = ImGui::GetItemRectMin();
    viewportSize = ImGui::GetItemRectSize();

    viewportHovered = ImGui::IsMouseHoveringRect(
        viewportMin,
        { viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y },
        /*clip=*/false);

    drawTransformHandles();

    drawOverlayToolbar();
    drawCameraSpeedOverlay();
    drawViewportInfoOverlay();
}

// ─── handleInput ──────────────────────────────────────────────────────────────

void MEditorSceneViewWindow::handleInput(float dt)
{
    const bool rmbOrMmbHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
                              ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (!viewportHovered && !rmbOrMmbHeld) return;

    auto& cameras = MViewManagement::getCameras();
    if (cameras.empty()) return;
    auto* camera = cameras[0];

    ImGui::GetIO().WantCaptureKeyboard = false;

    bool rmbHeld = handleCameraMouseInputs(camera, dt);

    if (rmbHeld)
        handleCameraKeyboardInputs(camera, dt);

    if (ImGui::IsKeyPressed(ImGuiKey_F))
        focusOnSelected(camera);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !rmbHeld                                      &&
        !ImGuizmo::IsOver()                           &&
        !ImGuizmo::IsUsing())
    {
        trySelectEntity(camera);
    }

    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f)
        cameraMoveSpeed = std::clamp(cameraMoveSpeed + scroll * 1.0f, 0.5f, 200.0f);
}

// ─── Camera — mouse ───────────────────────────────────────────────────────────

bool MEditorSceneViewWindow::handleCameraMouseInputs(MCameraEntity* camera, float dt)
{
    const bool   rmbHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    const bool   mmbHeld = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    const ImVec2 delta   = ImGui::GetIO().MouseDelta;

    if (rmbHeld || mmbHeld)
        ImGui::GetIO().WantCaptureMouse = true;

    if (rmbHeld)
    {
        cameraYaw   += delta.x * cameraSensitivity;
        cameraPitch -= delta.y * cameraSensitivity;
        cameraPitch  = std::clamp(cameraPitch, -89.0f, 89.0f);

        auto rot = glm::quat(glm::vec3(
            glm::radians(cameraPitch),
            glm::radians(-cameraYaw),
            0.0f));
        camera->setWorldRotation(rot);
    }

    if (mmbHeld)
    {
        SVector3 movement =
            camera->getRightVector() * (-delta.x * cameraMoveSpeed * 0.005f * dt * 60.0f) +
            camera->getUpVector()    * ( delta.y * cameraMoveSpeed * 0.005f * dt * 60.0f);
        camera->setWorldPosition(camera->getWorldPosition() + movement);
    }

    return rmbHeld;
}

// ─── Camera — keyboard ────────────────────────────────────────────────────────

void MEditorSceneViewWindow::handleCameraKeyboardInputs(MCameraEntity* camera, float dt)
{
    const float boost = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 4.0f : 1.0f;
    const float speed = cameraMoveSpeed * boost * dt;

    SVector3 pos = camera->getWorldPosition();

    if (ImGui::IsKeyDown(ImGuiKey_W)) pos += camera->getForwardVector() *  speed;
    if (ImGui::IsKeyDown(ImGuiKey_S)) pos += camera->getForwardVector() * -speed;
    if (ImGui::IsKeyDown(ImGuiKey_D)) pos += camera->getRightVector()   *  speed;
    if (ImGui::IsKeyDown(ImGuiKey_A)) pos += camera->getRightVector()   * -speed;
    if (ImGui::IsKeyDown(ImGuiKey_E)) pos += camera->getUpVector()      *  speed;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) pos += camera->getUpVector()      * -speed;

    camera->setWorldPosition(pos);
}

// ─── Camera — focus ───────────────────────────────────────────────────────────

void MEditorSceneViewWindow::focusOnSelected(MCameraEntity* camera)
{
    auto* sel = MEditorApplication::Selected;
    if (!sel) return;

    SVector3 target   = sel->getWorldPosition();
    SVector3 backward = -camera->getForwardVector();
    camera->setWorldPosition(target + backward * 5.0f);
}

// ─── Click-to-select ──────────────────────────────────────────────────────────

void MEditorSceneViewWindow::trySelectEntity(MCameraEntity* camera)
{
    ImVec2 mousePx = ImGui::GetMousePos();

    if (mousePx.x < viewportMin.x || mousePx.y < viewportMin.y ||
        mousePx.x > viewportMin.x + viewportSize.x ||
        mousePx.y > viewportMin.y + viewportSize.y)
        return;

    SVector3 rayOrigin, rayDir;
    if (!screenPointToRay(camera, mousePx, rayOrigin, rayDir))
        return;

    MSpatialEntity* hit = pickEntity(camera, rayOrigin, rayDir);
    MEditorApplication::Selected = hit;
}

bool MEditorSceneViewWindow::screenPointToRay(MCameraEntity*   camera,
                                               const ImVec2&    screenPx,
                                               SVector3&        outOrigin,
                                               SVector3&        outDir) const
{
    if (viewportSize.x <= 0 || viewportSize.y <= 0) return false;

    float ndcX = ( 2.0f * (screenPx.x - viewportMin.x) / viewportSize.x) - 1.0f;
    float ndcY = (-2.0f * (screenPx.y - viewportMin.y) / viewportSize.y) + 1.0f;

    SMatrix4 proj  = camera->getProjectionMatrix(SVector2(viewportSize.x, viewportSize.y));
    SMatrix4 view  = camera->getViewMatrix();
    SMatrix4 invVP = glm::inverse(proj * view);

    SVector4 nearPt = invVP * SVector4(ndcX, ndcY, -1.0f, 1.0f);
    SVector4 farPt  = invVP * SVector4(ndcX, ndcY,  1.0f, 1.0f);

    nearPt /= nearPt.w;
    farPt  /= farPt.w;

    outOrigin = SVector3(nearPt);
    outDir    = glm::normalize(SVector3(farPt) - outOrigin);
    return true;
}

MSpatialEntity* MEditorSceneViewWindow::pickEntity(MCameraEntity*  camera,
                                                    const SVector3& rayOrigin,
                                                    const SVector3& rayDir,
                                                    float           pickRadiusPx) const
{
    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
    if (!scene) return nullptr;

    // proj[1][1] = (1 / tan(fovY/2)).  At depth d from the camera,
    // one pixel spans:  d / (proj[1][1] * viewportHeight * 0.5)  world units.
    // We use this to convert the constant screen-space tolerance into a
    // world-space tolerance that scales with distance — so an object 100 units
    // away is just as easy to click as one 2 units away.
    const SMatrix4 proj      = camera->getProjectionMatrix(SVector2(viewportSize.x, viewportSize.y));
    const SMatrix4 view      = camera->getViewMatrix();
    const float    focalLen  = proj[1][1];   // cot(fovY/2)
    const float    halfH     = viewportSize.y * 0.5f;

    MSpatialEntity* bestEntity = nullptr;
    float           bestDist   = FLT_MAX;   // 3-D ray-miss distance of best candidate

    std::function<void(MSpatialEntity*)> visit = [&](MSpatialEntity* e)
    {
        if (!e) return;
        if ((e->getEntityFlags() & EEntityFlags::HideInEditor) == EEntityFlags::HideInEditor)
            return;

        const SVector3 worldPos = e->getWorldPosition();

        // Depth along the ray (negative = behind camera, skip)
        const float t = glm::dot(worldPos - rayOrigin, rayDir);
        if (t <= 0.0f) goto children;

        {
            // 3-D closest-approach distance between the ray and the entity pivot
            const SVector3 closest3D  = rayOrigin + rayDir * t;
            const float    missDist   = glm::length(closest3D - worldPos);

            // Convert the screen-space tolerance into world units at this depth.
            // toleranceWorld = pickRadiusPx * (t / (focalLen * halfH))
            const float toleranceWorld = pickRadiusPx * (t / (focalLen * halfH));

            if (missDist <= toleranceWorld && missDist < bestDist)
            {
                bestDist   = missDist;
                bestEntity = e;
            }
        }

        children:
        for (auto* child : e->getChildren())
            visit(child);
    };

    for (auto* root : scene->getRootEntities())
        visit(root);

    return bestEntity;
}

// ─── Gizmo handles ────────────────────────────────────────────────────────────

void MEditorSceneViewWindow::drawTransformHandles()
{
    if (!gizmosEnabled)                  return;
    if (!MEditorApplication::Selected)   return;

    auto cameras = MViewManagement::getCameras();
    if (cameras.empty()) return;

    auto* primaryCamera = cameras[0];
    if (!primaryCamera) return;

    auto* selected = MEditorApplication::Selected;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportSize.x, viewportSize.y);

    SMatrix4 view      = primaryCamera->getViewMatrix();
    SMatrix4 proj      = primaryCamera->getProjectionMatrix(
                             SVector2(viewportSize.x, viewportSize.y));
    SMatrix4 transform = selected->getTransformMatrix();

    ImGui::SetNextItemAllowOverlap();
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                         transformOperation, transformMode,
                         glm::value_ptr(transform));

    if (!ImGuizmo::IsUsing()) return;

    SVector3 newWorldPos, newWorldScale, newWorldRotEuler;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform),
                                          glm::value_ptr(newWorldPos),
                                          glm::value_ptr(newWorldRotEuler),
                                          glm::value_ptr(newWorldScale));

    selected->setWorldRotation(eulerToQuaternion(newWorldRotEuler));
    selected->setWorldPosition(newWorldPos);

    SVector3 parentScale(1.0f);
    auto* parent = selected->getParent();
    while (parent)
    {
        parentScale *= parent->getRelativeScale();
        parent       = parent->getParent();
    }
    selected->setRelativeScale(selected->getParent()
                               ? newWorldScale / parentScale
                               : newWorldScale);
}

// ─── Overlay: unified toolbar (top-left) ─────────────────────────────────────
//
//  [ Translate | T  R  S | | Local | L  W | | Gizmos ]
//  └─ op label ┘└─ icons ┘   └mode ┘└icons┘  └ text toggle ┘

void MEditorSceneViewWindow::drawOverlayToolbar()
{
    const float margin = 10.0f;

    // Use ICON_SIZE — never tex.getSize() — so a missing texture can't collapse
    // the panel height and hide all buttons.
    const float iconH  = ICON_SIZE;
    const float iconW  = ICON_SIZE;
    const float panelH = iconH + OVL_PAD * 2.0f + 4.0f;  // +4 for ImageButton frame

    auto gizmoText   = getCurrentTransformGizmoText();
    auto modeText    = getCurrentTransformModeText();
    const char* eyeLabel = gizmosEnabled ? "Gizmos On" : "Gizmos Off";

    float gizmoTextW = ImGui::CalcTextSize(gizmoText.c_str()).x;
    float modeTextW  = ImGui::CalcTextSize(modeText.c_str()).x;
    float eyeLabelW  = ImGui::CalcTextSize(eyeLabel).x + 12.0f; // +padding

    // Panel width = two text labels + 5 icon buttons + eye text button
    //             + 3 dividers + item gaps + breathing room
    float panelW = gizmoTextW + modeTextW
                 + (iconW + 8.0f) * 5.0f   // 5 ImageButtons (icon + frame padding)
                 + eyeLabelW
                 + OVL_PAD * 9.0f
                 + 3.0f * (1.0f + OVL_PAD * 2.0f)
                 + 24.0f;

    beginOverlayPanel("##toolbar",
                      { viewportMin.x + margin, viewportMin.y + margin },
                      { panelW, panelH });
    {
        const float textY = (panelH - ImGui::GetTextLineHeight()) * 0.5f - 1.0f;
        const float btnY  = (panelH - iconH - 4.0f) * 0.5f;   // vertically centre buttons

        // ── Section 1: operation label + T / R / S buttons ────────────────
        ImGui::SetCursorPosY(textY);
        ImGui::TextUnformatted(gizmoText.c_str());
        ImGui::SameLine(0, OVL_PAD);

        ImGui::SetCursorPosY(btnY);
        overlayDivider(iconH);

        ImGui::SetCursorPosY(btnY);
        if (overlayImageButton("##t", translateIcon, transformOperation == ImGuizmo::TRANSLATE))
            transformOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine(0, 2);
        ImGui::SetCursorPosY(btnY);
        if (overlayImageButton("##r", rotateIcon, transformOperation == ImGuizmo::ROTATE))
            transformOperation = ImGuizmo::ROTATE;
        ImGui::SameLine(0, 2);
        ImGui::SetCursorPosY(btnY);
        if (overlayImageButton("##s", scaleIcon, transformOperation == ImGuizmo::SCALE))
            transformOperation = ImGuizmo::SCALE;

        // ── Section 2: space label + L / W buttons ────────────────────────
        ImGui::SameLine(0, OVL_PAD);
        ImGui::SetCursorPosY(btnY);
        overlayDivider(iconH);

        ImGui::SetCursorPosY(textY);
        ImGui::TextUnformatted(modeText.c_str());
        ImGui::SameLine(0, OVL_PAD);

        ImGui::SetCursorPosY(btnY);
        if (overlayImageButton("##ml", localSpaceIcon, transformMode == ImGuizmo::LOCAL))
            transformMode = ImGuizmo::LOCAL;
        ImGui::SameLine(0, 2);
        ImGui::SetCursorPosY(btnY);
        if (overlayImageButton("##mw", worldSpaceIcon, transformMode == ImGuizmo::WORLD))
            transformMode = ImGuizmo::WORLD;

        // ── Section 3: gizmo visibility toggle (text button — no icon asset needed)
        ImGui::SameLine(0, OVL_PAD);
        ImGui::SetCursorPosY(btnY);
        overlayDivider(iconH);

        ImGui::SetCursorPosY(textY);
        if (overlayTextButton(eyeLabel, eyeLabel, gizmosEnabled))
        {
            gizmosEnabled = !gizmosEnabled;
            MGizmos::enableGizmos(gizmosEnabled);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(gizmosEnabled ? "Click to hide gizmos" : "Click to show gizmos");
    }
    endOverlayPanel();
}

// ─── Overlay: camera speed (top-right) ───────────────────────────────────────

void MEditorSceneViewWindow::drawCameraSpeedOverlay()
{
    const float panelW = 160.0f;
    const float panelH = 38.0f;
    const float margin = 10.0f;

    float posX = viewportMin.x + viewportSize.x - panelW - margin;
    float posY = viewportMin.y + margin;

    beginOverlayPanel("##cam_speed", { posX, posY }, { panelW, panelH });
    {
        ImGui::SetCursorPosY(4.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::TextUnformatted("Speed");
        ImGui::PopStyleColor();

        ImGui::SameLine(0, 6);
        ImGui::SetNextItemWidth(panelW - 60.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,        ImVec4(0.4f,  0.6f,  1.0f,  1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,  ImVec4(0.5f,  0.7f,  1.0f,  1.0f));
        ImGui::SliderFloat("##spd", &cameraMoveSpeed, 0.5f, 200.0f, "%.0f");
        ImGui::PopStyleColor(3);
    }
    endOverlayPanel();
}

// ─── Overlay: selection + camera info (bottom-left) ──────────────────────────
//
//  ┌─────────────────────────────┐
//  │ ● Test Spot Light           │   ← entity name, white when selected
//  │   (1.2, -4.0, 3.5)         │   ← camera world position, dimmed
//  │   F · Focus   RMB · Orbit  │   ← hint line, only when something selected
//  └─────────────────────────────┘

void MEditorSceneViewWindow::drawViewportInfoOverlay()
{
    const float margin = 10.0f;

    auto* camera = MEditorApplication::getSceneCamera();
    auto* sel    = MEditorApplication::Selected;

    // Build content strings up front so the panel can be auto-sized
    SString selLine  = sel ? sel->getName() : SString("Nothing selected");
    SString camLine  = "---";
    if (camera)
    {
        auto p  = camera->getWorldPosition();
        camLine = SString::format("({0:.1f}, {1:.1f}, {2:.1f})", p.x, p.y, p.z);
    }
    const char* hintLine = "F \xc2\xb7 Focus   RMB \xc2\xb7 Orbit";  // "F · Focus   RMB · Orbit"

    // Panel width fits the widest line
    float textW = std::max({ ImGui::CalcTextSize(selLine.c_str()).x,
                             ImGui::CalcTextSize(camLine.c_str()).x,
                             ImGui::CalcTextSize(hintLine).x });
    float panelW = textW + OVL_PAD * 2.0f + 20.0f;
    float panelH = ImGui::GetTextLineHeightWithSpacing() * 3.0f + OVL_PAD * 2.0f;

    float posY = viewportMin.y + viewportSize.y - panelH - margin;

    beginOverlayPanel("##vp_info",
                      { viewportMin.x + margin, posY },
                      { panelW, panelH });
    {
        // ── Row 1: dot + entity name ─────────────────────────────────────
        if (sel)
        {
            // Small accent dot before the name
            ImVec2 dotPos = ImGui::GetCursorScreenPos();
            dotPos.x += 2.0f;
            dotPos.y += ImGui::GetTextLineHeight() * 0.5f;
            ImGui::GetWindowDrawList()->AddCircleFilled(
                dotPos, 3.5f, IM_COL32(100, 160, 255, 220));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::TextUnformatted(selLine.c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
            ImGui::TextUnformatted(selLine.c_str());
            ImGui::PopStyleColor();
        }

        // ── Row 2: camera position ───────────────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::TextUnformatted(camLine.c_str());
        ImGui::PopStyleColor();

        // ── Row 3: keyboard shortcut hints ───────────────────────────────
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::TextUnformatted(hintLine);
        ImGui::PopStyleColor();
    }
    endOverlayPanel();
}

// ─── Render target ────────────────────────────────────────────────────────────

void MEditorSceneViewWindow::updateRenderTarget()
{
    SRenderBuffer* buf = nullptr;
    if (SRenderBuffer::makeFromRenderTarget(&renderTexture, buf))
        MRenderPipelineManager::getInstance()->setRenderTarget(buf);
}

// ─── Text helpers ─────────────────────────────────────────────────────────────

SString MEditorSceneViewWindow::getCurrentTransformGizmoText() const
{
    switch (transformOperation)
    {
        case ImGuizmo::TRANSLATE: return "Translate";
        case ImGuizmo::ROTATE:    return "Rotate";
        case ImGuizmo::SCALE:     return "Scale";
        default:                  return "???";
    }
}

SString MEditorSceneViewWindow::getCurrentTransformModeText() const
{
    switch (transformMode)
    {
        case ImGuizmo::LOCAL: return "Local";
        case ImGuizmo::WORLD: return "World";
        default:              return "???";
    }
}