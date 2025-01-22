//
// Created by ssj5v on 22-01-2025.
//

#ifndef CREATEEMPTYSCENE_H
#define CREATEEMPTYSCENE_H
#include "editor/window/menubar/menubaritem.h"


class MCreateEmptySceneMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;

private:
    static bool registered;
};



#endif //CREATEEMPTYSCENE_H
