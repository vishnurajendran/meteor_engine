//
// Created by ssj5v on 22-03-2026.
//

#include "render_pipeline_manager.h"
#include "core/engine/skybox/procedural_sky/procedural_skybox_queue.h"
#include "core/utils/logger.h"
#include "render_queue.h"
#include "stages/lighting/lighting_system_manager.h"
#include "stages/skybox/skybox_queue.h"

MRenderPipelineManager::MRenderPipelineManager()
    : renderQueue(std::make_unique<MRenderQueue>()),
      skyboxQueue(std::make_unique<MSkyboxQueue>()),
      proceduralSkyboxQueue(std::make_unique<MProceduralSkyboxQueue>()),
      lightSystem(std::make_unique<MLightSystemManager>())
{
    pipeline = new MRenderPipeline();
    MLOG("MRenderPipelineManager:: Created render pipeline manager");
}

MRenderPipelineManager::~MRenderPipelineManager()
{
    MLOG("MRenderPipelineManager:: cleaning render pipeline manager");
    delete pipeline;
    MLOG("MRenderPipelineManager:: clean complete");
}

void MRenderPipelineManager::setRenderTarget(SRenderBuffer* renderBuffer)
{
    pipeline->setRenderBuffer(renderBuffer);
}




