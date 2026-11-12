//
// particle_emitter_inspector_drawer.h
//
// Inspector for MParticleEmitter:
//   Particle Emitter — live count, emission, start values
//   Shape            — emission shape and its parameters
//   Renderer         — pick Sprite / Mesh / None, then its settings
//   Modules          — the attached simulator modules in run order, each
//                      with an enable box and its settings; Add Module /
//                      Remove, like the post-process stack.
//
// Module and renderer settings are drawn generically from their
// DECLARE_FIELD members (see IParticleSettings for the supported types).
//
#pragma once
#ifndef PARTICLE_EMITTER_INSPECTOR_DRAWER_H
#define PARTICLE_EMITTER_INSPECTOR_DRAWER_H

#include <unordered_map>

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class IParticleSettings;
class MAssetReferenceControl;
class MParticleEmitter;
struct FieldBase;

class MParticleEmitterInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MParticleEmitterInspectorDrawer)
public:
    SString getInspectorName() const override { return "Particle Emitter"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    void drawEmitter(MParticleEmitter* emitter);
    void drawShape(MParticleEmitter* emitter);
    void drawRenderer(MParticleEmitter* emitter);
    void drawModules(MParticleEmitter* emitter);
    void drawSettingsFields(IParticleSettings* settings);

    enum class EAssetKind { Texture, Mesh };
    // One asset picker per asset field, created on first draw. Keyed by the
    // field's address — a field at a recycled address just re-syncs its
    // picker from the field's value.
    MAssetReferenceControl* getAssetControl(const FieldBase* field, EAssetKind kind);
    std::unordered_map<const FieldBase*, MAssetReferenceControl*> assetControls;

    static const bool registered;
};

#endif // PARTICLE_EMITTER_INSPECTOR_DRAWER_H
