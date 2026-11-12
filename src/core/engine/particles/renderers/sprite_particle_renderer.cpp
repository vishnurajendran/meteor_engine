//
// Sprite particle renderer — one camera-facing quad per particle.
//

#include <GL/glew.h>
#include "sprite_particle_renderer.h"

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/particles/particle_registry.h"

REGISTER_PARTICLE_RENDERER(MSpriteParticleRenderer, "Sprite")

MSpriteParticleRenderer::~MSpriteParticleRenderer()
{
    // Here, not only in the base destructor: by the time the base runs,
    // our destroyGeometry() override (which frees the quad) is gone.
    cleanup();
}

bool MSpriteParticleRenderer::syncGeometry()
{
    if (!geometry.empty()) return true;   // built once, never changes

    // UV v = 0 at the top, like MSpriteEntity's quad and loaded meshes.
    const SVector3 n(0.0f, 0.0f, 1.0f);
    const SVertex vertices[] = {
        { SVector3(-0.5f, -0.5f, 0.0f), n, SVector2(0.0f, 1.0f) },
        { SVector3( 0.5f, -0.5f, 0.0f), n, SVector2(1.0f, 1.0f) },
        { SVector3( 0.5f,  0.5f, 0.0f), n, SVector2(1.0f, 0.0f) },
        { SVector3(-0.5f,  0.5f, 0.0f), n, SVector2(0.0f, 0.0f) },
    };
    const unsigned int indices[] = { 0, 1, 2,  0, 2, 3 };

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &quadEBO);

    SGeometry g;
    g.vao         = createInstancedVAO(quadVBO, 0);
    g.ebo         = quadEBO;
    g.indexCount  = 6;
    g.vertexCount = 4;

    // Fill the EBO while our VAO is bound so the VAO captures it.
    glBindVertexArray(g.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);

    geometry.push_back(g);
    return true;
}

void MSpriteParticleRenderer::destroyGeometry()
{
    MParticleRendererBase::destroyGeometry();   // VAOs
    if (quadVBO != 0) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadEBO != 0) { glDeleteBuffers(1, &quadEBO); quadEBO = 0; }
}
