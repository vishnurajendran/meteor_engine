//
// clipboard_menu_items.h
//
// Edit/Cut (Ctrl+X), Edit/Copy (Ctrl+C), Edit/Paste (Ctrl+V) for entities.
// See MEntityClipboard.
//

#ifndef CLIPBOARD_MENU_ITEMS_H
#define CLIPBOARD_MENU_ITEMS_H
#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"

class MCutSelectedMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MCutSelectedMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_EDIT - 13; }
    [[nodiscard]] SString getPath()     const override { return "Edit/Cut"; }
    [[nodiscard]] MShortcutBinding getShortcut() const override { return {EKeyCode::X, true}; }
    void onSelect() override;
private:
    static bool registered;
};

class MCopySelectedMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MCopySelectedMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_EDIT - 12; }
    [[nodiscard]] SString getPath()     const override { return "Edit/Copy"; }
    [[nodiscard]] MShortcutBinding getShortcut() const override { return {EKeyCode::C, true}; }
    void onSelect() override;
private:
    static bool registered;
};

class MPasteMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MPasteMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_EDIT - 11; }
    [[nodiscard]] SString getPath()     const override { return "Edit/Paste"; }
    [[nodiscard]] MShortcutBinding getShortcut() const override { return {EKeyCode::V, true}; }
    void onSelect() override;
private:
    static bool registered;
};

#endif // CLIPBOARD_MENU_ITEMS_H
