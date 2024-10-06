//
// Created by ssj5v on 05-10-2024.
//

#include "viewmanagement.h"

std::vector<MCameraEntity*> MViewManagement::cameras;

void MViewManagement::addCamera(MCameraEntity *camera) {
    if(const auto it = std::ranges::find(cameras, camera); it == cameras.end()) {
        cameras.push_back(camera);
    }
    updateCameraOrder();
}
void MViewManagement::removeCamera(MCameraEntity *camera) {
    const auto it = std::ranges::find(cameras, camera);
    if(it == cameras.end()) {
       return;
    }
    cameras.erase(it);
}

std::vector<MCameraEntity *> MViewManagement::getCameras() {
    return cameras;
}

void MViewManagement::updateCameraOrder() {
     std::sort(cameras.begin(), cameras.end(), [](const MCameraEntity* a, const MCameraEntity* b) {
         return a->getPriority() < b->getPriority();
     });
}
