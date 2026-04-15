//
// Created by ssj5v on 11-04-2026.
//

#include "renderbuffer.h"
#include "core/utils/logger.h"

SRenderBuffer::~SRenderBuffer()
{
    unbind();
}

void SRenderBuffer::bindAsActive()
{
    if (renderTarget == nullptr)
        return;

    if (!renderTarget->setActive(true))
    {
        MWARN("SRenderBuffer:: Unable to activate render target");
    }
}

void SRenderBuffer::unbind()
{
    if (renderTarget == nullptr)
        return;

    if (!renderTarget->setActive(false))
    {
        MWARN("SRenderBuffer:: Unable to deactivate render target");
    }
}
SVector2 SRenderBuffer::getResolution() const
{
    if (renderTarget == nullptr) return SVector2(0);
    auto size = renderTarget->getSize();
    return { size.x, size.y };
}


bool SRenderBuffer::makeFromRenderTarget(sf::RenderTarget* inTarget, SRenderBuffer*& outBuffer)
{
    if (inTarget == nullptr)
    {
        MERROR("SRenderBuffer:: Invalid render target provided");
        return false;
    }

    outBuffer = new SRenderBuffer();
    outBuffer->renderTarget = inTarget;
    return true;
}
