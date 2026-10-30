//
// Created by ssj5v on 22-01-2025.
//

#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"
#include "quit_editor.h"
#include "core/memory/mnew.h"

bool MQuitEditorMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MQuitEditorMenubarItem>());
    return true;
}();

SString MQuitEditorMenubarItem::getPath() const
{
    return "File/Exit Editor";
}

void MQuitEditorMenubarItem::onSelect()
{
    MEditorApplication::exit();
}




