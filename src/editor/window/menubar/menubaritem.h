//
// menubaritem.h
//
#ifndef MENUBARITEM_H
#define MENUBARITEM_H
#include "core/object/object.h"
#include "editor/app/shortcut_listener.h"

class MMenubarItem : public MObject
{
public:
    [[nodiscard]] virtual int     getPriority() const = 0;
    [[nodiscard]] virtual SString getPath()     const = 0;
    virtual void onSelect() = 0;

    // Override to draw any popup/dialog that belongs to this item.
    // Called every frame from MImGuiWindow::drawGUI via drawAllPopups().
    // Default is a no-op so items that have no popup need not override.
    virtual void drawPopup() {}

    // Override to declare a keyboard shortcut for this item.
    // Return a binding with key == EKeyCode::Unknown (the default) to
    // indicate that no shortcut is assigned.
    [[nodiscard]] virtual MShortcutBinding getShortcut() const { return {}; }

public:
    static constexpr int PRIORITY_HIGHEST = -999;
    static constexpr int PRIORITY_REGULAR = 0;
    static constexpr int PRIORITY_LOWEST  = 999;
};

#endif // MENUBARITEM_H