//
// math_bindings.h
//
// Manual Sol2 bindings for engine math types.
// These are typedefs to GLM and cannot use the SCRIPT_BIND_CLASS annotation
// system, so they are registered by hand.
//
// Call MeteorBindings::registerMathBindings(lua) during Lua state init,
// BEFORE registerAllBindings() since bound classes use these types in
// their getter/setter signatures.
//
// Conventions scripts can rely on (documented in meteor_api.lua too):
//   - Angles are in DEGREES everywhere in the Lua API.
//   - Right-handed, Y up. Forward is -Z (matches MSpatialEntity::getForwardVector).
//   - SQuaternion.new takes (w, x, y, z) - GLM's constructor order.
//   - Functions never return NaN for zero-length input: normalizing a zero
//     vector gives a zero vector, angles involving one give 0.
//
// Scalar helpers (clamp, lerp, smoothDamp, ...) live in the Lua engine
// module MMath (.engine_data/scripting/modules/lua/MMath.lua).
//

#pragma once
#ifndef METEOR_MATH_BINDINGS_H
#define METEOR_MATH_BINDINGS_H

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>

#include <sol/sol.hpp>
#include "core/utils/glmhelper.h"
#include "core/utils/color.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MeteorBindings {

namespace MathDetail {

    constexpr float EPSILON = 1e-6f;

    template <typename V>
    V safeNormalize(const V& v)
    {
        const float len = glm::length(v);
        return len > EPSILON ? v / len : V(0.0f);
    }

    template <typename V>
    float lengthSquared(const V& v) { return glm::dot(v, v); }

    template <typename V>
    bool approxEquals(const V& a, const V& b, float epsilon)
    {
        return lengthSquared(a - b) <= epsilon * epsilon;
    }

    // Unsigned angle between two directions, in degrees [0, 180].
    inline float angleBetween(const SVector3& a, const SVector3& b)
    {
        const float denom = std::sqrt(lengthSquared(a) * lengthSquared(b));
        if (denom < EPSILON) return 0.0f;
        const float c = std::clamp(glm::dot(a, b) / denom, -1.0f, 1.0f);
        return glm::degrees(std::acos(c));
    }

    inline SVector3 moveTowards(const SVector3& current, const SVector3& target, float maxDelta)
    {
        const SVector3 delta = target - current;
        const float dist = glm::length(delta);
        if (dist <= maxDelta || dist < EPSILON) return target;
        return current + delta / dist * maxDelta;
    }

    inline SVector3 clampLength(const SVector3& v, float maxLength)
    {
        const float len = glm::length(v);
        return len > maxLength && len > EPSILON ? v / len * maxLength : v;
    }

    inline SVector3 project(const SVector3& v, const SVector3& onNormal)
    {
        const float sq = lengthSquared(onNormal);
        return sq < EPSILON ? SVector3(0.0f) : onNormal * (glm::dot(v, onNormal) / sq);
    }

    // Angle between two unit quaternions in degrees [0, 180].
    inline float quatAngle(const SQuaternion& a, const SQuaternion& b)
    {
        const float d = std::min(std::abs(glm::dot(glm::normalize(a), glm::normalize(b))), 1.0f);
        return glm::degrees(2.0f * std::acos(d));
    }

    inline SQuaternion lookRotation(const SVector3& forward, const SVector3& up)
    {
        const SVector3 f = safeNormalize(forward);
        if (lengthSquared(f) < EPSILON) return glm::identity<SQuaternion>();
        SVector3 u = safeNormalize(up);
        // Forward parallel to up: any perpendicular up will do.
        if (std::abs(glm::dot(f, u)) > 0.9999f)
            u = std::abs(f.y) < 0.9f ? SVector3(0, 1, 0) : SVector3(0, 0, 1);
        return glm::quatLookAtRH(f, u);   // forward maps to -Z, like entities
    }

    inline std::string vecToString(const char* name, std::initializer_list<float> parts)
    {
        std::string s = std::string(name) + "(";
        bool first = true;
        for (float p : parts)
        {
            if (!first) s += ", ";
            s += std::to_string(p);
            first = false;
        }
        return s + ")";
    }

} // namespace MathDetail

static void registerMathBindings(sol::state& lua) {
    using namespace MathDetail;

    // ---- SVector2 ----
    lua.new_usertype<SVector2>("SVector2",
        sol::constructors<
            SVector2(),
            SVector2(float),
            SVector2(float, float)
        >(),
        "x", &SVector2::x,
        "y", &SVector2::y,

        // Arithmetic
        sol::meta_function::addition,       [](const SVector2& a, const SVector2& b) { return a + b; },
        sol::meta_function::subtraction,    [](const SVector2& a, const SVector2& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const SVector2& v, float s) { return v * s; },
            [](float s, const SVector2& v) { return s * v; },
            [](const SVector2& a, const SVector2& b) { return a * b; }   // component-wise
        ),
        sol::meta_function::division,       sol::overload(
            [](const SVector2& v, float s) { return v / s; },
            [](const SVector2& a, const SVector2& b) { return a / b; }
        ),
        sol::meta_function::unary_minus,    [](const SVector2& v) { return -v; },
        sol::meta_function::equal_to,       [](const SVector2& a, const SVector2& b) { return a == b; },

        // Utility
        "length",        [](const SVector2& v) { return glm::length(v); },
        "lengthSquared", [](const SVector2& v) { return lengthSquared(v); },
        "normalized",    [](const SVector2& v) { return safeNormalize(v); },
        "dot",           [](const SVector2& a, const SVector2& b) { return glm::dot(a, b); },
        "distance",      [](const SVector2& a, const SVector2& b) { return glm::distance(a, b); },
        "lerp",          [](const SVector2& a, const SVector2& b, float t) { return glm::mix(a, b, t); },
        "min",           [](const SVector2& a, const SVector2& b) { return glm::min(a, b); },
        "max",           [](const SVector2& a, const SVector2& b) { return glm::max(a, b); },
        "approxEquals",  sol::overload(
            [](const SVector2& a, const SVector2& b) { return approxEquals(a, b, 1e-4f); },
            [](const SVector2& a, const SVector2& b, float eps) { return approxEquals(a, b, eps); }
        ),
        "zero",          []() { return SVector2(0.0f); },
        "one",           []() { return SVector2(1.0f); },

        sol::meta_function::to_string, [](const SVector2& v) {
            return vecToString("SVector2", { v.x, v.y });
        }
    );

    // ---- SVector3 ----
    lua.new_usertype<SVector3>("SVector3",
        sol::constructors<
            SVector3(),
            SVector3(float),
            SVector3(float, float, float)
        >(),
        "x", &SVector3::x,
        "y", &SVector3::y,
        "z", &SVector3::z,

        // Arithmetic
        sol::meta_function::addition,       [](const SVector3& a, const SVector3& b) { return a + b; },
        sol::meta_function::subtraction,    [](const SVector3& a, const SVector3& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const SVector3& v, float s) { return v * s; },
            [](float s, const SVector3& v) { return s * v; },
            [](const SVector3& a, const SVector3& b) { return a * b; }   // component-wise
        ),
        sol::meta_function::division,       sol::overload(
            [](const SVector3& v, float s) { return v / s; },
            [](const SVector3& a, const SVector3& b) { return a / b; }
        ),
        sol::meta_function::unary_minus,    [](const SVector3& v) { return -v; },
        sol::meta_function::equal_to,       [](const SVector3& a, const SVector3& b) { return a == b; },

        // Length / direction
        "length",          [](const SVector3& v) { return glm::length(v); },
        "lengthSquared",   [](const SVector3& v) { return lengthSquared(v); },
        "normalized",      [](const SVector3& v) { return safeNormalize(v); },
        "clampLength",     [](const SVector3& v, float maxLength) { return clampLength(v, maxLength); },

        // Products
        "dot",             [](const SVector3& a, const SVector3& b) { return glm::dot(a, b); },
        "cross",           [](const SVector3& a, const SVector3& b) { return glm::cross(a, b); },
        "scale",           [](const SVector3& a, const SVector3& b) { return a * b; },

        // Distance / angle
        "distance",        [](const SVector3& a, const SVector3& b) { return glm::distance(a, b); },
        "distanceSquared", [](const SVector3& a, const SVector3& b) { return lengthSquared(a - b); },
        "angle",           [](const SVector3& a, const SVector3& b) { return angleBetween(a, b); },
        "signedAngle",     [](const SVector3& a, const SVector3& b, const SVector3& axis) {
            const float angle = angleBetween(a, b);
            return glm::dot(axis, glm::cross(a, b)) < 0.0f ? -angle : angle;
        },

        // Projection
        "project",         [](const SVector3& v, const SVector3& onNormal) { return project(v, onNormal); },
        "projectOnPlane",  [](const SVector3& v, const SVector3& planeNormal) { return v - project(v, planeNormal); },
        "reflect",         [](const SVector3& v, const SVector3& normal) { return glm::reflect(v, safeNormalize(normal)); },

        // Interpolation
        "lerp",            [](const SVector3& a, const SVector3& b, float t) { return glm::mix(a, b, t); },
        "moveTowards",     [](const SVector3& current, const SVector3& target, float maxDelta) {
            return moveTowards(current, target, maxDelta);
        },

        // Component-wise
        "min",             [](const SVector3& a, const SVector3& b) { return glm::min(a, b); },
        "max",             [](const SVector3& a, const SVector3& b) { return glm::max(a, b); },
        "abs",             [](const SVector3& v) { return glm::abs(v); },
        "approxEquals",    sol::overload(
            [](const SVector3& a, const SVector3& b) { return approxEquals(a, b, 1e-4f); },
            [](const SVector3& a, const SVector3& b, float eps) { return approxEquals(a, b, eps); }
        ),

        // Constants (new value each call, so scripts can't mutate a shared one)
        "zero",            []() { return SVector3(0.0f); },
        "one",             []() { return SVector3(1.0f); },
        "up",              []() { return SVector3(0, 1, 0); },
        "down",            []() { return SVector3(0, -1, 0); },
        "right",           []() { return SVector3(1, 0, 0); },
        "left",            []() { return SVector3(-1, 0, 0); },
        "forward",         []() { return SVector3(0, 0, -1); },
        "back",            []() { return SVector3(0, 0, 1); },

        sol::meta_function::to_string, [](const SVector3& v) {
            return vecToString("SVector3", { v.x, v.y, v.z });
        }
    );

    // ---- SVector4 ----
    lua.new_usertype<SVector4>("SVector4",
        sol::constructors<
            SVector4(),
            SVector4(float),
            SVector4(float, float, float, float)
        >(),
        "x", &SVector4::x,
        "y", &SVector4::y,
        "z", &SVector4::z,
        "w", &SVector4::w,

        sol::meta_function::addition,       [](const SVector4& a, const SVector4& b) { return a + b; },
        sol::meta_function::subtraction,    [](const SVector4& a, const SVector4& b) { return a - b; },
        sol::meta_function::multiplication, sol::overload(
            [](const SVector4& v, float s) { return v * s; },
            [](float s, const SVector4& v) { return s * v; },
            [](const SVector4& a, const SVector4& b) { return a * b; }
        ),
        sol::meta_function::division,       sol::overload(
            [](const SVector4& v, float s) { return v / s; },
            [](const SVector4& a, const SVector4& b) { return a / b; }
        ),
        sol::meta_function::unary_minus,    [](const SVector4& v) { return -v; },
        sol::meta_function::equal_to,       [](const SVector4& a, const SVector4& b) { return a == b; },

        "length",        [](const SVector4& v) { return glm::length(v); },
        "lengthSquared", [](const SVector4& v) { return lengthSquared(v); },
        "normalized",    [](const SVector4& v) { return safeNormalize(v); },
        "dot",           [](const SVector4& a, const SVector4& b) { return glm::dot(a, b); },
        "lerp",          [](const SVector4& a, const SVector4& b, float t) { return glm::mix(a, b, t); },
        "zero",          []() { return SVector4(0.0f); },
        "one",           []() { return SVector4(1.0f); },

        sol::meta_function::to_string, [](const SVector4& v) {
            return vecToString("SVector4", { v.x, v.y, v.z, v.w });
        }
    );

    // ---- SQuaternion ----
    lua.new_usertype<SQuaternion>("SQuaternion",
        sol::constructors<
            SQuaternion(),
            SQuaternion(float, float, float, float)   // (w, x, y, z)
        >(),
        "x", &SQuaternion::x,
        "y", &SQuaternion::y,
        "z", &SQuaternion::z,
        "w", &SQuaternion::w,

        // q1 * q2 composes (q2 first, then q1); q * v rotates a vector.
        sol::meta_function::multiplication, sol::overload(
            [](const SQuaternion& a, const SQuaternion& b) { return a * b; },
            [](const SQuaternion& q, const SVector3& v) { return q * v; }
        ),
        sol::meta_function::equal_to, [](const SQuaternion& a, const SQuaternion& b) { return a == b; },

        // Construction
        "identity",       []() { return glm::identity<SQuaternion>(); },
        "fromEuler",      sol::overload(
            [](float x, float y, float z) { return eulerToQuaternion(SVector3(x, y, z)); },
            [](const SVector3& e) { return eulerToQuaternion(e); }
        ),
        "fromAxisAngle",  [](const SVector3& axis, float degrees) {
            const SVector3 a = safeNormalize(axis);
            if (lengthSquared(a) < EPSILON) return glm::identity<SQuaternion>();
            return glm::angleAxis(glm::radians(degrees), a);
        },
        "lookRotation",   sol::overload(
            [](const SVector3& forward) { return lookRotation(forward, SVector3(0, 1, 0)); },
            [](const SVector3& forward, const SVector3& up) { return lookRotation(forward, up); }
        ),
        "fromToRotation", [](const SVector3& from, const SVector3& to) {
            const SVector3 f = safeNormalize(from), t = safeNormalize(to);
            if (lengthSquared(f) < EPSILON || lengthSquared(t) < EPSILON) return glm::identity<SQuaternion>();
            return glm::rotation(f, t);
        },

        // Conversions
        "toEuler",        [](const SQuaternion& q) { return quaternionToEuler(q); },
        "rotate",         [](const SQuaternion& q, const SVector3& v) { return q * v; },
        "forward",        [](const SQuaternion& q) { return q * SVector3(0, 0, -1); },
        "right",          [](const SQuaternion& q) { return q * SVector3(1, 0, 0); },
        "up",             [](const SQuaternion& q) { return q * SVector3(0, 1, 0); },

        // Utility
        "normalized",     [](const SQuaternion& q) { return glm::normalize(q); },
        "inverse",        [](const SQuaternion& q) { return glm::inverse(q); },
        "dot",            [](const SQuaternion& a, const SQuaternion& b) { return glm::dot(a, b); },
        "angle",          [](const SQuaternion& a, const SQuaternion& b) { return quatAngle(a, b); },
        "slerp",          [](const SQuaternion& a, const SQuaternion& b, float t) {
            return glm::slerp(a, b, std::clamp(t, 0.0f, 1.0f));
        },
        "rotateTowards",  [](const SQuaternion& from, const SQuaternion& to, float maxDegrees) {
            const float angle = quatAngle(from, to);
            if (angle < EPSILON) return to;
            return glm::slerp(from, to, std::min(1.0f, maxDegrees / angle));
        },

        sol::meta_function::to_string, [](const SQuaternion& q) {
            return vecToString("SQuaternion(w,x,y,z)", { q.w, q.x, q.y, q.z });
        }
    );

    // ---- SColor ----
    // Plain struct (not a GLM type), but bound here with the other value
    // types so scripts can construct one: SColor.new(r, g, b, a).
    // Light setColor/getColor take and return it.
    lua.new_usertype<SColor>("SColor",
        sol::constructors<
            SColor(float, float, float, float)
        >(),
        "r", &SColor::r,
        "g", &SColor::g,
        "b", &SColor::b,
        "a", &SColor::a,
        "red",     &SColor::red,
        "green",   &SColor::green,
        "blue",    &SColor::blue,
        "yellow",  &SColor::yellow,
        "skyBlue", &SColor::skyBlue,
        "lerp",    [](const SColor& a, const SColor& b, float t) {
            return SColor(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
                          a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
        },
        sol::meta_function::to_string, [](const SColor& c) {
            return vecToString("SColor", { c.r, c.g, c.b, c.a });
        }
    );

    // ---- Free functions ----
    // Kept for existing scripts; same as SQuaternion.fromEuler. Degrees.
    lua["eulerToQuaternion"] = [](float x, float y, float z) {
        return eulerToQuaternion(SVector3(x, y, z));
    };
}

} // namespace MeteorBindings

#endif // METEOR_MATH_BINDINGS_H
