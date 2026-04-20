#include "save_scene.h"

#include "core/engine/scene/scene_io.h"
#include "editor/window/menubar/menubartree.h"

bool MSaveSceneMenubarItem::registered = []()
{
    MMenubarTreeNode::registerItem(new MSaveSceneMenubarItem());
    return true;
}();

int MSaveSceneMenubarItem::getPriority() const
{
    return MMenubarItem::PRIORITY_HIGHEST + 2;
}

SString MSaveSceneMenubarItem::getPath() const
{
    return "File/Save Scene";
}

void MSaveSceneMenubarItem::onSelect()
{
    // Saves back to the file it was opened from.
    // If no path is set yet (new unsaved scene), logs a warning —
    // user should Save As first (or set a path via Open).
    MSceneIO::saveCurrentScene();
}