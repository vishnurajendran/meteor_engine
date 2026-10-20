//
// Created by ssj5v on 27-01-2025.
//

#ifndef LIGHT_TYPE_H
#define LIGHT_TYPE_H

#include "core/engine/scripting/script_binding_macros.h"

SCRIPT_BIND_ENUM()
enum class ELightType
{
    Ambient,
    Directional,
    Point,
    Spot
};

#endif //LIGHT_TYPE_H
