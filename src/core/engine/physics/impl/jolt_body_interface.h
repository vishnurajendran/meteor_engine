//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_BODY_INTERFACE_H
#define JOLT_BODY_INTERFACE_H
#include <Jolt/Jolt.h>
#include "Jolt/Physics/Body/BodyID.h"

class IJoltCollisionBodyInterface
{
public:
    IJoltCollisionBodyInterface() = default;
    virtual ~IJoltCollisionBodyInterface() = default;

    [[nodiscard]] virtual uint32_t getJoltBodyIndex() const = 0;
    [[nodiscard]] virtual JPH::BodyID getBodyID() const = 0;
};

#endif //JOLT_BODY_INTERFACE_H
