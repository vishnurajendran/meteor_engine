//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorinspectorwindow.h"
#include "editor/app/editorapplication.h"
#include "editor/editorwindows/inspectordrawer/inspectordrawer.h"

MEditorInspectorWindow::MEditorInspectorWindow(): MEditorInspectorWindow(700, 300) {

}

MEditorInspectorWindow::MEditorInspectorWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Inspector";
    auto dpi = DPIHelper::GetDPIScaleFactor();
    minSize = {820 * dpi, FLT_MAX};
    MInspectorDrawer::initialise();
}

void MEditorInspectorWindow::onGui() {
    if(MEditorApplication::Selected == nullptr)
        return;

    auto selected = MEditorApplication::Selected;
    auto drawer = MInspectorDrawer::getDrawer(selected);
    if(drawer){
        drawer->onDrawInspector(selected);
    }
}