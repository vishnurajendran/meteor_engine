// thumbnailrenderer.h
//
// Renders asset previews through a dedicated MRenderPipeline (opaque +
// lighting stages), composites the result, and stores it as an sf::Texture
// in MThumbnailCache.  One request is processed per tick() call so the
// editor frame time stays predictable.
//
// -- Mesh thumbnails --
// Rendered directly into a private 128x128 SFrameBuffer using the built-in
// 3-point Lambert shader.  No pipeline involvement.
//
// -- Material thumbnails --
// Rendered through a dedicated MRenderPipeline with opaque + lighting
// stages, then composited (BUFFER_OPAQUE x BUFFER_LIGHTS) into the
// thumbnail FBO for readback.

#pragma once
#ifndef THUMBNAIL_RENDERER_H
#define THUMBNAIL_RENDERER_H

#include <queue>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include "asset_thumbnail_request.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/render_pipeline.h"
#include "thumbnail_cache.h"

class MStaticMeshAsset;
class MMaterialAsset;
class SHeadlessRenderBuffer;

class MThumbnailRenderer
{
public:
    static constexpr int         THUMBNAIL_SIZE = 128;
    static constexpr const char* CACHE_DIR     = ".engine_data/thumbnail_cache/";

    MThumbnailRenderer()  = default;
    ~MThumbnailRenderer();

    void init();

    void requestThumbnail(MAsset* asset, MThumbnailCache& cache);
    bool tick(MThumbnailCache& cache);
    void clearQueue();

    bool isInitialised() const { return initialised; }

    static void deleteFromDisk(const SString& assetId);

private:
    void buildThumbnailShader();
    void buildSphereGeometry();
    void buildCompositeResources();
    void initThumbnailPipeline();
    void destroyGL();

    sf::Texture* renderMeshThumbnail    (MStaticMeshAsset* asset);
    sf::Texture* renderMaterialThumbnail(MMaterialAsset*   asset);

    // Composites BUFFER_OPAQUE x BUFFER_LIGHTS into the thumbnail FBO.
    void compositeThumbnail();

    static void         saveToDisk   (const SString& assetId, sf::Texture* tex);
    static sf::Texture* loadFromDisk (const SString& assetId);
    static void         ensureCacheDir();

    sf::Texture* readbackToTexture();

    static constexpr float FOV_DEG  = 35.0f;
    static constexpr float MARGIN   = 1.15f;

    struct SThumbnailCamera { glm::mat4 view; glm::mat4 proj; };
    static SThumbnailCamera computeThumbnailCamera(const glm::vec3& center, float radius);

    struct SMeshDraw { unsigned int vao; unsigned int ebo; int count; };
    void drawMeshes(const std::vector<SMeshDraw>& meshes,
                    const SThumbnailCamera& cam,
                    const glm::vec3& baseColor);

private:
    bool              initialised     = false;

    // -- Direct-render resources (mesh thumbnails) --
    SFrameBuffer      fbo;
    unsigned int      thumbnailShader = 0;

    // UV sphere for material/shader previews
    unsigned int sphereVAO = 0;
    unsigned int sphereVBO = 0;
    unsigned int sphereEBO = 0;
    int          sphereIndexCount = 0;

    // Uniform locations for the Lambert shader
    int uModel        = -1;
    int uView         = -1;
    int uProj         = -1;
    int uBaseColor    = -1;
    int uAlbedoTex    = -1;
    int uUseAlbedoTex = -1;

    // -- Composite resources (opaque x lights multiply) --
    unsigned int compositeShader = 0;
    unsigned int compositeQuadVAO = 0;
    unsigned int compositeQuadVBO = 0;
    unsigned int compositeQuadEBO = 0;
    int          ucAlbedoTex = -1;
    int          ucLightsTex = -1;

    // -- Pipeline resources (material thumbnails) --
    MRenderPipeline        thumbnailPipeline;
    SHeadlessRenderBuffer* headlessRenderBuffer = nullptr;

    std::queue<SAssetThumbnailRequest> requestQueue;
};

#endif // THUMBNAIL_RENDERER_H