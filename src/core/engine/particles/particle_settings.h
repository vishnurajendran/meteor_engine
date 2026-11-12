//
// particle_settings.h
//
// Base for anything that hangs off a particle emitter and is saved with it:
// simulator modules and renderers. Settings are DECLARE_FIELD members, so
// they save / load with the emitter and the inspector can draw them
// generically.
//
// Field types the particle inspector draws:
//   bool, int, float, SVector3, SCurve, SGradient,
//   TAssetRef<MTextureAsset>, TAssetRef<MStaticMeshAsset>
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_SETTINGS_H
#define METEOR_ENGINE_PARTICLE_SETTINGS_H

#include <string>
#include <vector>

#include "core/utils/sstring.h"
#include "data/serialized_class_base.h"

// Inspector hints for one field (same idea as SPostProcessEffectSettings::SFieldMeta).
struct SParticleFieldMeta
{
    float                    min      = 0.0f;    // min == max: unbounded drag
    float                    max      = 0.0f;
    float                    speed    = 0.01f;   // drag speed when unbounded
    const char*              tooltip  = nullptr;
    std::vector<const char*> options;            // int fields: shown as a combo
    bool                     color    = false;   // SVector3 fields: colour picker
    float                    curveMin = 0.0f;    // SCurve fields: initial graph range
    float                    curveMax = 1.0f;
};

class IParticleSettings : public SerializedClassBase
{
public:
    IParticleSettings() = default;
    ~IParticleSettings() override = default;

    // `fields` holds pointers to this object's own members — a copy would
    // point at the original's fields. Never copy.
    IParticleSettings(const IParticleSettings&)            = delete;
    IParticleSettings& operator=(const IParticleSettings&) = delete;

    [[nodiscard]] virtual SParticleFieldMeta getFieldMeta(const std::string& fieldName) const { return {}; }

    // Registered type name ("Gravity", "Sprite"), set by the emitter on
    // creation. Saved in scene files.
    [[nodiscard]] const SString& getTypeName() const { return typeName; }
    void setTypeName(const SString& name) { typeName = name; }

private:
    SString typeName;
};

#endif // METEOR_ENGINE_PARTICLE_SETTINGS_H
