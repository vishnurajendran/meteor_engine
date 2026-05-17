//
// Created by ssj5v on 22-03-2026.
//

#include "render_pipeline_manager.h"
#include "core/utils/logger.h"

MRenderPipelineManager::MRenderPipelineManager()
{
    pipeline = new MRenderPipeline();
    MLOG("MRenderPipelineManager:: Created render pipeline manager");
}

MRenderPipelineManager::~MRenderPipelineManager()
{
    delete pipeline;
    MLOG("MRenderPipelineManager:: cleaning render pipeline manager");
}

void MRenderPipelineManager::setRenderTarget(SRenderBuffer* renderBuffer)
{
    pipeline->setRenderBuffer(renderBuffer);
}




