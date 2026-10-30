//
// Created by ssj5v on 22-01-2025.
//

#include "reset_camera_to_origin.h"

#include "core/engine/camera/camera_spatial_entity.h"
#include "editor/app/editorapplication.h"
#include "editor/window/menubar/menubartree.h"
#include "core/memory/mnew.h"

bool MResetCameraToOriginMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(mnew<MResetCameraToOriginMenubarItem>());
    return true;
}();

SString MResetCameraToOriginMenubarItem::getPath() const
{
    return "Edit/Reset camera";
}

void MResetCameraToOriginMenubarItem::onSelect()
{
    const auto sceneCamera = MEditorApplication::getSceneCamera();
    if (sceneCamera == nullptr)
        return;

    sceneCamera->setWorldPosition(SVector3(0, 0, 0));
}
