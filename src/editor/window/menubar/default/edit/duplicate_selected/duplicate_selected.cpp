//
// duplicate_selected.cpp
//

#include "duplicate_selected.h"

#include "imgui.h"

#include "editor/app/editorapplication.h"
#include "editor/editor_utils/entity_duplicator.h"
#include "editor/window/menubar/menubartree.h"

bool MDuplicateSelectedMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MDuplicateSelectedMenubarItem());
    return true;
}();

void MDuplicateSelectedMenubarItem::onSelect()
{
    // The shortcut listener doesn't know about ImGui focus: don't steal
    // Ctrl+D from a text field (rename box, inspector input, search).
    if (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput)
        return;

    SEntityDuplicator::duplicateAndSelect(MEditorApplication::SelectedObject);
}
