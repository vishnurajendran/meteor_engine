//
// menubaritem.h
//
#ifndef MENUBARITEM_H
#define MENUBARITEM_H
#include "core/object/object.h"

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

public:
    static constexpr int PRIORITY_HIGHEST = -999;
    static constexpr int PRIORITY_REGULAR = 0;
    static constexpr int PRIORITY_LOWEST  = 999;
};

#endif // MENUBARITEM_H