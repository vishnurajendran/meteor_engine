//
// Created by ssj5v on 21-03-2026.
//

#ifndef DEPTHBUFFER_H
#define DEPTHBUFFER_H

#include "core/graphics/core/render-pipeline/buffer.h"
#include "core/utils/glmhelper.h"

class SDepthBuffer : public SBuffer
{
public:
    ~SDepthBuffer() override;

    void bindAsActive() override;
    void unbind() override;

    // Satisfies SBuffer contract; actual allocation is deferred to resize().
    bool makeBuffer(const SString& bufferName) override;

    // (Re)allocates the FBO and depth texture at the given dimensions.
    // Safe to call multiple times — destroys previous GL objects first.
    bool resize(int width, int height) override;

    [[nodiscard]] GLuint    getFBOHandle()          const { return fboHandle; }
    [[nodiscard]] GLuint    getDepthTextureHandle() const { return depthTextureHandle; }
    [[nodiscard]] SVector2  getResolution()         const { return resolution; }

private:
    void destroyGL();

private:
    GLuint   fboHandle          = 0;
    GLuint   depthTextureHandle = 0;
    SVector2 resolution         = { 0.f, 0.f };
    SString  name;
};

#endif // DEPTHBUFFER_H