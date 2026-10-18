//
// engine_domain_subsystem.cpp
//

#include "engine_domain_subsystem.h"

#include "core/utils/logger.h"

void MEngineDomainSubsystem::init()
{
    if (!domains.empty())
        return;

    domains.push_back(std::make_unique<MEngineDomain>(0));
    MLOG("MEngineDomainSubsystem:: DOMAIN_0 created");
}

void MEngineDomainSubsystem::cleanup()
{
    // Top first, the reverse of creation.
    while (!domains.empty())
        domains.pop_back();
    MLOG("MEngineDomainSubsystem:: all domains destroyed");
}

MEngineDomain* MEngineDomainSubsystem::getCurrentActiveDomain() const
{
    return domains.empty() ? nullptr : domains.back().get();
}

bool MEngineDomainSubsystem::addNewDomain()
{
    if (domains.size() >= MAX_DOMAINS)
    {
        MERROR(SString::format("MEngineDomainSubsystem:: cannot add a domain - the limit is {0}",
                               SString::fromInt(static_cast<int>(MAX_DOMAINS))));
        return false;
    }

    const auto index = static_cast<uint32_t>(domains.size());
    domains.push_back(std::make_unique<MEngineDomain>(index));
    MLOG(SString::format("MEngineDomainSubsystem:: DOMAIN_{0} created", SString::fromInt(static_cast<int>(index))));
    return true;
}

bool MEngineDomainSubsystem::removeTopDomain()
{
    if (domains.size() <= 1)
    {
        MWARN("MEngineDomainSubsystem:: cannot remove DOMAIN_0");
        return false;
    }

    const uint32_t index = domains.back()->getIndex();
    domains.pop_back();
    MLOG(SString::format("MEngineDomainSubsystem:: DOMAIN_{0} removed", SString::fromInt(static_cast<int>(index))));
    return true;
}
