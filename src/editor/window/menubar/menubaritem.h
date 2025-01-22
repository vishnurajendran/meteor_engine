//
// Created by ssj5v on 21-01-2025.
//

#ifndef MENUBARITEM_H
#define MENUBARITEM_H
#include "core/object/object.h"


class MMenubarItem : public MObject {
public:
    [[nodiscard]] virtual int getPriority() const = 0;
    [[nodiscard]] virtual SString getPath() const = 0;
    virtual void onSelect() = 0;
public:
    static constexpr int PRIORITY_HIGHEST = -999;
    static constexpr int PRIORITY_REGULAR = 0;
    static constexpr int PRIORITY_LOWEST = 999;
};



#endif //MENUBARITEM_H
