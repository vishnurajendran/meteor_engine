//
// ui_render_stage.cpp
//

#include <GL/glew.h>

#include "ui_render_stage.h"

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/ui/viewport/viewport_subsystem.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"

void MUIRenderStage::render(IRenderPipeline* const pipeline)
{
    auto* ui = MEngineSubsystemRegistry::getSubsystem<MViewportSubsystem>();
    MViewport* viewport = ui ? ui->getMainViewport() : nullptr;
    if (!viewport)
        return;

    SRenderBuffer* renderBuffer = pipeline->getRenderBuffer();
    if (!renderBuffer)
        return;

    // The pipeline's final target: the window, or the Scene view's texture.
    renderBuffer->bindAsActive();
    GLint framebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);

    const SVector2 resolution = pipeline->getRenderResolution();
    ui->renderViewport(*viewport, static_cast<unsigned int>(framebuffer),
                       static_cast<int>(resolution.x), static_cast<int>(resolution.y));
}
