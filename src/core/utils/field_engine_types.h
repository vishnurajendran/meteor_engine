//
// field_engine_types.h
// Field<T> specializations for engine math types.
// Include this in spatial.h (and anything that inherits MSpatialEntity)
// AFTER the engine math headers are already visible.
//

#ifndef FIELD_ENGINE_TYPES_H
#define FIELD_ENGINE_TYPES_H

#include "data/field.h"
#include "glmhelper.h"
#include "core/utils/sstring.h"
#include "pugixml.hpp"

// ─── SVector3 ────────────────────────────────────────────────────────────────

template<>
inline void Field<SVector3>::write(pugi::xml_node& parent) const
{
    auto node = parent.append_child(name.c_str());
    node.append_child("x").text().set(value.x);
    node.append_child("y").text().set(value.y);
    node.append_child("z").text().set(value.z);
}

template<>
inline void Field<SVector3>::load(const pugi::xml_node& parent)
{
    auto node = parent.child(name.c_str());
    if (!node) return;
    value.x = node.child("x").text().as_float();
    value.y = node.child("y").text().as_float();
    value.z = node.child("z").text().as_float();
}

// ─── SQuaternion ─────────────────────────────────────────────────────────────

template<>
inline void Field<SQuaternion>::write(pugi::xml_node& parent) const
{
    auto node = parent.append_child(name.c_str());
    node.append_child("x").text().set(value.x);
    node.append_child("y").text().set(value.y);
    node.append_child("z").text().set(value.z);
    node.append_child("w").text().set(value.w);
}

template<>
inline void Field<SQuaternion>::load(const pugi::xml_node& parent)
{
    auto node = parent.child(name.c_str());
    if (!node) return;
    value.x = node.child("x").text().as_float();
    value.y = node.child("y").text().as_float();
    value.z = node.child("z").text().as_float();
    value.w = node.child("w").text().as_float();
}

// String type
template<>
inline void Field<SString>::write(pugi::xml_node& parent) const
{
    const auto node = parent.append_child(name.c_str());
    node.text().set(value.c_str());
}

template<>
inline void Field<SString>::load(const pugi::xml_node& parent)
{
    const auto node = parent.child(name.c_str());
    if (!node) return;
    value = node.text().as_string("");
}


#endif // FIELD_ENGINE_TYPES_H