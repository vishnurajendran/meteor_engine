//
// Fullscreen quad - shared NDC quad for screen-space passes.
//

#include <GL/glew.h>
#include "fullscreen_quad.h"

static const float k_quadVerts[] = {
    -1.f,-1.f,0.f, 0.f,0.f,   1.f,-1.f,0.f, 1.f,0.f,
     1.f, 1.f,0.f, 1.f,1.f,  -1.f, 1.f,0.f, 0.f,1.f,
};
static const unsigned int k_quadIdx[] = { 0,1,2, 0,2,3 };

void SFullscreenQuad::create()
{
    if (vao) return;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadVerts), k_quadVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(k_quadIdx), k_quadIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

void SFullscreenQuad::destroy()
{
    if (ebo) { glDeleteBuffers(1, &ebo);      ebo = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo);      vbo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
}

void SFullscreenQuad::draw() const
{
    if (!vao) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
