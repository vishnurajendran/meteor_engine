//
// Created by ssj5v on 05-10-2024.
//

#ifndef METEORDRAWABLES_H
#define METEORDRAWABLES_H
#include <vector>

#include "core/object/object.h"

class IDrawCallSubmitable;

class MMeteorDrawables : public MObject {
private:
    static std::vector<IDrawCallSubmitable*> drawables;
public:
    static void addToSubmitables(IDrawCallSubmitable* instance);
    static void removeFromSubmitables(IDrawCallSubmitable* instance);
    static void requestDrawCalls();
};

#endif //METEORDRAWABLES_H
