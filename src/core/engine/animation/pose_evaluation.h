//
// pose_evaluation.h
//
// The per-instance pose maths run by MAnimationSubsystem's jobs. Pure
// functions of one SAnimInstance (plus read-only clips and skeleton), so they
// are safe on any worker thread. Stateless: every member is static.
//
#pragma once
#ifndef POSE_EVALUATION_H
#define POSE_EVALUATION_H

#include "pose.h"

class MPoseEvaluation
{
public:
    MPoseEvaluation() = delete;

    // Samples every eval entry, blends (positions and scales linearly,
    // rotations by normalized quaternion sum on the bind rotation's
    // hemisphere; uncovered weight comes from the bind pose), then
    // computeModelSpace(). No entries = bind pose.
    static void evaluate(SAnimInstance& instance);

    // Walks bones parent-first from `local` to model and skinning matrices,
    // and the deform joints' box (model space).
    static void computeModelSpace(SAnimInstance& instance);
};

#endif // POSE_EVALUATION_H
