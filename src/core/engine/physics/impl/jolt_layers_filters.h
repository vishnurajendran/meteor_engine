//
// jolt_layers_filters.h
//
// Jolt layer setup for the Meteor physics layer system.
//
// ObjectLayers are split into two bands of 32:
//   0-31   : static bodies on physics layers 0-31
//   32-63  : dynamic/kinematic bodies on physics layers 0-31
//
// The physics layer is extracted via (objectLayer & 31).
// The motion class is determined by (objectLayer < 32) = static.
//
// BroadPhase layers remain at 2 (static / dynamic) so Jolt's broadphase
// structure stays compact. The physics-layer collision check happens in the
// ObjectLayerPairFilter, which is called during broadphase pair creation
// and is extremely cheap (one bitmask lookup per pair).

#ifndef JOLT_LAYERS_FILTERS_H
#define JOLT_LAYERS_FILTERS_H

#include <cstdint>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace JPH;

// ---------------------------------------------------------------------------
// Object layers
// ---------------------------------------------------------------------------

namespace Layers
{
    // Base offsets. A body's ObjectLayer = STATIC_BASE or DYNAMIC_BASE + physicsLayer.
    static constexpr ObjectLayer STATIC_BASE  = 0;
    static constexpr ObjectLayer DYNAMIC_BASE = 32;
    static constexpr ObjectLayer NUM_LAYERS   = 64;

    // Convenience: default ObjectLayer for bodies before physicsLayer is set.
    static constexpr ObjectLayer DEFAULT_STATIC  = STATIC_BASE;   // 0
    static constexpr ObjectLayer DEFAULT_DYNAMIC  = DYNAMIC_BASE;  // 32
}

// ---------------------------------------------------------------------------
// BroadPhase layers (unchanged — still 2)
// ---------------------------------------------------------------------------

namespace BroadPhaseLayers
{
    static constexpr BroadPhaseLayer STATIC_BODY(0);
    static constexpr BroadPhaseLayer DYNAMIC_BODY(1);
    static constexpr unsigned int NUM_LAYERS = 2;
}

// ---------------------------------------------------------------------------
// ObjectLayerPairFilter — checks the collision matrix
// ---------------------------------------------------------------------------

class MObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    // Call from MJoltPhysicsEngine::init after loading physics layers.
    // `masks` points to the 32-element uint32_t array owned by
    // MPhysicsLayersSettings. The pointer remains valid for the engine lifetime.
    void setCollisionMasks(const uint32_t* masks) { collisionMasks = masks; }

    [[nodiscard]] bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
    {
        const bool static1 = inObject1 < 32;
        const bool static2 = inObject2 < 32;

        // Static vs static never collide (same rule as before).
        if (static1 && static2)
            return false;

        // Extract physics layer from each ObjectLayer.
        const uint32_t layer1 = inObject1 & 31u;
        const uint32_t layer2 = inObject2 & 31u;

        // If no collision matrix has been loaded yet, fall back to
        // "everything collides" for backward compatibility.
        if (!collisionMasks)
            return true;

        // AND both directions — an asymmetric matrix takes the stricter path.
        return ((collisionMasks[layer1] >> layer2) & 1u) &&
               ((collisionMasks[layer2] >> layer1) & 1u);
    }

private:
    const uint32_t* collisionMasks = nullptr;
};

// ---------------------------------------------------------------------------
// BroadPhaseLayerInterface — maps 64 ObjectLayers to 2 BroadPhase layers
// ---------------------------------------------------------------------------

class MBPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
    MBPLayerInterfaceImpl()
    {
        // 0-31 = static, 32-63 = dynamic.
        for (uint32_t i = 0; i < 32; ++i)
            mObjectToBroadPhase[i] = BroadPhaseLayers::STATIC_BODY;
        for (uint32_t i = 32; i < 64; ++i)
            mObjectToBroadPhase[i] = BroadPhaseLayers::DYNAMIC_BODY;
    }

    [[nodiscard]] unsigned int GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    [[nodiscard]] BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch (static_cast<BroadPhaseLayer::Type>(inLayer))
        {
        case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::STATIC_BODY):  return "STATIC_BODY";
        case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::DYNAMIC_BODY): return "DYNAMIC_BODY";
        default: JPH_ASSERT(false); return "INVALID";
        }
    }
#endif

private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// ---------------------------------------------------------------------------
// ObjectVsBroadPhaseLayerFilter
// ---------------------------------------------------------------------------

class MObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    [[nodiscard]] bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
    {
        // Static ObjectLayers (0-31) only collide with the dynamic broadphase layer.
        // Dynamic/Kinematic ObjectLayers (32-63) collide with both broadphase layers.
        if (inLayer1 < 32)
            return inLayer2 == BroadPhaseLayers::DYNAMIC_BODY;
        return true;
    }
};

#endif // JOLT_LAYERS_FILTERS_H