//
// Created by ssj5v on 14-11-2026.
//
#ifndef VERTEX_H
#define VERTEX_H

#include <core/utils/glmhelper.h>

/// Vertex info struct for mesh
struct SVertex {
    SVector3 Position;
    SVector3 Normal;
    SVector2 TexCoords;
};
#endif //VERTEX_H
