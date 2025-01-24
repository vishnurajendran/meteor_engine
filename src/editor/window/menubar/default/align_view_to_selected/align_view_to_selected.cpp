//
// Created by ssj5v on 22-01-2025.
//

#include "align_view_to_selected.h"

#include "core/engine/camera/camera.h"
#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"

bool MAlignViewToSelectedMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MAlignViewToSelectedMenubarItem());
    return true;
}();

int MAlignViewToSelectedMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_REGULAR + 2;
}

SString MAlignViewToSelectedMenubarItem::getPath() const
{
    return "Edit/Align view to selected";
}

void MAlignViewToSelectedMenubarItem::onSelect()
{
    const auto camera = MEditorApplication::getSceneCamera();
    const auto selected = MEditorApplication::Selected;
    if (selected == nullptr)
        return;
    if (camera == nullptr)
        return;

    MLOG(STR("COMMAND: Align View to Selected - SELECTED:" + std::to_string(selected->getWorldPosition().z)));
    camera->setWorldPosition(selected->getWorldPosition());
    camera->setWorldRotation(selected->getWorldRotation());
    MLOG("COMMAND: Align View to Selected");
}
