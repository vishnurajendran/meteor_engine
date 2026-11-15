//
// bone_attachment_inspectors.h
//
// Inspectors for MBoneSocketEntity and MBoneControlEntity: a bone dropdown
// from the parent skeleton, warnings when the parent isn't a skeletal mesh
// entity or the bone is missing (shown in red), and their settings.
//
#pragma once
#ifndef BONE_ATTACHMENT_INSPECTORS_H
#define BONE_ATTACHMENT_INSPECTORS_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MBoneSocketInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MBoneSocketInspectorDrawer)
public:
    SString getInspectorName() const override { return "Bone Socket"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MBoneSocketInspectorDrawer() = default;
    static bool registered;
};

class MBoneControlInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MBoneControlInspectorDrawer)
public:
    SString getInspectorName() const override { return "Bone Control"; }
    bool    canDraw(MSpatialEntity* entity) override;

protected:
    void onDrawInspector(MSpatialEntity* target) override;

private:
    MBoneControlInspectorDrawer() = default;
    static bool registered;
};

#endif // BONE_ATTACHMENT_INSPECTORS_H
