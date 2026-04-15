//
// Created by ssj5v on 11-04-2026.
//

#ifndef RENDERBUFFER_H
#define RENDERBUFFER_H
#include "SFML/Graphics/RenderTarget.hpp"
#include "core/graphics/core/render-pipeline/buffer.h"
#include "core/utils/glmhelper.h"


class SRenderBuffer: public SBuffer {
public:
    ~SRenderBuffer();

    virtual void bindAsActive() override;
    virtual void unbind() override;

    SVector2 getResolution() const;
    sf::RenderTarget* getCoreRenderTarget() const { return renderTarget; }

    // no need in this buffer
    bool makeBuffer(const SString& bufferName) override { return false; }

    // creation helpers
    static bool makeFromRenderTarget(sf::RenderTarget* inTarget, SRenderBuffer*& outBuffer);

private:
    SRenderBuffer() = default;

private:
    sf::RenderTarget* renderTarget = nullptr;
};



#endif //RENDERBUFFER_H
