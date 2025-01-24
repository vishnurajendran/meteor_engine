//
// Created by ssj5v on 22-01-2025.
//

#ifndef RESET_CAMERA_TO_ORIGIN_H
#define RESET_CAMERA_TO_ORIGIN_H
#include "editor/window/menubar/menubaritem.h"


class MResetCameraToOriginMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //RESET_CAMERA_TO_ORIGIN_H
