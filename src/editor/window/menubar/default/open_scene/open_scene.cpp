//
// Created by ssj5v on 22-01-2025.
//

#include "open_scene.h"

#include "editor/window/menubar/menubartree.h"

bool registered = []()
{
    MMenubarTreeNode::registerItem(new MOpenSceneMenubarItem());
    return true;
}();

int MOpenSceneMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_HIGHEST + 1;
}

SString MOpenSceneMenubarItem::getPath() const
{
    return "File/Open Scene";
}

void MOpenSceneMenubarItem::onSelect()
{

}
