//
// particle_data.h
//
// Plain data shared by the emitter, its simulator modules and its renderer.
// Nothing here is exposed to scripts except the two enums.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_DATA_H
#define METEOR_ENGINE_PARTICLE_DATA_H

#include <cstdint>

#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/glmhelper.h"

// Where new particles start, in the emitter's local space. Particles are
// emitted along the emitter's local +Y axis; the 2D shapes lie flat in the
// local XZ plane, facing +Y.
SCRIPT_BIND_ENUM()
enum class EParticleEmitterShape
{
    // Point
    Point,    // at the origin, in every direction
    // 2D
    Square,   // anywhere on a shapeExtents.x * shapeExtents.z rectangle, along +Y
    Circle,   // anywhere on a disc of shapeRadius, along +Y
    // 3D
    Box,      // anywhere inside shapeExtents (half sizes), along +Y
    Sphere,   // anywhere inside shapeRadius, outwards from the centre
    Cone,     // from a disc of shapeRadius, spreading out by up to shapeAngle
};

// Which space particle positions and velocities are stored in.
//   Local  — the emitter's space. Particles move with the emitter.
//   Global — world space. Particles stay where they were emitted.
SCRIPT_BIND_ENUM()
enum class EParticleSimulationSpace
{
    Local,
    Global,
};

// One particle. Stored by value in the emitter's pool — the whole pool is
// one contiguous array, so modules stream through it without chasing
// pointers. About 120 bytes.
//
// Colours are authored (sRGB) values, like every colour in the editor; the
// particle shader converts them to linear.
struct SParticleData
{
    SVector3    position    = SVector3(0.0f);           // simulation space
    SVector3    velocity    = SVector3(0.0f);           // written by force modules — read by the velocity module
    SQuaternion rotation    = glm::identity<SQuaternion>();
    SVector3    startSize   = SVector3(1.0f);           // set once at spawn — never modified after
    SVector3    size        = SVector3(1.0f);           // derived each frame — e.g. startSize * curve(t)
    SVector4    startColor  = SVector4(1.0f);           // set once at spawn — rgba
    SVector4    color       = SVector4(1.0f);           // derived each frame
    float       age         = 0.0f;                     // seconds since spawn
    float       invLifetime = 1.0f;                     // 1 / lifetime — multiply instead of divide per particle
    uint32_t    seed        = 0;                        // per-particle random — stable across frames

    // 0 at birth, 1 at death.
    [[nodiscard]] float normalizedAge() const { return age * invLifetime; }
};

// What a module needs to know about the emitter for one simulate() call.
// Built once per emitter per frame, read-only for modules (safe to read
// from worker threads).
struct SParticleSimContext
{
    EParticleSimulationSpace space          = EParticleSimulationSpace::Local;

    // Emitter's local -> world matrix this frame.
    SMatrix4                 emitterToWorld = SMatrix4(1.0f);

    // Emitter's world rotation this frame (no scale) — for turning
    // directions between emitter space and world space.
    SQuaternion              emitterRotation = glm::identity<SQuaternion>();

    // Rotates a world-space direction (gravity, wind) into simulation
    // space. Identity for Global; inverse of the emitter's world rotation
    // for Local. Scale is ignored.
    SQuaternion              worldToSimulation = glm::identity<SQuaternion>();

    // Seconds the emitter has been simulating. For modules that scroll
    // through something over time (noise).
    float                    time           = 0.0f;
};

#endif // METEOR_ENGINE_PARTICLE_DATA_H
