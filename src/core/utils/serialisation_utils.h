//
// Created by ssj5v on 27-09-2024.
//

#pragma once
#ifndef VECTORUTILS_H
#define VECTORUTILS_H

#include <core/utils/glmhelper.h>
#include "sstring.h"

SString serializeVector (SVector3 vector);
bool parseVector (SString str, SVector3& out);
SString serializeQuaternion (SQuaternion quaternion);
bool parseQuaternion (SString str, SQuaternion& out);

#endif //VECTORUTILS_H
