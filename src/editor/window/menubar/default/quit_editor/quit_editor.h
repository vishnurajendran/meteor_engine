//
// Created by ssj5v on 22-01-2025.
//

#ifndef QUITEDITOR_H
#define QUITEDITOR_H
#include "editor/window/menubar/menubaritem.h"


class MQuitEditorMenubarItem : public MMenubarItem {
public:
    [[nodiscard]] int getPriority() const override;
    [[nodiscard]] SString getPath() const override;
    void onSelect() override;
private:
    static bool registered;
};



#endif //QUITEDITOR_H
