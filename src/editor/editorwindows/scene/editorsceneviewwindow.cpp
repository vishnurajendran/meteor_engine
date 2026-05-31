#include "editorsceneviewwindow.h"

#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/engine_statics.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/stages/composite/composite_stage.h"
#include "default_engine_icon_paths.h"
#include "editor/app/editorapplication.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/settings/editor_settings.h"
#include "imgui.h"
#include "scene_raycast.h"

// --- Style constants ----------------------------------------------------------
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

MEditorSceneViewWindow::MEditorSceneViewWindow() : MEditorSceneViewWindow(700, 300) {}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Scene";
    settings.depthBits = 24;

    renderTexture = sf::RenderTexture({ 1920, 1080 }, settings);
    updateRenderTarget();

    translateIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_MOVE);
    rotateIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_ROTATE);
    scaleIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_SCALE);
    localSpaceIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_GIZMO_LOCALSPACE);
    worldSpaceIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_GIZMO_WORLDSPACE);
    gizmoOnIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_GIZMO_ENABLED);
    gizmoOffIcon.loadFromFile(SEditorAssetPaths::LOWRES_TEX_BTTN_GIZMO_DISABLED);

    editorAppInst = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    if (!editorAppInst) return;

    auto* cam = editorAppInst->getSceneCamera();
    if (cam)
    {
        SVector3 euler = glm::degrees(glm::eulerAngles(cam->getWorldRotation()));
        cameraPitch    = euler.x;
        cameraYaw      = -euler.y;
    }
}

void MEditorSceneViewWindow::onGui(float deltaTime)
{
    if (!editorAppInst)
        return;

    bool playing = editorAppInst->isPlaying() && !editorAppInst->isPaused();

    // Toolbar bar above the scene — always visible
    drawToolbar(playing);

    ImVec2 region = ImGui::GetContentRegionAvail();
    if (region.x <= 0 || region.y <= 0) return;

    drawScene(region);
    tickDeltaTimeInfo(deltaTime);
    tickViewportInteraction();
    drawFpsOverlay();

    if (profilerVisible)
        profilerDisplayer.draw(viewportMin, viewportSize);

    if (!playing)
    {
        drawEditorSceneView(deltaTime, region);
    }
}

void MEditorSceneViewWindow::tickDeltaTimeInfo(const float& deltaTime)
{
    const float dt = deltaTime;
    if (dt > 0.0f)
    {
        fpsAccum += dt;
        fpsFrames += 1;
        if (fpsAccum >= 0.5f)
        {
            displayFps = static_cast<float>(fpsFrames) / fpsAccum;
            fpsAccum = 0.0f;
            fpsFrames = 0;
        }
    }
}

void MEditorSceneViewWindow::tickViewportInteraction()
{
    viewportHovered = ImGui::IsMouseHoveringRect(
        viewportMin,{ viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y }, false);
}

void MEditorSceneViewWindow::drawScene(ImVec2 region)
{
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
}

void MEditorSceneViewWindow::handleDragDropBehaviour()
{
    if (ImGui::BeginDragDropTarget())
    {
        // Highlight viewport edge
        ImGui::GetWindowDrawList()->AddRect(
            viewportMin,
            { viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y },
            IM_COL32(82, 160, 255, 180), 0.f, 0, 2.f);

        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str()))
        {
            SString droppedId(static_cast<const char*>(payload->Data));
            const auto asset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->getAssetById<MStaticMeshAsset>(droppedId);

            if (asset)
            {
                // Raycast from the mouse position into the scene.
                auto* camera = MViewManagement::getFirstActiveCamera();
                auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();

                if (camera && scene)
                {
                    SVector3 rayOrigin, rayDir;

                    if (screenPointToRay(camera, ImGui::GetMousePos(), rayOrigin, rayDir))
                    {
                        SRaycastHit hit = SceneRaycast::castRay(scene, rayOrigin, rayDir);

                        auto* entity = MSpatialEntity::createInstance<MStaticMeshEntity>(
                            asset->getName());
                        entity->setStaticMeshAsset(asset);
                        entity->setWorldPosition(hit.point);

                        MEditorApplication::SelectedObject = entity;
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}


void MEditorSceneViewWindow::drawEditorSceneView(const float& deltaTime, const ImVec2& region)
{
    handleDragDropBehaviour();
    drawTransformHandles();
    drawSceneInfoOverlay();
    drawAxisLegend();
}

void MEditorSceneViewWindow::handleInput(float dt)
{
    if (!editorAppInst)
        return;

    if (editorAppInst->isPlaying() || editorAppInst->isPaused())
        return;

    const bool rmbOrMmbHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
                              ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (!viewportHovered && !rmbOrMmbHeld) return;
    auto* camera = MViewManagement::getFirstActiveCamera();

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

void MEditorSceneViewWindow::focusOnSelected(MCameraEntity* camera)
{
    auto* sel = MEditorApplication::SelectedObject;
    if (!sel) return;

    auto spatial = dynamic_cast<MSpatialEntity*>(sel);
    if (spatial == nullptr) return;

    SVector3 target   = spatial->getWorldPosition();
    SVector3 backward = -camera->getForwardVector();
    camera->setWorldPosition(target + backward * 5.0f);
}

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
    MEditorApplication::SelectedObject = hit;
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

void MEditorSceneViewWindow::drawTransformHandles()
{
    if (!gizmosEnabled)
        return;
    if (!MEditorApplication::SelectedObject)
        return;

    auto* primaryCamera = MViewManagement::getFirstActiveCamera();
    if (!primaryCamera)
        return;

    auto* selected = MEditorApplication::SelectedObject;
    auto* spatial = dynamic_cast<MSpatialEntity*>(selected);
    if (spatial == nullptr)
        return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportMin.x, viewportMin.y, viewportSize.x, viewportSize.y);

    SMatrix4 view = primaryCamera->getViewMatrix();
    SMatrix4 proj = primaryCamera->getProjectionMatrix(SVector2(viewportSize.x, viewportSize.y));
    SMatrix4 transform = spatial->getTransformMatrix();

    ImGui::SetNextItemAllowOverlap();
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), transformOperation, transformMode,
                         glm::value_ptr(transform));

    if (!ImGuizmo::IsUsing())
        return;

    SVector3 newWorldPos, newWorldScale, newWorldRotEuler;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform), glm::value_ptr(newWorldPos),
                                          glm::value_ptr(newWorldRotEuler), glm::value_ptr(newWorldScale));

    spatial->setWorldRotation(eulerToQuaternion(newWorldRotEuler));
    spatial->setWorldPosition(newWorldPos);

    SVector3 parentScale(1.0f);
    auto* parent = spatial->getParent();
    while (parent)
    {
        parentScale *= parent->getRelativeScale();
        parent = parent->getParent();
    }
    spatial->setRelativeScale(spatial->getParent() ? newWorldScale / parentScale : newWorldScale);
}


void MEditorSceneViewWindow::drawToolbar(bool playMode)
{
    const float iconH = ICON_SIZE;
    const float barH  = iconH + 14.0f;   // tight: 16px icon + padding

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(OVL_BG));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(OVL_PAD, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(3.0f, 2.0f));
    ImGui::BeginChild("##scene_toolbar", ImVec2(0, barH), ImGuiChildFlags_None);
    {
        // -- Edit-mode only: transform operations + space mode ----------
        if (!playMode)
        {
            auto gizmoText = getCurrentTransformGizmoText();
            auto modeText  = getCurrentTransformModeText();

            // Transform operation label + T / R / S buttons
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(gizmoText.c_str());
            ImGui::SameLine(0, OVL_PAD);

            overlayDivider(iconH);

            if (overlayImageButton("##t", translateIcon, transformOperation == ImGuizmo::TRANSLATE))
                transformOperation = ImGuizmo::TRANSLATE;
            ImGui::SameLine(0, 2);
            if (overlayImageButton("##r", rotateIcon, transformOperation == ImGuizmo::ROTATE))
                transformOperation = ImGuizmo::ROTATE;
            ImGui::SameLine(0, 2);
            if (overlayImageButton("##s", scaleIcon, transformOperation == ImGuizmo::SCALE))
                transformOperation = ImGuizmo::SCALE;

            // Space mode label + L / W buttons
            ImGui::SameLine(0, OVL_PAD);
            overlayDivider(iconH);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(modeText.c_str());
            ImGui::SameLine(0, OVL_PAD);

            if (overlayImageButton("##ml", localSpaceIcon, transformMode == ImGuizmo::LOCAL))
                transformMode = ImGuizmo::LOCAL;
            ImGui::SameLine(0, 2);
            if (overlayImageButton("##mw", worldSpaceIcon, transformMode == ImGuizmo::WORLD))
                transformMode = ImGuizmo::WORLD;

            ImGui::SameLine(0, OVL_PAD);
            overlayDivider(iconH);
        }

        // -- Always visible: gizmos toggle ------------------------------
        const char* eyeLabel = gizmosEnabled ? "Gizmos On" : "Gizmos Off";
        if (overlayTextButton(eyeLabel, eyeLabel, gizmosEnabled))
        {
            gizmosEnabled = !gizmosEnabled;
            MGizmos::enableGizmos(gizmosEnabled);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(gizmosEnabled ? "Click to hide gizmos" : "Click to show gizmos");

        // -- Always visible: render mode dropdown -----------------------
        ImGui::SameLine(0, OVL_PAD);
        overlayDivider(iconH);

        ImGui::AlignTextToFramePadding();
        {
            static const char* viewNames[] = {
                "Final", "Opaque", "Lighting", "Depth", "Light Mask"
            };
            int cur = static_cast<int>(MCompositeStage::debugView);

            ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.12f, 0.12f, 0.12f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg,        ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::Combo("##bufview", &cur, viewNames, IM_ARRAYSIZE(viewNames)))
                MCompositeStage::debugView = static_cast<EBufferDebugView>(cur);
            ImGui::PopStyleColor(3);
        }

        // -- Always visible: profiler toggle ----------------------------
        ImGui::SameLine(0, OVL_PAD);
        overlayDivider(iconH);

        const char* profLabel = profilerVisible ? "Stats On" : "Stats Off";
        if (overlayTextButton(profLabel, profLabel, profilerVisible))
            profilerVisible = !profilerVisible;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(profilerVisible ? "Click to hide stats" : "Click to show stats");

        // -- Edit-mode only: camera speed (right-aligned) ---------------
        if (!playMode)
        {
            const float sliderW = 80.0f;
            const float labelW  = ImGui::CalcTextSize("Speed").x;
            const float totalW  = labelW + 6.0f + sliderW + OVL_PAD;
            float rightX = ImGui::GetWindowWidth() - totalW;

            ImGui::SameLine(rightX);
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextUnformatted("Speed");
            ImGui::PopStyleColor();

            ImGui::SameLine(0, 6);
            ImGui::SetNextItemWidth(sliderW);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,          ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrab,       ImVec4(0.4f,  0.6f,  1.0f,  1.0f));
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.5f,  0.7f,  1.0f,  1.0f));
            ImGui::SliderFloat("##spd", &cameraMoveSpeed, 0.5f, 200.0f, "%.0f");
            ImGui::PopStyleColor(3);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(1);
}


void MEditorSceneViewWindow::drawFpsOverlay()
{
    const float margin = 10.0f;
    const float fpsW   = 90.0f;
    const float fpsH   = ImGui::GetTextLineHeightWithSpacing() * 2.0f + OVL_PAD * 2.0f;
    const float fpsX   = viewportMin.x + viewportSize.x - fpsW - margin;
    const float fpsY   = viewportMin.y + viewportSize.y - fpsH - margin;

    beginOverlayPanel("##fps_ovl", { fpsX, fpsY }, { fpsW, fpsH });
    {
        ImVec4 fpsColor;
        if      (displayFps >= 60.0f) fpsColor = ImVec4(0.35f, 0.90f, 0.35f, 1.0f);
        else if (displayFps >= 30.0f) fpsColor = ImVec4(0.95f, 0.80f, 0.20f, 1.0f);
        else                          fpsColor = ImVec4(0.95f, 0.30f, 0.25f, 1.0f);

        char fpsBuf[32];
        std::snprintf(fpsBuf, sizeof(fpsBuf), "%.0f FPS", displayFps);
        ImGui::PushStyleColor(ImGuiCol_Text, fpsColor);
        ImGui::TextUnformatted(fpsBuf);
        ImGui::PopStyleColor();

        char msBuf[32];
        const float ms = displayFps > 0.0f ? 1000.0f / displayFps : 0.0f;
        std::snprintf(msBuf, sizeof(msBuf), "%.1f ms", ms);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::TextUnformatted(msBuf);
        ImGui::PopStyleColor();
    }
    endOverlayPanel();
}

void MEditorSceneViewWindow::drawSceneInfoOverlay()
{
    const float margin = 10.0f;

    auto* camera = MEditorApplication::getSceneCamera();
    auto* sel    = MEditorApplication::SelectedObject;

    SString selLine = sel ? sel->getName() : SString("Nothing selected");
    SString camLine = "---";
    if (camera)
    {
        auto p  = camera->getWorldPosition();
        camLine = SString::format("({0:.1f}, {1:.1f}, {2:.1f})", p.x, p.y, p.z);
    }
    const char* hintLine = "F \xc2\xb7 Focus   RMB \xc2\xb7 Orbit";

    const float textW  = std::max({ ImGui::CalcTextSize(selLine.c_str()).x,
                                    ImGui::CalcTextSize(camLine.c_str()).x,
                                    ImGui::CalcTextSize(hintLine).x });
    const float panelW = textW + OVL_PAD * 2.0f + 20.0f;
    const float panelH = ImGui::GetTextLineHeightWithSpacing() * 3.0f + OVL_PAD * 2.0f;
    const float posY   = viewportMin.y + viewportSize.y - panelH - margin;

    beginOverlayPanel("##vp_info", { viewportMin.x + margin, posY }, { panelW, panelH });
    {
        // Row 1: selected object
        if (sel)
        {
            ImVec2 dotPos = ImGui::GetCursorScreenPos();
            dotPos.x += 2.0f;
            dotPos.y += ImGui::GetTextLineHeight() * 0.5f;
            ImGui::GetWindowDrawList()->AddCircleFilled(dotPos, 3.5f, IM_COL32(100, 160, 255, 220));
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

        // Row 2: camera position
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::TextUnformatted(camLine.c_str());
        ImGui::PopStyleColor();

        // Row 3: shortcut hints
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
        ImGui::TextUnformatted(hintLine);
        ImGui::PopStyleColor();
    }
    endOverlayPanel();
}

void MEditorSceneViewWindow::drawAxisLegend()
{
    auto* camera = MViewManagement::getFirstActiveCamera();
    if (!camera) return;

    const float axisLen = 28.0f;
    const float margin  = 20.0f;

    // Place the legend in the top-right, below the camera speed overlay
    ImVec2 center = {
        viewportMin.x + viewportSize.x - margin - axisLen,
        viewportMin.y + margin + 48.0f + axisLen + 10.0f
    };

    // Extract rotation part of the view matrix.
    // Multiplying a world-space axis by this gives us the direction
    // in view space (X-right, Y-up, Z-out-of-screen).
    glm::mat3 viewRot = glm::mat3(camera->getViewMatrix());

    glm::vec3 xWorld = viewRot * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 yWorld = viewRot * glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 zWorld = viewRot * glm::vec3(0.0f, 0.0f, 1.0f);

    // View X maps to screen right, view Y maps to screen up (ImGui Y is down,
    // so we negate it).
    ImVec2 xEnd = { center.x + xWorld.x * axisLen, center.y - xWorld.y * axisLen };
    ImVec2 yEnd = { center.x + yWorld.x * axisLen, center.y - yWorld.y * axisLen };
    ImVec2 zEnd = { center.x + zWorld.x * axisLen, center.y - zWorld.y * axisLen };

    auto* dl = ImGui::GetWindowDrawList();
    const float thickness = 2.0f;

    const ImU32 colX = IM_COL32(220, 50,  50,  255);
    const ImU32 colY = IM_COL32(50,  220, 50,  255);
    const ImU32 colZ = IM_COL32(50,  100, 220, 255);

    dl->AddLine(center, xEnd, colX, thickness);
    dl->AddLine(center, yEnd, colY, thickness);
    dl->AddLine(center, zEnd, colZ, thickness);

    // Place labels slightly beyond each axis endpoint
    auto nudge = [&](ImVec2 end) -> ImVec2 {
        float dx = end.x - center.x;
        float dy = end.y - center.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 0.001f) return end;
        float scale = 6.0f / len;
        return { end.x + dx * scale - 3.0f, end.y + dy * scale - 5.0f };
    };

    dl->AddText(nudge(xEnd), colX, "X");
    dl->AddText(nudge(yEnd), colY, "Y");
    dl->AddText(nudge(zEnd), colZ, "Z");
}

void MEditorSceneViewWindow::updateRenderTarget()
{
    SRenderBuffer* buf = nullptr;
    if (SRenderBuffer::makeFromRenderTarget(&renderTexture, buf))
        MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>()->setRenderTarget(buf);
}


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