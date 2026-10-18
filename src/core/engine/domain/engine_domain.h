//
// engine_domain.h
//
// One level of MEngineDomainSubsystem's stack: the state that must not leak
// between edit time and play time. Today that is the script context; more
// can move in later.
//
// Scenes are deliberately not part of a domain: Play keeps using the scene
// snapshot file, which doubles as a recovery copy if the game crashes.
//

#ifndef ENGINE_DOMAIN_H
#define ENGINE_DOMAIN_H

#include <cstdint>
#include <memory>

class MLuaScriptContext;

class MEngineDomain
{
public:
    explicit MEngineDomain(uint32_t index);
    ~MEngineDomain();

    MEngineDomain(const MEngineDomain&)            = delete;
    MEngineDomain& operator=(const MEngineDomain&) = delete;

    // Position in the stack: 0 is the default domain created at startup.
    [[nodiscard]] uint32_t getIndex() const { return index; }

    [[nodiscard]] MLuaScriptContext& getScriptContext() { return *scriptContext; }

private:
    uint32_t index;
    std::unique_ptr<MLuaScriptContext> scriptContext;
};

#endif // ENGINE_DOMAIN_H
