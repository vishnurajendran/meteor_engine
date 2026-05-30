//
// Created by ssj5v on 21-05-2026.
//

#ifndef BODY_ACTIVATION_LISTENER_H
#define BODY_ACTIVATION_LISTENER_H
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include "core/utils/logger.h"

using namespace JPH;

class MBodyActivationListener : public BodyActivationListener
{
public:
    void OnBodyActivated(const BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        MLOG("MBodyActivationListener::OnBodyActivated");
    }

    void OnBodyDeactivated(const BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        MLOG("MBodyActivationListener::OnBodyDeactivated");
    }
};
#endif //BODY_ACTIVATION_LISTENER_H
