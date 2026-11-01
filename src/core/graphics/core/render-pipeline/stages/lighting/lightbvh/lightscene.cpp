//
// Created by ssj5v on 29-04-2025.
//
#include "lightscene.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "glm/gtx/norm.hpp"
#include "core/utils/frustum.h"
#include <algorithm>

MLightScene::~MLightScene()
{
    destroyBVH(bvhRoot);
}

void MLightScene::setLights(const std::vector<MDynamicLight*>& lights)
{
    // Keep only valid lights so BVH leaf indices map 1:1 onto dynamicLights.
    dynamicLights.clear();
    dynamicLights.reserve(lights.size());
    for (auto* light : lights)
        if (light) dynamicLights.push_back(light);

    rebuildRequired = true;
}

void MLightScene::rebuild()
{
    rebuildRequired = false;
    lightBounds.clear();

    // Positions and ranges are re-read here, so moved lights and changed
    // ranges are picked up by any rebuild.
    for (auto* light : dynamicLights) {
        SLightBounds bounds;
        bounds.position = light->getWorldPosition();
        bounds.radius = light->getRange();
        lightBounds.push_back(bounds);
    }

    // List of indices to lights
    std::vector<int> indices(lightBounds.size());
    for (int i = 0; i < indices.size(); i++) {
        indices[i] = i;
    }

    // Build the BVH
    if (bvhRoot) {
        destroyBVH(bvhRoot);
    }
    bvhRoot = buildBVH(indices, 0, static_cast<int>(indices.size()));
}

std::vector<MDynamicLight*> MLightScene::queryLights(const AABB& bounds, const int& maxLights)
{
    // Lights were added, removed, moved or resized since the last build.
    if (rebuildRequired)
        rebuild();

    std::vector<std::pair<int, float>> foundLights; // {lightIndex, distanceSq}
    queryBVH(bounds, bvhRoot, foundLights);

    // Sort by nearest
    std::sort(foundLights.begin(), foundLights.end(),
              [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                  return a.second < b.second;
              });

    std::vector<MDynamicLight*> result;
    for (int i = 0; i < std::min(maxLights, (int)foundLights.size()); ++i) {
        result.push_back(dynamicLights[foundLights[i].first]);
    }

    return result;
}


std::vector<MDynamicLight*> MLightScene::queryLightsInFrustum(const SFrustum& frustum,
                                                           const SVector3& viewerPosition)
{
    if (rebuildRequired)
        rebuild();

    std::vector<std::pair<int, float>> found; // {lightIndex, distance to sphere}
    queryFrustumBVH(frustum, viewerPosition, bvhRoot, found);

    std::sort(found.begin(), found.end(),
              [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                  return a.second < b.second;
              });

    std::vector<MDynamicLight*> result;
    result.reserve(found.size());
    for (const auto& [index, distance] : found)
        result.push_back(dynamicLights[index]);
    return result;
}

void MLightScene::queryFrustumBVH(const SFrustum& frustum, const SVector3& viewerPosition,
                                  SLightBVHNode* node, std::vector<std::pair<int, float>>& outLights)
{
    if (!node) return;

    // Node bounds enclose every light sphere below it - if the box is
    // outside the frustum, so is the whole branch.
    if (!frustum.testAABB(node->bounds))
        return;

    if (node->lightIndex != -1)
    {
        // Leaf bounds are exactly this light's sphere box, already tested.
        const SLightBounds& light = lightBounds[node->lightIndex];
        const float distance = std::max(0.0f, glm::length(light.position - viewerPosition) - light.radius);
        outLights.emplace_back(node->lightIndex, distance);
        return;
    }

    queryFrustumBVH(frustum, viewerPosition, node->left,  outLights);
    queryFrustumBVH(frustum, viewerPosition, node->right, outLights);
}

SLightBVHNode* MLightScene::buildBVH(std::vector<int>& indices, int start, int end)
{
    if (start >= end)
        return nullptr;

    SLightBVHNode* node = new SLightBVHNode();

    // Compute bounds
    node->bounds = computeBounds(indices, start, end);

    int count = end - start;
    if (count == 1) {
        // Leaf node
        node->lightIndex = indices[start];
        return node;
    }

    // Find axis with largest extent
    SVector3 extent = node->bounds.max - node->bounds.min;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    // Sort by chosen axis
    std::sort(indices.begin() + start, indices.begin() + end,
              [&](int a, int b) {
                  return lightBounds[a].position[axis] < lightBounds[b].position[axis];
              });

    int mid = (start + end) / 2;
    node->left = buildBVH(indices, start, mid);
    node->right = buildBVH(indices, mid, end);

    return node;
}

AABB MLightScene::computeBounds(const std::vector<int>& indices, int start, int end)
{
    AABB bounds;
    bounds.min = SVector3(FLT_MAX, FLT_MAX, FLT_MAX);
    bounds.max = SVector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = start; i < end; ++i) {
        const SLightBounds& light = lightBounds[indices[i]];
        SVector3 minPoint = light.position - SVector3(light.radius);
        SVector3 maxPoint = light.position + SVector3(light.radius);

        bounds.min = glm::min(bounds.min, minPoint);
        bounds.max = glm::max(bounds.max, maxPoint);
    }

    return bounds;
}

void MLightScene::queryBVH(const AABB& bounds, SLightBVHNode* node, std::vector<std::pair<int, float>>& outLights)
{
    if (!node) return;

    // Skip if the bounding volumes don't overlap
    if (!intersectsAABB(node->bounds, bounds))
        return;

    if (node->lightIndex != -1) {
        // Leaf node
        const SLightBounds& light = lightBounds[node->lightIndex];

        // Perform AABB vs sphere intersection
        if (sphereIntersectsAABB(light.position, light.radius, bounds)) {
            float distSq = glm::distance2(light.position, bounds.getCentre()); // optional, for sorting
            outLights.emplace_back(node->lightIndex, distSq);
        }
        return;
    }

    // Recurse into children
    queryBVH(bounds, node->left, outLights);
    queryBVH(bounds, node->right, outLights);
}

bool MLightScene::sphereIntersectsAABB(const glm::vec3& center, float radius, const AABB& box)
{
    float distSq = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float v = center[i];
        if (v < box.min[i]) distSq += (box.min[i] - v) * (box.min[i] - v);
        else if (v > box.max[i]) distSq += (v - box.max[i]) * (v - box.max[i]);
    }
    return distSq <= radius * radius;
}

bool MLightScene::intersectsAABB(const AABB& a, const AABB& b)
{
    return AABB::intersect(a,b);
}

void MLightScene::destroyBVH(SLightBVHNode* node)
{
    if (!node) return;
    destroyBVH(node->left);
    destroyBVH(node->right);
    delete node;
}
