//
// Created by ssj5v on 22-03-2026.
//

#include "render_pipeline_manager.h"
#include "core/utils/logger.h"

MRenderPipelineManager* MRenderPipelineManager::activeInstance = nullptr;

MRenderPipelineManager::MRenderPipelineManager()
{
    if (activeInstance != nullptr)
    {
        MERROR("Renderer instance already available, there should always be only one instance of RenderPipelineManager");
        return;
    }

    activeInstance = this;
}

MRenderPipelineManager::~MRenderPipelineManager()
{
    if (activeInstance == this)
        activeInstance = nullptr;
}

void MRenderPipelineManager::setRenderTarget(SRenderBuffer* renderBuffer)
{
    pipeline.setRenderBuffer(renderBuffer);
}




