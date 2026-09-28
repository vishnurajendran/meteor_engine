//
// duplicate_selected.h
//
// Edit/Duplicate (Ctrl+D): duplicates the selected entity and selects the copy.
//

#ifndef DUPLICATE_SELECTED_MENUBAR_ITEM_H
#define DUPLICATE_SELECTED_MENUBAR_ITEM_H
#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"

class MDuplicateSelectedMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MDuplicateSelectedMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_EDIT - 10; }
    [[nodiscard]] SString getPath()     const override { return "Edit/Duplicate"; }
    void onSelect() override;

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::D, true};
    }

private:
    static bool registered;
};

#endif // DUPLICATE_SELECTED_MENUBAR_ITEM_H
