//
// Created by ssj5v on 22-01-2025.
//

#ifndef CREATEEMPTYSCENE_H
#define CREATEEMPTYSCENE_H
#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"


class MCreateEmptySceneMenubarItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MCreateEmptySceneMenubarItem)
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::N, true};
    };

    void onSelect() override;

private:
    static bool registered;
};



#endif //CREATEEMPTYSCENE_H
