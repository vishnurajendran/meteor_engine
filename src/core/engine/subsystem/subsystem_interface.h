//
// Created by ssj5v on 18-05-2026.
//

#ifndef SUBSYSTEM_INTERFACE_H
#define SUBSYSTEM_INTERFACE_H

class IEngineSubSystem
{
public:
    virtual ~IEngineSubSystem() = default;
    virtual void init() = 0;
    virtual void cleanup() = 0;
    virtual bool canTick() { return false; }
    virtual void tick(float deltaTime) {};
};

#endif //SUBSYSTEM_INTERFACE_H
