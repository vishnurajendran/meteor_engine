//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "windowmanager.h"
#include "core/window/imgui/imguiwindow.h"

MWindow *MWindowManager::getSimpleWindow(const SString& title, int x, int y, int fps) {
    return new MWindow(title, x,y,fps);
}

MWindow *MWindowManager::getImGuiWindow(const SString& title, int x, int y, int fps) {
    return new MImGuiWindow(title, x, y, fps);
}
