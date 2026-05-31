//
// Created by ssj5v on 22-01-2025.
//

#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"
#include "quit_editor.h"

bool MQuitEditorMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MQuitEditorMenubarItem());
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




