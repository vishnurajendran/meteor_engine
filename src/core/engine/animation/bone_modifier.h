//
// bone_modifier.h
//
// IBoneModifier - edits an evaluated pose before onPostAnimate and skinning.
// Modifiers are not special-cased by the subsystem: it gathers them from the
// skeletal mesh entity's direct children each frame and runs them in
// parent-first bone order (by sort key), so a modifier sees its ancestors'
// edits. MBoneControlEntity is the first implementation; two-bone IK for legs
// and hands becomes another.
//
#pragma once
#ifndef BONE_MODIFIER_H
#define BONE_MODIFIER_H

class SPoseEditContext;
struct SSkeleton;

class IBoneModifier
{
public:
    virtual ~IBoneModifier() = default;

    // Bone index the modifier works on - its place in parent-first order.
    // Negative skips it this frame (no bone, disabled, ...).
    [[nodiscard]] virtual int getModifierSortKey(const SSkeleton& skeleton) const = 0;

    virtual void applyModifier(SPoseEditContext& pose, float deltaTime) = 0;
};

#endif // BONE_MODIFIER_H
