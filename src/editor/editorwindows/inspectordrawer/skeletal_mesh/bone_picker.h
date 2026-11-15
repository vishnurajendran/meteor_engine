//
// bone_picker.h
//
// Bone dropdown shared by the bone socket and bone control inspectors. Lists
// the parent skeletal mesh's bones (deform bones first) with a filter box.
// A name that isn't in the skeleton is shown in red - it is kept, so a
// reimport that brings the bone back reconnects it.
//
#pragma once
#ifndef BONE_PICKER_H
#define BONE_PICKER_H

#include "core/utils/sstring.h"

struct SSkeleton;

namespace MBonePicker
{
    // Returns true when the user picked a bone (written to boneName).
    bool draw(const char* id, const SSkeleton* skeleton, SString& boneName);
}

#endif // BONE_PICKER_H
