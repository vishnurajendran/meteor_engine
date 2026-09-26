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

#pragma once
#ifndef METEOR_MATH_BINDINGS_H
#define METEOR_MATH_BINDINGS_H

#include <sol/sol.hpp>
#include "core/utils/glmhelper.h"

namespace MeteorBindings {

static void registerMathBindings(sol::state& lua) {

    // ---- SVector2 ----
    lua.new_usertype<SVector2>("SVector2",
        sol::constructors<
            SVector2(),
            SVector2(float, float)
        >(),
        "x", &SVector2::x,
        "y", &SVector2::y,

        // Arithmetic
        sol::meta_function::addition,       sol::resolve<SVector2(const SVector2&, const SVector2&)>(glm::operator+),
        sol::meta_function::subtraction,    sol::resolve<SVector2(const SVector2&, const SVector2&)>(glm::operator-),
        sol::meta_function::multiplication, sol::overload(
            sol::resolve<SVector2(const SVector2&, float)>(glm::operator*),
            sol::resolve<SVector2(float, const SVector2&)>(glm::operator*)
        ),

        // tostring for print()
        sol::meta_function::to_string, [](const SVector2& v) {
            return "SVector2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
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
        sol::meta_function::addition,       sol::resolve<SVector3(const SVector3&, const SVector3&)>(glm::operator+),
        sol::meta_function::subtraction,    sol::resolve<SVector3(const SVector3&, const SVector3&)>(glm::operator-),
        sol::meta_function::multiplication, sol::overload(
            sol::resolve<SVector3(const SVector3&, float)>(glm::operator*),
            sol::resolve<SVector3(float, const SVector3&)>(glm::operator*)
        ),
        sol::meta_function::unary_minus,    [](const SVector3& v) { return -v; },

        // Utility
        "length",    [](const SVector3& v) { return glm::length(v); },
        "normalized",[](const SVector3& v) { return glm::normalize(v); },
        "dot",       [](const SVector3& a, const SVector3& b) { return glm::dot(a, b); },
        "cross",     [](const SVector3& a, const SVector3& b) { return glm::cross(a, b); },
        "lerp",      [](const SVector3& a, const SVector3& b, float t) { return glm::mix(a, b, t); },

        sol::meta_function::to_string, [](const SVector3& v) {
            return "SVector3(" + std::to_string(v.x) + ", "
                               + std::to_string(v.y) + ", "
                               + std::to_string(v.z) + ")";
        }
    );

    // ---- SVector4 ----
    lua.new_usertype<SVector4>("SVector4",
        sol::constructors<
            SVector4(),
            SVector4(float, float, float, float)
        >(),
        "x", &SVector4::x,
        "y", &SVector4::y,
        "z", &SVector4::z,
        "w", &SVector4::w,

        sol::meta_function::addition,       sol::resolve<SVector4(const SVector4&, const SVector4&)>(glm::operator+),
        sol::meta_function::subtraction,    sol::resolve<SVector4(const SVector4&, const SVector4&)>(glm::operator-),
        sol::meta_function::multiplication, sol::overload(
            sol::resolve<SVector4(const SVector4&, float)>(glm::operator*),
            sol::resolve<SVector4(float, const SVector4&)>(glm::operator*)
        ),

        sol::meta_function::to_string, [](const SVector4& v) {
            return "SVector4(" + std::to_string(v.x) + ", "
                               + std::to_string(v.y) + ", "
                               + std::to_string(v.z) + ", "
                               + std::to_string(v.w) + ")";
        }
    );

    // ---- SQuaternion ----
    lua.new_usertype<SQuaternion>("SQuaternion",
        sol::constructors<
            SQuaternion(),
            SQuaternion(float, float, float, float)
        >(),
        "x", &SQuaternion::x,
        "y", &SQuaternion::y,
        "z", &SQuaternion::z,
        "w", &SQuaternion::w,

        // Quaternion multiplication (composition)
        sol::meta_function::multiplication, sol::resolve<SQuaternion(const SQuaternion&, const SQuaternion&)>(glm::operator*),

        // Conversions
        "toEuler",  [](const SQuaternion& q) { return quaternionToEuler(q); },

        // Utility
        "normalized", [](const SQuaternion& q) { return glm::normalize(q); },
        "inverse",    [](const SQuaternion& q) { return glm::inverse(q); },
        "slerp",      [](const SQuaternion& a, const SQuaternion& b, float t) {
            return glm::slerp(a, b, t);
        },

        sol::meta_function::to_string, [](const SQuaternion& q) {
            return "SQuaternion(" + std::to_string(q.x) + ", "
                                  + std::to_string(q.y) + ", "
                                  + std::to_string(q.z) + ", "
                                  + std::to_string(q.w) + ")";
        }
    );

    // ---- Free functions ----
    lua["eulerToQuaternion"] = [](float x, float y, float z) {
        return eulerToQuaternion(SVector3(x, y, z));
    };
}

} // namespace MeteorBindings

#endif // METEOR_MATH_BINDINGS_H