//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "editorhierarchywindow.h"

MEditorHierarchyWindow::MEditorHierarchyWindow(): MEditorHierarchyWindow(700, 300) {

}

MEditorHierarchyWindow::MEditorHierarchyWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Hierarchy";
}

void MEditorHierarchyWindow::onGui() {
    // Create a child window with a specific size and enable scrolling
    auto size = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("TreeRegion", size, true, ImGuiWindowFlags_HorizontalScrollbar);
    // Root node
    if (ImGui::TreeNode("Root")) {
        // First child node
        if (ImGui::TreeNode("Child Node 1")) {
            ImGui::Text("This is Child Node 1");
            ImGui::TreePop();
        }

        // Second child node
        if (ImGui::TreeNode("Child Node 2")) {
            ImGui::Text("This is Child Node 2");
            ImGui::TreePop();
        }

        // Third child node
        if (ImGui::TreeNode("Child Node 3")) {
            ImGui::Text("This is Child Node 3");
            ImGui::TreePop();
        }

        // Add more nodes to test scrolling
        for (int i = 4; i <= 20; ++i) {
            if (ImGui::TreeNode(("Child Node " + std::to_string(i)).c_str())) {
                ImGui::Text("This is Child Node %d", i);
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();  // Close root node
    }

    ImGui::EndChild();
}