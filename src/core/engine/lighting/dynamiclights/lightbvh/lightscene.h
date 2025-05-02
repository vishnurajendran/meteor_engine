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
class MLightScene : MObject {
public:
    ~MLightScene();
    void build(std::vector<MDynamicLight*> lights);
    std::vector<MDynamicLight*> queryLights(const AABB& bounds, const int& maxLights);
private:
    std::vector<MDynamicLight*> dynamicLights;
    std::vector<SLightBounds> lightBounds;
    SLightBVHNode* bvhRoot = nullptr;

    SLightBVHNode* buildBVH(std::vector<int>& indices, int start, int end);
    AABB computeBounds(const std::vector<int>& indices, int start, int end);
    void queryBVH(const AABB& bounds, SLightBVHNode* node, std::vector<std::pair<int, float>>& outLights);
    auto intersectsAABB(const AABB& a, const AABB& b) -> bool;
    bool sphereIntersectsAABB(const glm::vec3& center, float radius, const AABB& box);
    void destroyBVH(SLightBVHNode* node);
};

#endif //LIGHTSCENE_H
