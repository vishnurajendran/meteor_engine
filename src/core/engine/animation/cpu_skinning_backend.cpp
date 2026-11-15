#include <GL/glew.h>
#include "cpu_skinning_backend.h"

#include <span>

#include "core/engine/3d/skeletal/skeletal_mesh.h"
#include "core/utils/parallel_for.h"

void MCpuSkinningBackend::skin(const std::vector<SMatrix4>& matrices, const std::vector<SSkinningJob>& jobs)
{
    if (jobs.empty())
        return;

    if (scratch.size() < jobs.size())
        scratch.resize(jobs.size());

    // One task per sub-mesh: coarse enough for the job system's claim lock.
    std::vector<uint32_t> jobIndices(jobs.size());
    for (uint32_t i = 0; i < jobIndices.size(); ++i) jobIndices[i] = i;

    MParallel::forEach(std::span<uint32_t>(jobIndices), [&](uint32_t j)
    {
        const SSkinningJob& job = jobs[j];
        if (!job.mesh) return;

        const auto& bind       = job.mesh->getVertices();
        const auto& offsets    = job.mesh->getInfluenceOffsets();
        const auto& influences = job.mesh->getInfluences();
        auto& out = scratch[j];
        out.resize(bind.size());

        for (size_t v = 0; v < bind.size(); ++v)
        {
            SMatrix4 m(0.0f);
            for (uint32_t i = offsets[v]; i < offsets[v + 1]; ++i)
            {
                const uint32_t bone = job.boneOffset + influences[i].joint;
                if (bone < matrices.size())
                    m += influences[i].weight * matrices[bone];
            }
            out[v].Position  = SVector3(m * SVector4(bind[v].Position, 1.0f));
            SVector3 n       = SMatrix3(m) * bind[v].Normal;
            const float len  = glm::length(n);
            out[v].Normal    = len > 0.0f ? n / len : bind[v].Normal;
            out[v].TexCoords = bind[v].TexCoords;
        }
    }, 1, 2);

    // GL on the main thread only.
    for (size_t j = 0; j < jobs.size(); ++j)
    {
        if (!jobs[j].mesh || !jobs[j].outputBuffer || scratch[j].empty()) continue;
        glBindBuffer(GL_ARRAY_BUFFER, jobs[j].outputBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(scratch[j].size() * sizeof(SVertex)), scratch[j].data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
