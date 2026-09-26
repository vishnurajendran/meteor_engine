//
// staticmesh.h
//
#pragma once
#ifndef STATICMESH_H
#define STATICMESH_H

#include <vector>
#include "core/object/object.h"
#include "core/utils/glmhelper.h"

/// Vertex info struct for mesh
struct SVertex {
    SVector3 Position;
    SVector3 Normal;
    SVector2 TexCoords;
};

/// Represents a static mesh, in the meteor engine.
SCRIPT_BIND_CLASS()
class MStaticMesh : public MObject
{
    // Non-spatial, non-serialized - just needs typeInfo() for editor/reflection.
    // Mesh data comes from asset loading, not XML fields.
    DEFINE_OBJECT_SUBCLASS(MStaticMesh);

public:
    MStaticMesh(std::vector<SVertex> vertices, std::vector<unsigned int> indices);

    SCRIPT_BIND_FUNC()
    const std::vector<SVertex>&      getVertices()   const { return vertices; }
    SCRIPT_BIND_FUNC()
    const std::vector<unsigned int>& getIndices()    const { return indices; }

    unsigned int getVAO()        const { return VAO; }
    unsigned int getEBO()        const { return EBO; }

    SCRIPT_BIND_FUNC()
    int          getIndexCount() const { return static_cast<int>(indices.size()); }
    SCRIPT_BIND_FUNC()
    int          getVertexCount()const { return static_cast<int>(vertices.size()); }

    void prepareMesh();
    void draw();

private:
    std::vector<SVertex>      vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
};

#endif // STATICMESH_H