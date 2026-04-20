//
// Created by ssj5v on 29-09-2024.
//

#pragma once
#ifndef STATICMESH_H
#define STATICMESH_H

#include <vector>
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

struct SVertex {
    SVector3 Position;
    SVector3 Normal;
    SVector2 TexCoords;
};

class MStaticMesh : public MObject {
public:
    MStaticMesh(std::vector<SVertex> vertices, std::vector<unsigned int> indices);
    [[nodiscard]] const std::vector<SVertex>&       getVertices() const { return vertices; }
    [[nodiscard]] const std::vector<unsigned int>&  getIndices()  const { return indices;  }

    // Accessors for the pipeline's SRenderItem submission.
    [[nodiscard]] unsigned int getVAO()        const { return VAO; }
    [[nodiscard]] unsigned int getEBO()        const { return EBO; }
    [[nodiscard]] int          getIndexCount() const { return static_cast<int>(indices.size()); }
    [[nodiscard]] int          getVertexCount()const { return static_cast<int>(vertices.size()); }

    void prepareMesh();
    void draw();

private:
    std::vector<SVertex>       vertices;
    std::vector<unsigned int>  indices;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
};

#endif // STATICMESH_H