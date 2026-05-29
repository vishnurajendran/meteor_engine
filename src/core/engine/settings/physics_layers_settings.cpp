//
// physics_layers_settings.cpp
//

#include "physics_layers_settings.h"
#include <sstream>

// ---------------------------------------------------------------------------
// Statics
// ---------------------------------------------------------------------------

SString MPhysicsLayersSettings::defaultRow()
{
    // "1 1 1 1 ... 1" with 32 ones separated by spaces.
    SString row;
    for (int i = 0; i < 32; ++i)
    {
        if (i > 0) row += " ";
        row += "1";
    }
    return row;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MPhysicsLayersSettings::MPhysicsLayersSettings()
{
    // Layer names: 0 = Default, 1-31 = Unnamed_Layer_N
    std::vector<SString> names(32);
    names[0] = "Default";
    for (int i = 1; i < 32; ++i)
        names[i] = SString::format("Unnamed_Layer_{0}", i);
    layerNames.set(std::move(names));

    // Collision matrix: all layers collide with all layers by default.
    std::vector<SString> rows(32);
    for (int i = 0; i < 32; ++i)
        rows[i] = defaultRow();
    collisionMatrix.set(std::move(rows));

    // Parse strings into bitmasks.
    rebuildParsedMasks();
}

// ---------------------------------------------------------------------------
// Layer name queries
// ---------------------------------------------------------------------------

const SString& MPhysicsLayersSettings::getLayerName(unsigned int index) const
{
    const auto& names = layerNames.get();
    return names[std::min(index, 31u)];
}

unsigned int MPhysicsLayersSettings::getIndexForName(const SString& name) const
{
    const auto& names = layerNames.get();
    for (unsigned int i = 0; i < static_cast<unsigned int>(names.size()); ++i)
        if (names[i] == name) return i;
    return 0;
}

// ---------------------------------------------------------------------------
// Collision matrix queries
// ---------------------------------------------------------------------------

bool MPhysicsLayersSettings::shouldLayersCollide(unsigned int layerA, unsigned int layerB) const
{
    layerA = std::min(layerA, 31u);
    layerB = std::min(layerB, 31u);
    // AND both directions so an asymmetric matrix takes the stricter path.
    return ((parsedMasks[layerA] >> layerB) & 1u) &&
           ((parsedMasks[layerB] >> layerA) & 1u);
}

// ---------------------------------------------------------------------------
// Serialisation hook
// ---------------------------------------------------------------------------

void MPhysicsLayersSettings::onDeserialise(const pugi::xml_node& /*node*/)
{
    // Pad to 32 rows if the file had fewer (forward compat).
    auto rows = collisionMatrix.get();
    while (rows.size() < 32)
        rows.push_back(defaultRow());
    if (rows.size() != collisionMatrix.size())
        collisionMatrix.set(std::move(rows));

    rebuildParsedMasks();
}

// ---------------------------------------------------------------------------
// Parsing
// ---------------------------------------------------------------------------

void MPhysicsLayersSettings::rebuildParsedMasks()
{
    const auto& rows = collisionMatrix.get();

    for (int i = 0; i < 32; ++i)
    {
        uint32_t mask = 0;

        if (i < static_cast<int>(rows.size()))
        {
            // Parse "1 0 1 1 0 ..." — space-separated 0/1 tokens.
            std::istringstream stream(rows[i]);
            int bit = 0;
            int value = 0;
            while (stream >> value && bit < 32)
            {
                if (value != 0) mask |= (1u << bit);
                ++bit;
            }

            // If the string had fewer than 32 tokens, remaining bits stay 0
            // (conservative — no collision with unlisted layers).
        }
        else
        {
            // Missing row — default to all-collide.
            mask = ~0u;
        }

        parsedMasks[i] = mask;
    }
}