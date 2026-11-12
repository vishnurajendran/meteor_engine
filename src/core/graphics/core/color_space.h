//
// Colour-space helpers.
//
// Meteor renders in linear space:
//   - colour textures are uploaded as sRGB and decoded by the GPU on sample,
//   - colour values authored in the editor (material colours, light colours)
//     are sRGB and are converted to linear before they reach a shader,
//   - every stage writes linear values into HDR buffers,
//   - the final composite encodes linear -> sRGB exactly once.
//
// Anything that writes straight to the screen outside the pipeline (ImGui,
// gizmos drawn after the composite) stays in sRGB and is unaffected.
//

#pragma once
#ifndef METEOR_COLOR_SPACE_H
#define METEOR_COLOR_SPACE_H

#include <cmath>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace MColorSpace
{
    // Exact piecewise sRGB transfer functions (IEC 61966-2-1).
    inline float srgbToLinear(float c)
    {
        return c <= 0.04045f ? c / 12.92f
                             : std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    inline float linearToSrgb(float c)
    {
        return c <= 0.0031308f ? c * 12.92f
                               : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
    }

    inline glm::vec3 srgbToLinear(const glm::vec3& c)
    {
        return { srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b) };
    }

    // Alpha is coverage, not colour - left untouched.
    inline glm::vec4 srgbToLinear(const glm::vec4& c)
    {
        return { srgbToLinear(c.r), srgbToLinear(c.g), srgbToLinear(c.b), c.a };
    }
}

#endif // METEOR_COLOR_SPACE_H
