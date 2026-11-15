#include <GL/glew.h>
#include "skeletal_mesh.h"

#include <cstddef>

#include <SFML/Window/Context.hpp>

#include "core/utils/logger.h"

// The vertex layout every stage's shaders read (locations 0-2), shared by the
// bind VAO here and each entity's skinned-output VAO.
void setupSVertexAttributes()
{
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<void*>(offsetof(SVertex, Position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<void*>(offsetof(SVertex, Normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex),
                          reinterpret_cast<void*>(offsetof(SVertex, TexCoords)));
}

bool isGLContextCurrent()
{
    return sf::Context::getActiveContextId() != 0;
}

MSkeletalMesh::MSkeletalMesh(std::vector<SVertex> bindVertices,
                             std::vector<uint32_t> influenceOffsets,
                             std::vector<SBoneInfluence> boneInfluences,
                             std::vector<unsigned int> triangleIndices)
    : vertices(std::move(bindVertices))
    , offsets(std::move(influenceOffsets))
    , influences(std::move(boneInfluences))
    , indices(std::move(triangleIndices))
{
    upload();
}

MSkeletalMesh::~MSkeletalMesh()
{
    if (!isGLContextCurrent())
        return;
    // Deleting an unbound name is fine; 0 is ignored.
    if (bindVAO) glDeleteVertexArrays(1, &bindVAO);
    const GLuint buffers[] = { bindBuffer, offsetsBuffer, influenceBuffer, ebo };
    glDeleteBuffers(4, buffers);
}

void MSkeletalMesh::upload()
{
    // An empty SSBO can't be bound; give every buffer at least one element.
    const SBoneInfluence noInfluence{};

    glGenBuffers(1, &bindBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, bindBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SVertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &offsetsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, offsetsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, offsets.size() * sizeof(uint32_t), offsets.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &influenceBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, influenceBuffer);
    if (influences.empty())
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SBoneInfluence), &noInfluence, GL_STATIC_DRAW);
    else
        glBufferData(GL_SHADER_STORAGE_BUFFER, influences.size() * sizeof(SBoneInfluence), influences.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenVertexArrays(1, &bindVAO);
    glBindVertexArray(bindVAO);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, bindBuffer);
    setupSVertexAttributes();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        MERROR(STR("MSkeletalMesh::upload - GL error: ") + std::to_string(err));
}
