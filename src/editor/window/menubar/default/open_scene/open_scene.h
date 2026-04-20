#pragma once
#ifndef OPEN_SCENE_H
#define OPEN_SCENE_H
#include "editor/window/menubar/menubaritem.h"

class MOpenSceneMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int     getPriority() const override;
    [[nodiscard]] SString getPath()     const override;

    // onSelect opens the path input popup.
    // drawPopup must be called every frame from the editor's main draw loop
    // so the popup can render while open.
    void onSelect() override;
    static void drawPopup();

private:
    static bool registered;
};

#endif //OPEN_SCENE_H