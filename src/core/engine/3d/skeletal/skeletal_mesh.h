//
// skeletal_mesh.h
//
// MSkeletalMesh - one skinned sub-mesh on the GPU. Shared, read-only input
// for compute skinning; every entity that draws it skins into its own output
// buffer (see MSkeletalMeshEntity), so the bind pose here is never touched.
//
//   bind-pose vertices   8 floats per vertex (position, normal, UV) - the
//                        packed SVertex layout. Also drawn directly through
//                        getBindVAO() for thumbnails.
//   influence offsets    vertexCount + 1 uints: vertex v's influences are
//                        [offsets[v], offsets[v+1])
//   influences           packed { uint joint; float weight; }, sorted by
//                        weight per vertex, weights summing to 1
//   indices              triangle list
//
#pragma once
#ifndef SKELETAL_MESH_H
#define SKELETAL_MESH_H

#include <cstdint>
#include <vector>

#include "core/engine/3d/common/vertex.h"
#include "core/object/object.h"

struct SBoneInfluence
{
    uint32_t joint  = 0;
    float    weight = 0.0f;
};
static_assert(sizeof(SBoneInfluence) == 8, "SBoneInfluence must match the shader's Influence struct");
static_assert(sizeof(SVertex) == 32, "SVertex must stay 8 packed floats - the skinning shader indexes it by hand");

// Points attribute locations 0-2 (position, normal, UV) at the GL_ARRAY_BUFFER
// currently bound, in the SVertex layout. Call with the target VAO bound.
void setupSVertexAttributes();

// False once the window (and its GL context) is gone - objects freed during
// shutdown skip their glDelete* calls then; the driver reclaims everything.
bool isGLContextCurrent();

class MSkeletalMesh : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MSkeletalMesh)
public:
    MSkeletalMesh(std::vector<SVertex> bindVertices,
                  std::vector<uint32_t> influenceOffsets,
                  std::vector<SBoneInfluence> influences,
                  std::vector<unsigned int> indices);
    ~MSkeletalMesh() override;

    MSkeletalMesh(const MSkeletalMesh&) = delete;
    MSkeletalMesh& operator=(const MSkeletalMesh&) = delete;

    [[nodiscard]] const std::vector<SVertex>&        getVertices()         const { return vertices; }
    [[nodiscard]] const std::vector<uint32_t>&       getInfluenceOffsets() const { return offsets; }
    [[nodiscard]] const std::vector<SBoneInfluence>& getInfluences()       const { return influences; }
    [[nodiscard]] const std::vector<unsigned int>&   getIndices()          const { return indices; }

    [[nodiscard]] int getVertexCount() const { return static_cast<int>(vertices.size()); }
    [[nodiscard]] int getIndexCount()  const { return static_cast<int>(indices.size()); }

    // GL objects (0 if the upload failed).
    [[nodiscard]] unsigned int getBindPoseBuffer() const { return bindBuffer; }
    [[nodiscard]] unsigned int getOffsetsBuffer()  const { return offsetsBuffer; }
    [[nodiscard]] unsigned int getInfluenceBuffer()const { return influenceBuffer; }
    [[nodiscard]] unsigned int getEBO()            const { return ebo; }
    // Bind pose, unskinned - for thumbnails and previews.
    [[nodiscard]] unsigned int getBindVAO()        const { return bindVAO; }

private:
    void upload();

    std::vector<SVertex>        vertices;
    std::vector<uint32_t>       offsets;
    std::vector<SBoneInfluence> influences;
    std::vector<unsigned int>   indices;

    unsigned int bindBuffer      = 0;
    unsigned int offsetsBuffer   = 0;
    unsigned int influenceBuffer = 0;
    unsigned int ebo             = 0;
    unsigned int bindVAO         = 0;
};

#endif // SKELETAL_MESH_H
