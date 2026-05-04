//
// entity_type_registry.h
//
// Maps MTypeInfo (hash) -> factory lambda.
// Hash-keyed lookup — type dispatch during scene load is a single integer compare.
// Also keeps a name->hash index so XML string type names resolve without string scanning.
//

#ifndef ENTITY_TYPE_REGISTRY_H
#define ENTITY_TYPE_REGISTRY_H

#include <functional>
#include <string>
#include <unordered_map>
#include <iostream>
#include "core/object/type_info.h"

class MSpatialEntity;
class MEntityTypeRegistry
{
public:
    using FactoryFn = std::function<MSpatialEntity*()>;

    static MEntityTypeRegistry& get()
    {
        static MEntityTypeRegistry instance;
        return instance;
    }

    //  Registration
    // Called automatically by IMPLEMENT_CLASS — never call manually.
    //
    void registerType(const MTypeInfo& info, FactoryFn factory)
    {
        if (byHash.count(info.hash))
        {
            std::cerr << "[EntityTypeRegistry] Duplicate type: '" << info.name << "'\n";
            return;
        }
        byHash[info.hash]   = std::move(factory);
        nameToHash[info.name] = info.hash;  // for XML string -> hash lookup
    }

    //  Create by MTypeInfo (runtime, hash key — O(1))
    MSpatialEntity* create(const MTypeInfo& info) const
    {
        return createByHash(info.hash);
    }

    // Create by class name string (used when reading XML)
    // Converts name -> hash once, then does a hash lookup — still O(1).
    //
    MSpatialEntity* create(const std::string& typeName) const
    {
        auto it = nameToHash.find(typeName);
        if (it == nameToHash.end())
        {
            MERROR(SString::format("[EntityTypeRegistry] Unknown type: '{0}'",typeName));
            return nullptr;
        }
        return createByHash(it->second);
    }

    //  Query
    [[nodiscard]] bool hasType(const MTypeInfo& info) const
    {
        return byHash.count(info.hash) > 0;
    }

    [[nodiscard]] bool hasType(const std::string& name) const
    {
        return nameToHash.count(name) > 0;
    }

private:
    MEntityTypeRegistry() = default;

    MSpatialEntity* createByHash(uint64_t hash) const
    {
        auto it = byHash.find(hash);
        if (it == byHash.end())
        {
            MERROR(SString::format("[EntityTypeRegistry] No factory for hash: {0}",hash));
            return nullptr;
        }
        return it->second();
    }

    std::unordered_map<uint64_t,    FactoryFn>  byHash;
    std::unordered_map<std::string, uint64_t>   nameToHash;
};

#endif // ENTITY_TYPE_REGISTRY_H