//
// skeletal_mesh_inspector.h
//
// Inspector for MSkeletalMeshEntity. Reads top to bottom as cause and
// effect: pick the mesh, add sources, see the clips that result.
//
//   Skeletal Mesh      [ Hero.fbx ]
//   Default Clip       [ Idle    v]    available clips only
//   v Additional Animation Sources     reorderable, later wins
//   v Available Clips                  read-only - unavailable and overridden
//                                      clips stay, greyed, with the reason
//   v Preview                          edit mode: play a clip without Play
//   v Material Slots
//
#pragma once
#ifndef SKELETAL_MESH_INSPECTOR_H
#define SKELETAL_MESH_INSPECTOR_H

#include <vector>

#include "editor/editorwindows/inspectordrawer/mesh_common/material_slots_drawer.h"
#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MAssetReferenceControl;
class MSkeletalMeshEntity;

class MSkeletalMeshInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MSkeletalMeshInspectorDrawer)
public:
    SString getInspectorName() const override { return "Skeletal Mesh"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MSkeletalMeshInspectorDrawer();

    void drawMeshSection(MSkeletalMeshEntity* entity);
    void drawSourcesSection(MSkeletalMeshEntity* entity);
    void drawClipsSection(MSkeletalMeshEntity* entity);
    void drawPreviewSection(MSkeletalMeshEntity* entity);

    MAssetReferenceControl*              meshAssetControl = nullptr;
    std::vector<MAssetReferenceControl*> sourceControls;
    MMaterialSlotsDrawer                 materialSlots;

    static bool registered;
};

#endif // SKELETAL_MESH_INSPECTOR_H
