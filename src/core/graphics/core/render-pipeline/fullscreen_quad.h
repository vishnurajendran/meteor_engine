//
// Fullscreen quad - shared NDC quad for screen-space passes.
//

#pragma once
#ifndef FULLSCREEN_QUAD_H
#define FULLSCREEN_QUAD_H

// A quad covering the whole viewport in NDC (-1..1), with
//   location 0 = position (vec3), location 2 = uv (vec2)
// matching the standard shader base's vertex attributes. Draw it with the
// model/view/projection uniforms set to identity.
//
// No GL headers here - only in the .cpp.
class SFullscreenQuad
{
public:
    ~SFullscreenQuad() { destroy(); }

    void create();
    void destroy();
    void draw() const;   // binds and unbinds its own VAO

    [[nodiscard]] bool isValid() const { return vao != 0; }

private:
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
};

#endif // FULLSCREEN_QUAD_H
