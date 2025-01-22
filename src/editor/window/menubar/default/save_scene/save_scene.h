//
// Created by ssj5v on 22-01-2025.
//

#ifndef SAVE_SCENE_MENUBAR_ITEM_H
#define SAVE_SCENE_MENUBAR_ITEM_H
#include "editor/window/menubar/menubaritem.h"


class MSaveSceneMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //SAVE_SCENE_MENUBAR_ITEM_H
