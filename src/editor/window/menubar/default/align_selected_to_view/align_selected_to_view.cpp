//
// Created by ssj5v on 22-01-2025.
//

#include "align_selected_to_view.h"
#include "core/engine/camera/camera.h"
#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"

bool MAlignObjectToViewMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MAlignObjectToViewMenubarItem());
    return true;
}();

int MAlignObjectToViewMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_REGULAR + 1;
}

SString MAlignObjectToViewMenubarItem::getPath() const
{
    return "Edit/Align object to view";
}

void MAlignObjectToViewMenubarItem::onSelect()
{
    const auto camera = MEditorApplication::getSceneCamera();
    const auto selected = MEditorApplication::Selected;
    if (selected == nullptr)
        return;
    if (camera == nullptr)
        return;

    selected->setWorldPosition(camera->getWorldPosition());
    selected->setWorldRotation(camera->getWorldRotation());
}
