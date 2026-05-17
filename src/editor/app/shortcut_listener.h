//
// shortcutlistener.h
//
// Centralized keyboard shortcut dispatcher. Menubar items register their key
// combinations here; each frame poll() checks SInput state and fires
// onSelect() on the matching item.
//
#pragma once
#ifndef SHORTCUTLISTENER_H
#define SHORTCUTLISTENER_H

#include <string>
#include <vector>
#include "core/engine/input/input.h"


class MMenubarItem;
// A key combination: one primary key plus modifier flags.
// A binding with key == EKeyCode::Unknown means "no shortcut assigned".
struct MShortcutBinding
{
    EKeyCode key   = EKeyCode::Unknown;
    bool     ctrl  = false;
    bool     shift = false;
    bool     alt   = false;

    [[nodiscard]] bool isValid() const { return key != EKeyCode::Unknown; }

    // Human-readable label for menu hint columns, e.g. "Ctrl+Shift+S".
    [[nodiscard]] std::string getDisplayText() const;
};

class MShortcutListener
{
public:
    // Register a menubar item and its binding.
    // Silently ignores duplicates (same item pointer already present).
    static void registerShortcut(MMenubarItem* item, const MShortcutBinding& binding);

    // Call once per frame after SInput::poll().
    // Checks all registered bindings against current input state.
    // Fires item->onSelect() on the first match found.
    static void poll();

    // Remove all registrations (e.g. when the menubar tree is rebuilt).
    static void clear();

private:
    struct Entry
    {
        MMenubarItem*    item;
        MShortcutBinding binding;
    };

    static std::vector<Entry> entries;
};

#endif // SHORTCUTLISTENER_H