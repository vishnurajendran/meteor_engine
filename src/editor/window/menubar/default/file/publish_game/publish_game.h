//
// publish_game.h
//
// File / Publish Game...
//
// Opens the publish settings dialog (output folder, game name, version,
// publisher, startup scene, pak size). Publish starts MGamePublisher and
// shows a modal progress popup that blocks the editor until the build
// finishes, fails or is cancelled. Settings are saved to the .mtproj.
//

#ifndef PUBLISH_GAME_MENUBAR_ITEM_H
#define PUBLISH_GAME_MENUBAR_ITEM_H

#include "editor/window/menubar/menubaritem.h"

class MPublishGameMenubarItem : public MMenubarItem
{
    DEFINE_OBJECT_SUBCLASS(MPublishGameMenubarItem)
public:
    // After Save Scene As, before Quit.
    [[nodiscard]] int     getPriority() const override { return MENU_PRIORITY_BASE_FILE + 10; }
    [[nodiscard]] SString getPath()     const override { return "File/Publish Game..."; }
    void onSelect()  override;
    void drawPopup() override;

private:
    void loadSettingsFromProject();
    void drawSettingsDialog();
    void drawProgressPopup();
    bool startPublish();

    static bool registered;
};

#endif // PUBLISH_GAME_MENUBAR_ITEM_H
