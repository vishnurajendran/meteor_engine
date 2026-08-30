// glmhelper.h  (STUB for binding generator parsing)
#ifndef ENGINE_TYPES_STUB_H
#define ENGINE_TYPES_STUB_H

#include "std_stub.h"

struct SVector2 {
    float x, y;
    SVector2() : x(0), y(0) {}
    SVector2(float v) : x(v), y(v) {}
    SVector2(float x, float y) : x(x), y(y) {}
};

struct SVector3 {
    float x, y, z;
    SVector3() : x(0), y(0), z(0) {}
    SVector3(float v) : x(v), y(v), z(v) {}
    SVector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct SVector4 {
    float x, y, z, w;
};

struct SQuaternion {
    float x, y, z, w;
};

struct SMatrix4 {
    float m[16];
};

using SString = std::string;

// Smart pointer for ref props
template<typename T>
class M {
public:
    T* get() const { return nullptr; }
    T* operator->() const { return nullptr; }
    explicit operator bool() const { return false; }
};

#endif // ENGINE_TYPES_STUB_H
