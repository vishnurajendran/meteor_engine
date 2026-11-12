//
// Created by ssj5v on 29-04-2025.
//

#ifndef LIGHTSCENE_H
#define LIGHTSCENE_H
#include "core/object/object.h"
#include "core/utils/glmhelper.h"
#include "light_bounds.h"
#include "light_bvh_node.h"

class MDynamicLight;
struct SFrustum;
class MLightScene : public MObject {
    DEFINE_OBJECT_SUBCLASS(MLightScene)
public:
    ~MLightScene();

    // Replaces the set of lights the BVH is built from. Marks the BVH for
    // rebuild; nothing is built until the next query.
    void setLights(const std::vector<MDynamicLight*>& lights);

    // Call when any light moved or changed range. Cheap - the rebuild
    // happens once, at the start of the next query.
    void markDirty() { rebuildRequired = true; }
    [[nodiscard]] bool isRebuildRequired() const { return rebuildRequired; }

    // Rebuilds first if anything changed since the last build, then
    // returns up to maxLights lights touching bounds, nearest first.
    std::vector<MDynamicLight*> queryLights(const AABB& bounds, const int& maxLights);

    // Rebuilds first if needed, then returns every light whose sphere of
    // influence touches the frustum, nearest to viewerPosition first.
    // Whole branches outside the frustum are skipped. Enabled state is not
    // checked here - callers filter.
    std::vector<MDynamicLight*> queryLightsInFrustum(const SFrustum& frustum,
                                                     const SVector3& viewerPosition);
private:
    void rebuild();

    bool rebuildRequired = false;
    std::vector<MDynamicLight*> dynamicLights;
    std::vector<SLightBounds> lightBounds;
    SLightBVHNode* bvhRoot = nullptr;

    SLightBVHNode* buildBVH(std::vector<int>& indices, int start, int end);
    AABB computeBounds(const std::vector<int>& indices, int start, int end);
    void queryBVH(const AABB& bounds, SLightBVHNode* node, std::vector<std::pair<int, float>>& outLights);
    void queryFrustumBVH(const SFrustum& frustum, const SVector3& viewerPosition,
                         SLightBVHNode* node, std::vector<std::pair<int, float>>& outLights);
    auto intersectsAABB(const AABB& a, const AABB& b) -> bool;
    bool sphereIntersectsAABB(const glm::vec3& center, float radius, const AABB& box);
    void destroyBVH(SLightBVHNode* node);
};

#endif //LIGHTSCENE_H
