//
// Created by ssj5v on 01-06-2026.
//

#ifndef OPEN_DOCUMENTATION_H
#define OPEN_DOCUMENTATION_H
#include "editor/editor_utils/editor_utility.h"
#include "editor/window/menubar/menubaritem.h"


class MOpenDocumentation : MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_HELP + 0; };
    [[nodiscard]] SString getPath() const override { return SString("Help/Documentation"); }
    void onSelect() override { MEditorUtility::openUrl("https://vishnurajendran.github.io/meteor_engine/"); }
private:
    static bool registerd;
};



#endif //OPEN_DOCUMENTATION_H
