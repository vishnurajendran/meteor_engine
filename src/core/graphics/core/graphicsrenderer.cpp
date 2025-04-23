//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include "graphicsrenderer.h"
#include <GL/glew.h>
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/lighting/lighting_system_manager.h"
#include "opengldrawcall.h"
#include "render_queue.h"
#include "sfmldrawcall.h"

sf::RenderTarget* MGraphicsRenderer::renderTarget;
std::vector<MSFMLDrawCall*> MGraphicsRenderer::sfmlDrawCalls;
std::vector<MOpenGlDrawCall*> MGraphicsRenderer::openGlDrawCalls;

bool MGraphicsRenderer::drawGizmos = true;

void MGraphicsRenderer::submit(MDrawCall* drawCall) {
    if(!renderTarget) {
        MERROR(STR("Graphics Render Target NULL, Submission omitted"));
        return;
    }

    if(drawCall) {
        if(const auto glDrawCall = dynamic_cast<MOpenGlDrawCall*>(drawCall)) {
            auto resolution = renderTarget->getSize();
            glDrawCall->setTargetResolution({resolution.x, resolution.y});
            openGlDrawCalls.push_back(glDrawCall);
        }
        else
        {
            sfmlDrawCalls.push_back(dynamic_cast<MSFMLDrawCall*>(drawCall));
        }
    }
    else
        MERROR(STR("drawCall NULL, ignored"));
}

void MGraphicsRenderer::prepare()
{
    if (!renderTarget)
    {
        MERROR(STR("Graphics Render Target NULL, Draw cancelled"));
        return;
    }

    // sort these calls in order
    std::ranges::sort(openGlDrawCalls, [](MOpenGlDrawCall* lhs, MOpenGlDrawCall* rhs)
                      { return lhs->getSortinOrder() > rhs->getSortinOrder(); });

    std::ranges::sort(sfmlDrawCalls, [](MSFMLDrawCall* lhs, MSFMLDrawCall* rhs)
                      { return lhs->getSortinOrder() > rhs->getSortinOrder(); });

    auto size = renderTarget->getSize();
    renderTarget->setActive(true);
    glViewport(0, 0, size.x, size.y);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // prepare lights for rendering.
    MLightSystemManager::getInstance()->prepareLights();
    MRenderQueue::prepareToDraw();
    MRenderQueue::requestDrawCalls();
}

SVector2 MGraphicsRenderer::getResolution()
{
    if (renderTarget)
    {
        return {renderTarget->getSize().x, renderTarget->getSize().y};
    }

    return {0,0};
}


void MGraphicsRenderer::initialise(sf::RenderTarget *target) {
    renderTarget = target;
}

const char* getGLErrorString(GLenum error) {
    switch (error) {
    case GL_NO_ERROR:
        return "No error";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM: An unacceptable value was specified for an enumerated argument.";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE: A numeric argument is out of range.";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW: An attempt was made to pop an empty stack.";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW: An attempt was made to push more values than the stack can hold.";
    default:
        return "Unknown OpenGL error.";
    }
}

void MGraphicsRenderer::draw() {

    if(!renderTarget) {
        MERROR(STR("Graphics Render Target NULL, Draw cancelled"));
        return;
    }

    for(const auto& drawCall : openGlDrawCalls){
        if(drawCall){
            drawCall->draw();
        }
        else
            MERROR(STR("Draw Call NULL, ignored"));
    }

    if (drawGizmos)
    {
        MGizmos::requestGizmoDraws();
    }

    auto error = glGetError();
    if (error != GL_NO_ERROR)
    {
        glGetError();
        MERROR(STR("OpenGL Error: ") + getGLErrorString(error));
    }

    openGlDrawCalls.clear();

    /*renderTarget->pushGLStates();
    for(const auto drawCall : sfmlDrawCalls){
        if(drawCall){
            drawCall->setTarget(renderTarget);
            drawCall->draw();
        }
        else
            MERROR(STR("Draw Call NULL, ignored"));
    }
    renderTarget->popGLStates();
    sfmlDrawCalls.clear();*/
}