#include "editorinspectorwindow.h"

#include "asset_inspector.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/entities/spatial/spatial.h"
#include "editor/app/editorapplication.h"
#include "editor/editorwindows/inspectordrawer/inspectordrawer.h"

MEditorInspectorWindow::MEditorInspectorWindow()
    : MEditorInspectorWindow(700, 300) {}

MEditorInspectorWindow::MEditorInspectorWindow(int x, int y)
    : MImGuiSubWindow(x, y)
{
    title = "Inspector";
    auto dpi = DPIHelper::GetDPIScaleFactor();
    minSize  = { 820 * dpi, FLT_MAX };
    MInspectorDrawer::initialise();
}

void MEditorInspectorWindow::onGui(float deltaTime)
{
    if (!MEditorApplication::SelectedObject)
        return;

    if (auto* assetRef = dynamic_cast<MAsset*>(MEditorApplication::SelectedObject))
    {
        MAssetInspector::draw(assetRef);
        return;
    }

    if (auto* spatial = dynamic_cast<MSpatialEntity*>(MEditorApplication::SelectedObject))
    {
        if (auto* drawer = MInspectorDrawer::getDrawer(spatial))
            drawer->onDraw(spatial);

        return;
    }

    ImGui::TextDisabled("No inspector for this object type.");
}