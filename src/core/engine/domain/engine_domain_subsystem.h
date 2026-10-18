//
// engine_domain_subsystem.h
//
// A stack of engine domains. Systems always use the top one:
//
//   DOMAIN_1  <- runtime: pushed when the editor hits Play, removed on Stop
//   DOMAIN_0  <- default: created at init. The editor's sandbox, or the game
//                itself in a player build (where it is the only domain)
//
// Domains can get heavy, so the stack is capped at MAX_DOMAINS. Editor use
// needs two.
//

#ifndef ENGINE_DOMAIN_SUBSYSTEM_H
#define ENGINE_DOMAIN_SUBSYSTEM_H

#include <memory>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "engine_domain.h"

class MEngineDomainSubsystem : public IEngineSubSystem
{
public:
    static constexpr size_t MAX_DOMAINS = 4;

    MEngineDomainSubsystem() = default;
    ~MEngineDomainSubsystem() override = default;

    void init() override;      // creates DOMAIN_0
    void cleanup() override;   // destroys every domain, top first - DOMAIN_0 included

    // The top of the stack, or nullptr before init / after cleanup.
    [[nodiscard]] MEngineDomain* getCurrentActiveDomain() const;

    // Pushes a new domain on top. Returns false at MAX_DOMAINS.
    bool addNewDomain();

    // Destroys the top domain. Returns false when only DOMAIN_0 is left -
    // the default domain lives as long as the subsystem.
    bool removeTopDomain();

private:
    std::vector<std::unique_ptr<MEngineDomain>> domains;
};

#endif // ENGINE_DOMAIN_SUBSYSTEM_H
