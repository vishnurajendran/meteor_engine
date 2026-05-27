//
// Created by ssj5v on 22-04-2025.
//

#ifndef GIZMOS_H
#define GIZMOS_H

#include <vector>

#include "core/object/object.h"
#include "core/utils/color.h"
#include "core/utils/glmhelper.h"


class MSpatialEntity;
class MCameraEntity;
class MTexture;

class MGizmos : public MObject {
    DEFINE_OBJECT_SUBCLASS(MGizmos)

public:
    static void drawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture);

    static void drawWireCube(SVector3 position, SVector3 halfExtents, SColor color, float thickness,
                             SQuaternion rotation = SQuaternion(1.0f, 0.0f, 0.0f, 0.0f));

    static void drawWireSphere(SVector3 position, float radius, SColor color, float thickness,
                               SQuaternion rotation = SQuaternion(1.0f, 0.0f, 0.0f, 0.0f));

    // Draws a circle in the plane whose normal is `normal`, centred on `center`.
    // The normal is normalised internally; passing a zero vector is undefined.
    static void drawWireCircle(const SVector3& center, float radius, const SVector3& normal,
                                const SColor& color, float thickness, int segments = 32);

    // Draws a cylinder centred on `center`. `axisDir` is the world-space direction
    // of the cylinder's long axis — it is normalised internally.
    static void drawWireCylinder(const SVector3& center, float halfHeight, float radius,
                                  const SVector3& axisDir, const SColor& color, float thickness);

    // Draws a capsule centred on `center`. `halfHeight` is half the cylindrical
    // segment (not including the hemisphere caps). `axisDir` is the world-space
    // long axis — normalised internally.
    static void drawWireCapsule(const SVector3& center, float halfHeight, float radius,
                                 const SVector3& axisDir, const SColor& color, float thickness);

    static void drawRay(const SVector3& origin, const SVector3& direction, const float& length,
                        const SColor& color, const float& thickness);

    // Queues a line segment. During a gizmo traversal (between requestGizmoDraws's
    // begin/end) this accumulates into the batch and produces no GL calls. Outside
    // a traversal it falls back to an immediate single-line draw so call sites
    // outside onDrawGizmo keep working unchanged.
    static void drawLine(const SVector3& start, const SVector3& end, const SColor& color,
                         const float& thickness, bool ignoreZDepth = false);

    static void drawWireFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                                 SColor color, float thickness);

    static void enableGizmos(bool enable);
    static void requestGizmoDraws();
    static void recursiveGizmoDraws(MSpatialEntity* entity);

private:
    // ---- Batched line rendering --------------------------------------------

    // Groups line segments by (color, thickness, ignoreZDepth). During the
    // gizmo traversal, drawLine pushes endpoints here instead of submitting GL
    // calls. flushBatches uploads and draws each group in two passes — opaque
    // first, then depth-ignoring — to minimise GL depth-test state changes.
    //
    // Note: batch matching uses exact float equality on `thickness`. All current
    // gizmo code passes literal 1.0f so this is safe; passing 1.0f in one call
    // and 1.000001f in another would produce two separate batches.
    struct SGizmoBatchEntry
    {
        SColor              color;
        float               thickness;
        bool                ignoreZDepth;
        std::vector<SVector3> vertices; // flat list of endpoint pairs — every two = one segment
    };

    static std::vector<SGizmoBatchEntry> pendingBatches;

    // True while requestGizmoDraws is running its entity traversal. drawLine
    // checks this to decide between batch-accumulate and immediate-draw.
    static bool batchingActive;

    // Finds the batch matching (color, thickness, ignoreZDepth), or creates
    // one. Returns a non-owning pointer valid until the next push_back on
    // pendingBatches — callers must not store it across that boundary.
    static SGizmoBatchEntry* findOrCreateBatch(const SColor& color, float thickness, bool ignoreZDepth);

    // Uploads each batch to the GPU and issues one glDrawArrays call per group.
    // Called at the end of requestGizmoDraws after the traversal completes.
    static void flushBatches();

    // ---- Immediate-mode draw (fallback / non-gizmo-pass use) ---------------
    static void drawLineImmediate(const SVector3& start, const SVector3& end,
                                   const SColor& color, const float& thickness, bool ignoreZDepth);

    // ---- Internal helpers --------------------------------------------------
    static MCameraEntity* getActiveCamera();
    static SVector2       getResolution();

    // Builds two vectors perpendicular to `n` and to each other, forming a
    // right-handed basis. `n` must already be normalised.
    static void buildTangentFrame(const SVector3& n, SVector3& outTangent, SVector3& outBitangent);

    static unsigned int uiQuadVAO;
    static unsigned int uiLineVAO;
    static unsigned int uiLineVBO;
    static bool         gizmosEnabled;

    static unsigned int createQuadVAO();
    static void         createLineVAO();
};

#endif // GIZMOS_H