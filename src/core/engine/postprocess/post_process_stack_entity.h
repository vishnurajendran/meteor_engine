//
// Post-process stack entity - the scene's handle on post-processing.
//

#pragma once
#ifndef POST_PROCESS_STACK_ENTITY_H
#define POST_PROCESS_STACK_ENTITY_H

#include "core/engine/entities/spatial/spatial.h"

class MPostProcessStack;

// Holds no effects itself. The main pipeline's MPostProcessStack attachment
// owns effects and settings; this entity
//   - claims the stack for its scene (so leaving the scene clears it),
//   - turns the stack on/off with its own enabled state,
//   - is what the inspector edits and what the scene file saves
//     (onSerialise writes the stack's effects and settings).
//
// One per scene. A second one takes the stack over and logs a warning.
class MPostProcessStackEntity : public MSpatialEntity
{
    DEFINE_SPATIAL_CLASS(MPostProcessStackEntity)
public:
    MPostProcessStackEntity();
    ~MPostProcessStackEntity() override = default;

    // The main pipeline's stack, or nullptr (no renderer / no attachment).
    // Valid for the pipeline's lifetime; fine to call every frame.
    static MPostProcessStack* getStack();

    void onUpdate(float deltaTime) override;
    void onEnable()  override;
    void onDisable() override;
    void onExit()    override;

protected:
    // Saved with the scene through the entity's own serialisation:
    //   <post_process>
    //     <effect type="Tonemapping"> <enabled>true</enabled> <mode>2</mode> ... </effect>
    //   </post_process>
    // Loading adds each effect to the stack by type name, then reads its fields.
    void onSerialise(pugi::xml_node& node) override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    void claimStack();
};

#endif // POST_PROCESS_STACK_ENTITY_H
