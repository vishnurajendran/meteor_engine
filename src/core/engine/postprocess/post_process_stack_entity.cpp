//
// Post-process stack entity - the scene's handle on post-processing.
//

#include <GL/glew.h>
#include "post_process_stack_entity.h"

#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_stack.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager_interface.h"
#include "core/utils/logger.h"

IMPLEMENT_SPATIAL_CLASS(MPostProcessStackEntity)

MPostProcessStackEntity::MPostProcessStackEntity()
{
    claimStack();
}

MPostProcessStack* MPostProcessStackEntity::getStack()
{
    auto* renderer = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();
    if (!renderer || !renderer->getPipeline()) return nullptr;
    return renderer->getPipeline()->getAttachment<MPostProcessStack>();
}

void MPostProcessStackEntity::claimStack()
{
    auto* stack = getStack();
    if (!stack) return;

    if (stack->getOwner() && stack->getOwner() != this)
    {
        MWARN("Only one Post Process Stack per scene is supported - the newest one takes over.");
        stack->clear();
    }
    stack->setOwner(this);
    stack->setEnabled(getEnabled());
}

void MPostProcessStackEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    // Only ticks while enabled in the hierarchy; onDisable covers turning off.
    if (auto* stack = getStack(); stack && stack->getOwner() == this)
        stack->setEnabled(true);
}

void MPostProcessStackEntity::onEnable()
{
    MSpatialEntity::onEnable();
    if (auto* stack = getStack(); stack && stack->getOwner() == this)
        stack->setEnabled(true);
}

void MPostProcessStackEntity::onDisable()
{
    MSpatialEntity::onDisable();
    if (auto* stack = getStack(); stack && stack->getOwner() == this)
        stack->setEnabled(false);
}

void MPostProcessStackEntity::onExit()
{
    // Leaving the scene (unload, delete, play-mode restore): drop this
    // scene's effects - unless a newer stack entity has already taken over.
    if (auto* stack = getStack())
        stack->releaseIfOwner(this);
    MSpatialEntity::onExit();
}

void MPostProcessStackEntity::onSerialise(pugi::xml_node& node)
{
    MSpatialEntity::onSerialise(node);

    pugi::xml_node pp = node.append_child("post_process");

    auto* stack = getStack();
    if (!stack || stack->getOwner() != this) return;

    for (const SString& type : stack->getEffectTypeNames())
    {
        auto* settings = stack->getSettings(type);
        if (!settings) continue;
        pugi::xml_node effectNode = pp.append_child("effect");
        effectNode.append_attribute("type").set_value(type.c_str());
        settings->serialiseToNode(effectNode);
    }
}

void MPostProcessStackEntity::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    // The constructor already claimed the stack (emptying it if another
    // entity owned it), so effects go into a stack that belongs to us.
    auto* stack = getStack();
    if (!stack || stack->getOwner() != this) return;

    const pugi::xml_node pp = node.child("post_process");
    if (!pp) return;

    for (pugi::xml_node effectNode : pp.children("effect"))
    {
        const SString type = effectNode.attribute("type").as_string();
        if (!stack->hasEffect(type) && !stack->addEffect(type))
        {
            MWARN("Post Process Stack: skipping unknown effect '" + type + "'");
            continue;
        }
        if (auto* settings = stack->getSettings(type))
            settings->deserialiseFromNode(effectNode);
    }

    stack->setEnabled(getEnabled());
}
