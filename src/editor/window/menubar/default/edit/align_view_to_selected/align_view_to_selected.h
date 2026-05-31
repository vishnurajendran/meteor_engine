//
// Created by ssj5v on 22-01-2025.
//

#ifndef ALIGN_VIEW_TO_SELECTED_H
#define ALIGN_VIEW_TO_SELECTED_H
#include "editor/window/menubar/menubaritem.h"


class MAlignViewToSelectedMenubarItem : public MMenubarItem {
    DEFINE_OBJECT_SUBCLASS(MAlignViewToSelectedMenubarItem)
public:
    [[nodiscard]] int getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_EDIT + 1; }
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //ALIGN_VIEW_TO_SELECTED_H
