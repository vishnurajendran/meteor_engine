//
// Created by ssj5v on 17-05-2026.
//

#ifndef AUDIOLISTENER_INTERFACE_H
#define AUDIOLISTENER_INTERFACE_H
#include "core/utils/glmhelper.h"

class IAudioListener
{
public:
    virtual ~IAudioListener() = default;
    virtual void init() = 0;
    virtual void cleanup() = 0;

    virtual void tick(const float& deltaTime) = 0;

    // Spatialization
    virtual void setPosition(const SVector3& pos) = 0;
    virtual void setWorldUp(const SVector3& upVector) = 0;
    virtual void setDirection(const SVector3& direction) = 0;
    virtual void setVelocity(const SVector3& velocity) = 0;
};

#endif //AUDIOLISTENER_INTERFACE_H
