//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef RENDER_ITEM_H
#define RENDER_ITEM_H

#include <glm/mat4x4.hpp>

class MMaterial;

// Plain data submitted by a drawable each frame.
// The pipeline collects these once in preRender(); each stage decides
// independently what to do with them (bind material, skip it, etc.).
//
// Geometry contract:
//   ebo != 0 : glDrawElements using indexCount
//   ebo == 0 : glDrawArrays   using vertexCount
struct SRenderItem
{
    // Geometry
    unsigned int vao = 0;
    unsigned int ebo = 0;
    int vertexCount  = 0;
    int indexCount   = 0;

    // Transform - world-space model matrix.
    // Stages set this as the "u_model" uniform after binding their shader.
    glm::mat4 transform    = glm::mat4(1.f);

    // Material - may be nullptr.
    // Depth stage ignores it; opaque stage calls material->bindMaterial().
    MMaterial* material    = nullptr;

    // Sorting hint - stages may sort items before drawing.
    int sortOrder    = 0;
};

#endif // RENDER_ITEM_H