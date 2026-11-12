//
// Shadow inputs - binds this frame's shadow maps for any lit shader.
//

#pragma once
#ifndef SHADOW_INPUTS_H
#define SHADOW_INPUTS_H

class IRenderPipeline;

// Every shader that calls meteorSurfaceLighting() (meteor_lighting_utils.glsl)
// samples the directional, spot and point shadow maps. MLightingStage and
// MTransparentStage both bind them through here, so the texture units and
// uniform names are defined in one place.
//
// Texture units used: FIRST_UNIT .. FIRST_UNIT + UNIT_COUNT - 1.
// Bind other textures from UNIT_COUNT upwards.
namespace MShadowInputs
{
    inline constexpr int UNIT_DIRECTIONAL = 0;
    inline constexpr int UNIT_SPOT_BASE   = 1;   // 1..4
    inline constexpr int UNIT_POINT_BASE  = 5;   // 5..8
    inline constexpr int UNIT_COUNT       = 9;

    // Binds shadow maps and sets the shadow uniforms on `program`, which
    // must be the currently bound program. Safe when there are no shadows
    // (sets hasShadow / hasDirShadow to 0).
    void bind(IRenderPipeline* pipeline, unsigned int program);

    // Unbinds the shadow texture units. Leaves GL_TEXTURE0 active.
    void unbind();
}

#endif // SHADOW_INPUTS_H
