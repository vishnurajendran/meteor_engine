// Lightweight compile-time type descriptor.
//
//   const MTypeInfo& t = entity->typeInfo();
//   t.name        → "MCamera"          (const char*, zero-cost)
//   t.hash        → 0xA3F2...          (uint64_t, computed at compile time)
//   t == other    → hash compare       (single integer op, no string scan)
//   std::string s = t → implicit conv  (for logging / XML)
//

#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include <cstdint>
#include <string>
#include <functional>

// Deterministic across platforms - same string always produces the same hash.
// Evaluated entirely at compile time when fed a string literal.

namespace MTypeDetail
{
    static constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    static constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

    constexpr uint64_t fnv1a(const char* str, uint64_t hash = FNV_OFFSET)
    {
        return *str ? fnv1a(str + 1, (hash ^ static_cast<uint64_t>(*str)) * FNV_PRIME)
                    : hash;
    }
}

// MTypeInfo

struct MTypeInfo
{
    const char* name;   // human-readable class name (string literal lifetime)
    uint64_t    hash;   // compile-time FNV-1a hash of name

    // Construct from a string literal — both fields set at compile time.
    constexpr MTypeInfo(const char* n, uint64_t h) noexcept
        : name(n), hash(h) {}

    //  Comparisons (hash only — single integer op) 
    constexpr bool operator==(const MTypeInfo& o) const noexcept { return hash == o.hash; }
    constexpr bool operator!=(const MTypeInfo& o) const noexcept { return hash != o.hash; }

    // String output
    // Implicit conversion so it drops into SString / std::string contexts naturally.
    operator std::string() const { return name; }

    // std::unordered_map / unordered_set support
    struct Hash
    {
        constexpr std::size_t operator()(const MTypeInfo& t) const noexcept
        {
            return static_cast<std::size_t>(t.hash);
        }
    };
};

// Helper macro (used inside DEFINE_MOBJECT_CLASS / DEFINE_CLASS)
// Builds an MTypeInfo from a bare class name token.
// The hash is evaluated at compile time — zero runtime cost.
#define MAKE_TYPE_INFO(ClassName) \
    MTypeInfo { #ClassName, MTypeDetail::fnv1a(#ClassName) }

//  Free hash function so MTypeInfo works as an unordered_map key 
namespace std
{
    template<>
    struct hash<MTypeInfo>
    {
        constexpr size_t operator()(const MTypeInfo& t) const noexcept
        {
            return static_cast<size_t>(t.hash);
        }
    };
}

#endif // TYPE_INFO_H