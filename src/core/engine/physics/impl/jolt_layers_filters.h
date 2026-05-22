//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_LAYERS_FILTERS_H
#define JOLT_LAYERS_FILTERS_H
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

using namespace JPH;

namespace Layers
{
    static constexpr ObjectLayer STATIC_BODY = 0;
    static constexpr ObjectLayer DYNAMIC_BODY = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class MObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::STATIC_BODY:
            return inObject2 == Layers::DYNAMIC_BODY; // Non-moving only collides with moving
        case Layers::DYNAMIC_BODY:
            return true;                              // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr BroadPhaseLayer STATIC_BODY(0);
    static constexpr BroadPhaseLayer DYNAMIC_BODY(1);
    static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class MBPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
    MBPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::STATIC_BODY] = BroadPhaseLayers::STATIC_BODY;
        mObjectToBroadPhase[Layers::DYNAMIC_BODY] = BroadPhaseLayers::DYNAMIC_BODY;
    }

    [[nodiscard]] uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    [[nodiscard]] BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char * GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
    {
        switch (static_cast<BroadPhaseLayer::Type>(inLayer))
        {
        case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::STATIC_BODY):	    return "STATIC_BODY";
        case static_cast<BroadPhaseLayer::Type>(BroadPhaseLayers::DYNAMIC_BODY):		return "DYNAMIC_BODY";
        default:
            JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class MObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::STATIC_BODY:
            return inLayer2 == BroadPhaseLayers::DYNAMIC_BODY;
        case Layers::DYNAMIC_BODY:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

#endif //JOLT_LAYERS_FILTERS_H
