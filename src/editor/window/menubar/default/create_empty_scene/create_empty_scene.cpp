//
// Created by ssj5v on 22-01-2025.
//

#include "create_empty_scene.h"

#include "core/engine/scene/scenemanager.h"
#include "editor/window/menubar/menubartree.h"

bool MCreateEmptySceneMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MCreateEmptySceneMenubarItem());
    return true;
}();

int MCreateEmptySceneMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_HIGHEST;
}

SString MCreateEmptySceneMenubarItem::getPath() const
{
    return "File/New Empty Scene";
}

void MCreateEmptySceneMenubarItem::onSelect()
{
    MSceneManager::loadEmptyScene();
}
