//
// Created by ssj5v on 22-01-2025.
//

#include "create_empty_scene.h"

#include "core/application/application.h"
#include "core/engine/scene/scenemanager.h"
#include "editor/helper/scene_io.h"
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
    return "File/New Scene";
}

void MCreateEmptySceneMenubarItem::onSelect()
{
    if (MApplication::getAppInstance()->isPlaying() || MApplication::getAppInstance()->isPaused())
    {
        MWARN("New Scene operation blocked during play-mode");
        return;
    }
    MSceneIO::newScene();
}
