//
// Created by ssj5v on 22-01-2025.
//

#ifndef ALIGN_OBJECT_TO_VIEW_H
#define ALIGN_OBJECT_TO_VIEW_H
#include "editor/window/menubar/menubaritem.h"


class MAlignObjectToViewMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //ALIGN_OBJECT_TO_VIEW_H
