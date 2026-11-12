//
// View data - the camera a pipeline renders this frame.
//

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "view_data.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"

SViewData resolveViewData(IRenderPipeline* pipeline)
{
    SViewData data;
    if (!pipeline) return data;

    const auto& camOverride = pipeline->getCameraOverride();
    if (camOverride.active)
    {
        data.view       = camOverride.view;
        data.projection = camOverride.proj;
        data.valid      = true;
    }
    else if (const MCameraEntity* camera = MEngineSubsystemRegistry::getSubsystem<MViewManagement>()->getFirstActiveCamera())
    {
        data.view       = camera->getViewMatrix();
        data.projection = camera->getProjectionMatrix(pipeline->getRenderResolution());
        data.valid      = true;
    }

    data.inverseView       = glm::inverse(data.view);
    data.inverseProjection = glm::inverse(data.projection);
    data.cameraPosition    = glm::vec3(data.inverseView[3]);
    return data;
}
