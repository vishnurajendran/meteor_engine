//
// Created by ssj5v on 22-01-2025.
//

#include "save_scene.h"

#include "core/engine/scene/scenemanager.h"
#include "editor/window/menubar/menubartree.h"

bool MSaveSceneMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MSaveSceneMenubarItem());
    return true;
}();

int MSaveSceneMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_HIGHEST + 2;
}

SString MSaveSceneMenubarItem::getPath() const
{
    return "File/Save Scene";
}

void MSaveSceneMenubarItem::onSelect()
{
    //Todo: Add scene serialisation to MSceneManager
    //MSceneManager::saveScene();

}
