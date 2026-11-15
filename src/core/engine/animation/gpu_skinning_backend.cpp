#include <GL/glew.h>
#include "gpu_skinning_backend.h"

#include <algorithm>

#include "core/engine/3d/skeletal/skeletal_mesh.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/graphics/core/shader/compute_shader.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/assetmanagement/assetmanager/asset_manager_subsystem.h"
#include "core/graphics/core/render-pipeline/stages/lighting/light_shader_constants.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include "core/utils/logger.h"

namespace
{
    // Must match local_size_x in skinning.mesl.
    constexpr unsigned int LOCAL_SIZE = 64;

    constexpr const char* SKINNING_SHADER_PATH = "meteor_assets/engine_assets/shaders/internal/skinning.mesl";

    // SSBO blocks in skinning.mesl, in the order of MGpuSkinningBackend::bindings.
    constexpr const char* BLOCK_NAMES[5] = { "BindPose", "Offsets", "Influences", "Bones", "Out" };
}

bool MGpuSkinningBackend::refreshShader()
{
    auto* assetManager = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
    MShaderAsset* asset = assetManager ? assetManager->getAsset<MShaderAsset>(SKINNING_SHADER_PATH).get() : nullptr;
    MComputeShader* shader = asset ? asset->getComputeShader() : nullptr;
    if (!shader)
    {
        if (!warnedMissingShader)
            MERROR(STR("MGpuSkinningBackend:: ") + SKINNING_SHADER_PATH + " is missing or is not a compute shader");
        warnedMissingShader = true;
        activeShader = seenShader = nullptr;
        return false;
    }

    // A hot reload swaps in a new MComputeShader (the old one stays alive in
    // the asset), so a pointer change means a new shader to set up.
    if (shader == seenShader)
        return activeShader != nullptr;
    seenShader = shader;

    if (!shader->getProgramHandle())                 // compiles on first use
        return activeShader != nullptr;              // errors are logged; keep the last good shader

    for (int i = 0; i < 5; ++i)
        if (!shader->setStorageBlockBinding(BLOCK_NAMES[i], bindings[i]))
            MWARN(STR("MGpuSkinningBackend:: skinning shader has no buffer block ") + BLOCK_NAMES[i]);

    activeShader = shader;
    warnedMissingShader = false;
    return true;
}

bool MGpuSkinningBackend::init()
{
    if (activeShader)
        return true;

    if (!glDispatchCompute || !glMemoryBarrier || !glShaderStorageBlockBinding)
    {
        MWARN("MGpuSkinningBackend:: compute shaders are not available on this GL context");
        return false;
    }

    // Pick five SSBO binding points from the top of the range, skipping the
    // one the lighting system keeps bound.
    GLint maxBindings = 8;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxBindings);
    int found = 0;
    for (int b = maxBindings - 1; b >= 0 && found < 5; --b)
    {
        if (b == LIGHT_INDEX_DYNAMIC) continue;
        bindings[found++] = static_cast<unsigned int>(b);
    }
    if (found < 5)
    {
        MWARN("MGpuSkinningBackend:: not enough SSBO binding points");
        return false;
    }

    if (!refreshShader())
        return false;

    glGenBuffers(1, &boneBuffer);
    glGenQueries(2, timerQueries);

    MLOG("MGpuSkinningBackend:: compute skinning ready");
    return true;
}

void MGpuSkinningBackend::cleanup()
{
    if (!isGLContextCurrent())
    {
        boneBuffer = 0;
        activeShader = seenShader = nullptr;
        boneBufferSize = 0;
        timerQueries[0] = timerQueries[1] = 0;
        return;
    }
    // The shaders belong to the skinning shader asset.
    activeShader = seenShader = nullptr;
    if (boneBuffer)    { glDeleteBuffers(1, &boneBuffer); boneBuffer = 0; boneBufferSize = 0; }
    if (timerQueries[0]) { glDeleteQueries(2, timerQueries); timerQueries[0] = timerQueries[1] = 0; }
}

void MGpuSkinningBackend::readTimerQuery()
{
    const int slot = timerFrame & 1;
    if (!timerPending[slot])
        return;
    GLint available = 0;
    glGetQueryObjectiv(timerQueries[slot], GL_QUERY_RESULT_AVAILABLE, &available);
    if (!available)
        return;
    GLuint64 ns = 0;
    glGetQueryObjectui64v(timerQueries[slot], GL_QUERY_RESULT, &ns);
    timerPending[slot] = false;
    lastGpuTimeMs = static_cast<float>(static_cast<double>(ns) / 1.0e6);
    MSimpleProfiler::recordTimes("Animation.SkinningGPU", lastGpuTimeMs);
}

void MGpuSkinningBackend::skin(const std::vector<SMatrix4>& matrices, const std::vector<SSkinningJob>& jobs)
{
    if (jobs.empty() || matrices.empty() || !refreshShader())
        return;

    // The slot used two frames ago is free again once its result is read.
    readTimerQuery();
    const int slot = timerFrame & 1;
    const bool timing = timerQueries[slot] != 0 && !timerPending[slot];
    if (timing)
        glBeginQuery(GL_TIME_ELAPSED, timerQueries[slot]);

    // One upload for every instance's matrices.
    const size_t bytes = matrices.size() * sizeof(SMatrix4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, boneBuffer);
    if (bytes > boneBufferSize)
    {
        boneBufferSize = std::max(bytes, boneBufferSize * 2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(boneBufferSize), nullptr, GL_DYNAMIC_DRAW);
    }
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(bytes), matrices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (!activeShader->bind())
    {
        if (timing) glEndQuery(GL_TIME_ELAPSED);
        return;
    }
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[3], boneBuffer);

    for (const auto& job : jobs)
    {
        if (!job.mesh || !job.outputBuffer) continue;
        const auto vertexCount = static_cast<GLuint>(job.mesh->getVertexCount());
        if (vertexCount == 0) continue;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[0], job.mesh->getBindPoseBuffer());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[1], job.mesh->getOffsetsBuffer());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[2], job.mesh->getInfluenceBuffer());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings[4], job.outputBuffer);
        activeShader->setUInt("u_vertexCount", vertexCount);
        activeShader->setUInt("u_boneOffset", job.boneOffset);
        MComputeShader::dispatch((vertexCount + LOCAL_SIZE - 1) / LOCAL_SIZE);
    }

    // Once, after every dispatch, before any stage reads the output as vertices.
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    for (unsigned int b : bindings)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, b, 0);
    glUseProgram(0);

    if (timing)
    {
        glEndQuery(GL_TIME_ELAPSED);
        timerPending[slot] = true;
    }
    ++timerFrame;
}
