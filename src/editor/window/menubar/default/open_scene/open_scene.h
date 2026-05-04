#pragma once
#ifndef OPEN_SCENE_H
#define OPEN_SCENE_H

#include "editor/window/menubar/menubaritem.h"

class MOpenSceneMenubarItem : public MMenubarItem
{
public:
    [[nodiscard]] int     getPriority() const override;
    [[nodiscard]] SString getPath()     const override;

    void onSelect()  override;
    void drawPopup() override;   // called every frame via MMenubarTreeNode::drawAllPopups()

private:
    static bool registered;
};

#endif // OPEN_SCENE_H