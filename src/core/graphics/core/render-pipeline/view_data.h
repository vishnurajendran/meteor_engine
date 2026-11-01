//
// View data - the camera a pipeline renders this frame.
//

#pragma once
#ifndef VIEW_DATA_H
#define VIEW_DATA_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class IRenderPipeline;

// Camera matrices for the current frame, resolved the same way by every
// stage: the pipeline's camera override if set, otherwise the first active
// camera. Stages that reconstruct positions from BUFFER_DEPTH must use these
// so they invert exactly the projection the prepass rendered with.
struct SViewData
{
    bool      valid             = false;   // false when there is no camera
    glm::mat4 view              = glm::mat4(1.f);
    glm::mat4 projection        = glm::mat4(1.f);
    glm::mat4 inverseView       = glm::mat4(1.f);
    glm::mat4 inverseProjection = glm::mat4(1.f);
    glm::vec3 cameraPosition    = glm::vec3(0.f);
};

SViewData resolveViewData(IRenderPipeline* pipeline);

#endif // VIEW_DATA_H
