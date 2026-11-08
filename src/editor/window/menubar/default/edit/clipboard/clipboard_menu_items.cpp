//
// clipboard_menu_items.cpp
//

#include "clipboard_menu_items.h"

#include "imgui.h"

#include "editor/editor_utils/entity_clipboard.h"
#include "editor/window/menubar/menubartree.h"
#include "core/memory/mnew.h"

// The shortcut listener doesn't know about ImGui focus: leave Ctrl+X/C/V to
// a text field (rename box, inspector input, search) while one is active.
static bool textFieldActive()
{
    return ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
}

bool MCutSelectedMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MCutSelectedMenubarItem>());
    return true;
}();

void MCutSelectedMenubarItem::onSelect()
{
    if (!textFieldActive()) MEntityClipboard::cutSelected();
}

bool MCopySelectedMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MCopySelectedMenubarItem>());
    return true;
}();

void MCopySelectedMenubarItem::onSelect()
{
    if (!textFieldActive()) MEntityClipboard::copySelected();
}

bool MPasteMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MPasteMenubarItem>());
    return true;
}();

void MPasteMenubarItem::onSelect()
{
    if (!textFieldActive()) MEntityClipboard::pasteAtSelection();
}
