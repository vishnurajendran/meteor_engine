//
// Created by ssj5v on 22-01-2025.
//

#ifndef OPEN_SCENE_H
#define OPEN_SCENE_H
#include "editor/window/menubar/menubaritem.h"

class MOpenSceneMenubarItem : public MMenubarItem{
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //OPEN_SCENE_H
