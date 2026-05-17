//
// shortcutlistener.cpp
//
#include "shortcut_listener.h"
#include "editor/window/menubar/menubaritem.h"


// -- MShortcutBinding ---------------------------------------------------------

std::string MShortcutBinding::getDisplayText() const
{
    if (!isValid()) return {};

    std::string result;
    if (ctrl)  result += "Ctrl+";
    if (alt)   result += "Alt+";
    if (shift) result += "Shift+";
    result += SInput::getKeyName(key);
    return result;
}

// -- MShortcutListener --------------------------------------------------------

std::vector<MShortcutListener::Entry> MShortcutListener::entries;

void MShortcutListener::registerShortcut(MMenubarItem* item, const MShortcutBinding& binding)
{
    if (!item || !binding.isValid()) return;

    for (const auto& e : entries)
        if (e.item == item) return;

    entries.push_back({ item, binding });
}

void MShortcutListener::poll()
{
    for (const auto& [item, binding] : entries)
    {
        // Modifier state must match exactly so that e.g. Ctrl+S does not
        // also trigger a bare "S" binding.
        if (binding.ctrl  != SInput::isCtrlDown())  continue;
        if (binding.shift != SInput::isShiftDown()) continue;
        if (binding.alt   != SInput::isAltDown())   continue;

        if (SInput::pressedThisFrame(binding.key))
        {
            item->onSelect();
            // Fire only the first match per frame to prevent double dispatch.
            return;
        }
    }
}

void MShortcutListener::clear()
{
    entries.clear();
}