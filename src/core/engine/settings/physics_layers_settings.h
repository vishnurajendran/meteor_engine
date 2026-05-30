//
// physics_layers_settings.h
//

#ifndef PHYSICS_LAYERS_SETTINGS_H
#define PHYSICS_LAYERS_SETTINGS_H

#include <cstdint>
#include "core/object/object.h"
#include "core/utils/field_engine_types.h"
#include "data/serialized_class_base.h"
#include "core/utils/sstring.h"

// MPhysicsLayersSettings stores the 32 physics layer name slots and the
// collision matrix for the project. Saved as a standalone XML file
// (e.g. project/settings/physics_layers.xml).
//
// XML format:
//   <root>
//     <layerNames>
//       <item>Default</item>
//       <item>Player</item>
//       ...32 items...
//     </layerNames>
//     <collisionMatrix>
//       <item>1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1</item>
//       <item>1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0</item>
//       ...32 rows...
//     </collisionMatrix>
//   </root>
//
// Row i in collisionMatrix is "which layers does layer i collide with".
// The matrix must be symmetric — if layer A collides with layer B then
// layer B must also collide with layer A. The pair filter AND-gates both
// directions as a safety net, so an asymmetric matrix results in the
// stricter of the two directions being used.
//
// Default: all layers collide with all layers (all 1s).
class MPhysicsLayersSettings : public SerializedClassBase, public MObject
{
    DEFINE_OBJECT_SUBCLASS(MPhysicsLayersSettings)

    DECLARE_LIST_FIELD(layerNames,       SString, {})
    DECLARE_LIST_FIELD(collisionMatrix,  SString, {})

public:
    MPhysicsLayersSettings();
    ~MPhysicsLayersSettings() override = default;

    // ---- Layer names -------------------------------------------------------

    [[nodiscard]] const SString& getLayerName(unsigned int index) const;
    [[nodiscard]] unsigned int   getIndexForName(const SString& name) const;

    // ---- Collision matrix --------------------------------------------------

    // Returns a pointer to 32 uint32_t bitmasks. Bit N of parsedMasks[layer]
    // is set if layer should collide with N. Valid for the lifetime of this
    // object. Updated on construction and on every deserialise.
    [[nodiscard]] const uint32_t* getParsedCollisionMasks() const { return parsedMasks; }

    // Returns true if layerA and layerB should collide according to the matrix.
    [[nodiscard]] bool shouldLayersCollide(unsigned int layerA, unsigned int layerB) const;

protected:
    void onDeserialise(const pugi::xml_node& node) override;

private:
    // Generates the default all-collide row string: "1 1 1 1 ... 1" (32 ones).
    static SString defaultRow();

    // Parses all 32 collisionMatrix strings into parsedMasks[32].
    void rebuildParsedMasks();

    // Cached bitmasks parsed from collisionMatrix strings.
    // parsedMasks[i] bit N = 1 means layer i collides with layer N.
    uint32_t parsedMasks[32] = {};
};

#endif // PHYSICS_LAYERS_SETTINGS_H