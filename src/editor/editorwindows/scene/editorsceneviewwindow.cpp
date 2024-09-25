//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorsceneviewwindow.h"

MEditorSceneViewWindow::MEditorSceneViewWindow(): MEditorSceneViewWindow(700, 300) {

}

MEditorSceneViewWindow::MEditorSceneViewWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Scene";
}

void MEditorSceneViewWindow::onGui() {
    // Create a child window with a specific size and enable scrolling

}