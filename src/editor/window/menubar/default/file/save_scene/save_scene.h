//
// save_scene.h
//

#ifndef SAVE_SCENE_MENUBAR_ITEM_H
#define SAVE_SCENE_MENUBAR_ITEM_H
#include "core/engine/input/input.h"
#include "editor/window/menubar/menubaritem.h"

// -- File / Save Scene ---------------------------------------------------------
// Saves to the known path immediately. If no path is known (new unsaved scene),
// falls through to the Save As dialog automatically.

class MSaveSceneMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MSaveSceneMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MMenubarItem::MENU_PRIORITY_BASE_FILE + 2; }
    [[nodiscard]] SString getPath()     const override;
    void onSelect()  override;
    void drawPopup() override;   // Save As dialog, only shown when path is unknown

    [[nodiscard]] MShortcutBinding getShortcut() const override
    {
        return {EKeyCode::S, true};
    };

private:
    static bool registered;
};

// -- File / Save Scene As ------------------------------------------------------
// Always opens the file browser regardless of whether a path is already set.

class MSaveSceneAsMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_CLASS(MSaveSceneAsMenubarItem)
public:
    [[nodiscard]] int     getPriority() const override { return MENU_PRIORITY_BASE_FILE + 3;}
    [[nodiscard]] SString getPath()     const override;
    void onSelect()  override;
    void drawPopup() override;

private:
    static bool registered;
};

#endif // SAVE_SCENE_MENUBAR_ITEM_H