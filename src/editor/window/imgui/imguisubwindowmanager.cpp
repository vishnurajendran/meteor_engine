//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "imguisubwindowmanager.h"

std::vector<MImGuiSubWindow*> MImGuiSubWindowManager::windows;

void MImGuiSubWindowManager::add(MImGuiSubWindow *subWindow) {
    windows.push_back(subWindow);
}

void MImGuiSubWindowManager::remove(MImGuiSubWindow *subWindow) {
    windows.erase(std::remove(windows.begin(), windows.end(), subWindow), windows.end());
}

std::vector<MImGuiSubWindow *> MImGuiSubWindowManager::getSubWindows() {
    return windows;
}
