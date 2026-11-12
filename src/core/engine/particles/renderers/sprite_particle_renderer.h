//
// Sprite particle renderer — one camera-facing quad per particle.
//
#pragma once
#ifndef METEOR_ENGINE_SPRITE_PARTICLE_RENDERER_H
#define METEOR_ENGINE_SPRITE_PARTICLE_RENDERER_H

#include "core/engine/particles/particle_renderer.h"

// draws particles instanced using data
class MSpriteParticleRenderer : public MParticleRendererBase
{
    DEFINE_OBJECT_SUBCLASS(MSpriteParticleRenderer)
public:
    MSpriteParticleRenderer() = default;
    ~MSpriteParticleRenderer() override;

protected:
    bool syncGeometry() override;
    void destroyGeometry() override;
    [[nodiscard]] bool useBillboards() const override { return true; }

private:
    // A unit quad (-0.5..0.5 in XY) owned by this renderer. Not shared with
    // MSpriteEntity's quad: our VAO also carries the instance attributes.
    unsigned int quadVBO = 0;
    unsigned int quadEBO = 0;
};

#endif // METEOR_ENGINE_SPRITE_PARTICLE_RENDERER_H
