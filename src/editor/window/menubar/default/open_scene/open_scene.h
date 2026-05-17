#pragma once
#ifndef OPEN_SCENE_H
#define OPEN_SCENE_H

#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"

class MOpenSceneMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MOpenSceneMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override;
    [[nodiscard]] SString getPath()     const override;

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::O, true};
    };

    void onSelect()  override;
    void drawPopup() override;   // called every frame via MMenubarTreeNode::drawAllPopups()

private:
    static bool registered;
};

#endif // OPEN_SCENE_H