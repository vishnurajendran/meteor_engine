//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorinspectorwindow.h"

MEditorInspectorWindow::MEditorInspectorWindow(): MEditorInspectorWindow(700, 300) {

}

MEditorInspectorWindow::MEditorInspectorWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Inspector";
}

void MEditorInspectorWindow::onGui() {
    // Create a child window with a specific size and enable scrolling
}