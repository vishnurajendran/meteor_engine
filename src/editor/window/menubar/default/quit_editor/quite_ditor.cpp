//
// Created by ssj5v on 22-01-2025.
//

#include "quit_editor.h"
#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"

bool MQuitEditorMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MQuitEditorMenubarItem());
    return true;
}();

int MQuitEditorMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_LOWEST;
}

SString MQuitEditorMenubarItem::getPath() const
{
    return "File/Exit Editor";
}

void MQuitEditorMenubarItem::onSelect()
{
    MEditorApplication::exit();
}




