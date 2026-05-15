#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "composite_stage.h"

#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/utils/logger.h"

// ── Static ────────────────────────────────────────────────────────────────────

EBufferDebugView MCompositeStage::debugView = EBufferDebugView::Final;

static constexpr const char* FINAL_COMPOSITE_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/final_composite.mesl";

static const float k_quadVerts[] = {
    -1.f,-1.f,0.f, 0.f,0.f,  1.f,-1.f,0.f, 1.f,0.f,
     1.f, 1.f,0.f, 1.f,1.f, -1.f, 1.f,0.f, 0.f,1.f,
};
static const unsigned int k_quadIdx[] = { 0,1,2, 0,2,3 };

// ── Inline debug visualisation shader ─────────────────────────────────────────

static const char* k_debugVert = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aUV;
out vec2 vUV;
void main()
{
    vUV = aUV;
    gl_Position = vec4(aPos, 1.0);
}
)GLSL";

static const char* k_debugFrag = R"GLSL(
#version 460 core
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTex;
uniform int       uMode;       // 0 = color, 1 = depth, 2 = alpha-as-grey

void main()
{
    if (uMode == 1)
    {
        // Depth — apply power curve for contrast (raw depth is mostly 0.99+)
        float d = texture(uTex, vUV).r;
        d = pow(d, 128.0);   // exaggerate near-far difference
        FragColor = vec4(vec3(1.0 - d), 1.0);  // invert: near=white, far=black
    }
    else if (uMode == 2)
    {
        // Alpha channel as greyscale
        float a = texture(uTex, vUV).a;
        FragColor = vec4(vec3(a), 1.0);
    }
    else
    {
        // Passthrough color
        FragColor = vec4(texture(uTex, vUV).rgb, 1.0);
    }
}
)GLSL";

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void MCompositeStage::init(IRenderPipeline* const pipeline)
{
    initFullscreenQuad();
    buildDebugShader();

    auto handle = MAssetManager::getInstance()
                      ->getAsset<MShaderAsset>(FINAL_COMPOSITE_SHADER_PATH);
    if (!handle)
        MERROR("MCompositeStage::init — could not load: " + SString(FINAL_COMPOSITE_SHADER_PATH));
}

void MCompositeStage::cleanup(IRenderPipeline* const pipeline)
{
    destroyFullscreenQuad();
    if (debugShader) { glDeleteProgram(debugShader); debugShader = 0; }
}

void MCompositeStage::preRender (IRenderPipeline* const pipeline) {}
void MCompositeStage::postRender(IRenderPipeline* const pipeline) {}

void MCompositeStage::render(IRenderPipeline* const pipeline)
{
    while (glGetError() != GL_NO_ERROR) {}

    const uint32_t flags = pipeline->getCompositeFlags();
    if (flags == ECF_None) return;

    SRenderBuffer* renderBuffer = pipeline->getRenderBuffer();
    if (!renderBuffer) return;

    renderBuffer->bindAsActive();
    GLint sfmlFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sfmlFBO);

    const SVector2 res = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    // ── Debug view: skip normal composite and show the selected buffer ────
    if (debugView != EBufferDebugView::Final)
    {
        renderDebugView(pipeline, sfmlFBO, w, h);
        return;
    }

    // ── Normal composite path ─────────────────────────────────────────────
    const bool hasOpaque = flags & ECF_Opaque;
    const bool hasLights = flags & ECF_Lights;

    if (hasOpaque && hasLights)
        compositeOpaqueWithLights(pipeline, sfmlFBO, w, h);
    else if (hasOpaque)
        compositeOpaqueOnly(pipeline, sfmlFBO, w, h);

    if (flags & ECF_Depth)
        compositeDepth(pipeline, sfmlFBO, w, h);
}

// ── Debug buffer visualisation ────────────────────────────────────────────────

void MCompositeStage::renderDebugView(IRenderPipeline* const pipeline,
                                      int sfmlFBO, int w, int h) const
{
    if (!debugShader || quadVAO == 0) return;

    auto* opaque = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    auto* lights = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    auto* depth  = pipeline->getBufferRegistry()
                            .getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);

    GLuint texHandle = 0;
    int    mode      = 0;   // 0=color, 1=depth, 2=alpha

    switch (debugView)
    {
        case EBufferDebugView::Opaque:
            if (opaque) texHandle = opaque->getColorTextureHandle();
            mode = 0;
            break;

        case EBufferDebugView::Lighting:
            if (lights) texHandle = lights->getColorTextureHandle();
            mode = 0;
            break;

        case EBufferDebugView::Depth:
            // SFrameBuffer uses a depth renderbuffer (not sampleable).
            // SDepthBuffer from the depth prepass has a depth texture.
            if (depth) texHandle = depth->getDepthTextureHandle();
            mode = 1;
            break;

        case EBufferDebugView::LightMask:
            if (lights) texHandle = lights->getColorTextureHandle();
            mode = 2;
            break;

        default:
            return;
    }

    if (texHandle == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(debugShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    if (dbgLocTex  >= 0) glUniform1i(dbgLocTex, 0);
    if (dbgLocMode >= 0) glUniform1i(dbgLocMode, mode);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

// ── Normal composite helpers ──────────────────────────────────────────────────

void MCompositeStage::compositeOpaqueWithLights(IRenderPipeline* const pipeline,
                                                int sfmlFBO, int w, int h) const
{
    auto compositeHandle = MAssetManager::getInstance()
                               ->getAsset<MShaderAsset>(FINAL_COMPOSITE_SHADER_PATH);
    MShader* compositeShader = compositeHandle ? compositeHandle->getShader() : nullptr;

    if (!compositeShader || quadVAO == 0) return;

    auto* opaque = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    auto* lights = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (!opaque || opaque->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    compositeShader->bind();

    const glm::mat4 identity(1.0f);
    SShaderPropertyValue mv;
    mv.setMat4Val(identity);
    compositeShader->setPropertyValue("model",      mv);
    compositeShader->setPropertyValue("view",       mv);
    compositeShader->setPropertyValue("projection", mv);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, opaque->getColorTextureHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lights && lights->getFBOHandle() != 0
                                  ? lights->getColorTextureHandle()
                                  : opaque->getColorTextureHandle());

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    GLint albLoc = glGetUniformLocation(prog, "_albedoTex");
    GLint litLoc = glGetUniformLocation(prog, "_lightsTex");
    if (albLoc >= 0) glUniform1i(albLoc, 0);
    if (litLoc >= 0) glUniform1i(litLoc, 1);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void MCompositeStage::compositeOpaqueOnly(IRenderPipeline* const pipeline,
                                          int sfmlFBO, int w, int h) const
{
    auto* opaque = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaque || opaque->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, opaque->getFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);
    while (glGetError() != GL_NO_ERROR) {}
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
}

void MCompositeStage::compositeDepth(IRenderPipeline* const pipeline,
                                     int sfmlFBO, int w, int h) const
{
    auto* buf = pipeline->getBufferRegistry()
                         .getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    if (!buf || buf->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, buf->getFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);
    while (glGetError() != GL_NO_ERROR) {}
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
}

// ── GL resource creation ──────────────────────────────────────────────────────

void MCompositeStage::buildDebugShader()
{
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            MERROR(SString("CompositeStage debug shader: ") + log);
            glDeleteShader(s); return 0;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER,   k_debugVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, k_debugFrag);
    if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return; }

    debugShader = glCreateProgram();
    glAttachShader(debugShader, vs);
    glAttachShader(debugShader, fs);
    glLinkProgram(debugShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok; glGetProgramiv(debugShader, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(debugShader, 512, nullptr, log);
        MERROR(SString("CompositeStage debug shader link: ") + log);
        glDeleteProgram(debugShader); debugShader = 0;
        return;
    }

    dbgLocTex     = glGetUniformLocation(debugShader, "uTex");
    dbgLocMode    = glGetUniformLocation(debugShader, "uMode");
}

void MCompositeStage::initFullscreenQuad()
{
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadVerts), k_quadVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(k_quadIdx), k_quadIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glBindVertexArray(0);
}

void MCompositeStage::destroyFullscreenQuad()
{
    if (quadEBO) { glDeleteBuffers(1, &quadEBO); quadEBO = 0; }
    if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
}