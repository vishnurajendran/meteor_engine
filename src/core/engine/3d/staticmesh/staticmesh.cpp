//
// Created by ssj5v on 29-09-2024.
//

#include <GL/glew.h>
#include "staticmesh.h"
#include "core/utils/logger.h"

MStaticMesh::MStaticMesh(std::vector<SVertex> vertices, std::vector<unsigned int> indices)
{
    this->vertices = std::move(vertices);
    this->indices = std::move(indices);
    prepareMesh();
}

void MStaticMesh::prepareMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout(location = 0) — position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), (void*)offsetof(SVertex, Position));

    // layout(location = 1) — normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex), (void*)offsetof(SVertex, Normal));

    // layout(location = 2) — texcoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex), (void*)offsetof(SVertex, TexCoords));

    glBindVertexArray(0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        MERROR(STR("MStaticMesh::prepareMesh — GL error: ") + std::to_string(err));
}

void MStaticMesh::draw()
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}