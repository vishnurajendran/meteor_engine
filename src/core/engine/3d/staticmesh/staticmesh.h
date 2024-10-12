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
private:
    std::vector<SVertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
public:
    MStaticMesh(std::vector<SVertex> vertices, std::vector<unsigned int> indices);
    void prepareMesh();
    void draw();
};



#endif //STATICMESH_H
