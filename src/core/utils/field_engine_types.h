//
// field_engine_types.h
// Field<T> specializations for engine math types.
// Include this in spatial.h (and anything that inherits MSpatialEntity)
// AFTER the engine math headers are already visible.
//

#ifndef FIELD_ENGINE_TYPES_H
#define FIELD_ENGINE_TYPES_H

#include "data/field.h"
#include "data/list_field.h"
#include "glmhelper.h"
#include "core/utils/sstring.h"
#include "pugixml.hpp"

// =============================================================================
// Field<T> specialisations  (single-value fields)
// =============================================================================

// --- SVector3 ----------------------------------------------------------------

template<>
inline void Field<SVector3>::write(pugi::xml_node& parent) const
{
    auto node = parent.append_child(name.c_str());
    node.append_child("x").text().set(rawValue.x);
    node.append_child("y").text().set(rawValue.y);
    node.append_child("z").text().set(rawValue.z);
}

template<>
inline void Field<SVector3>::load(const pugi::xml_node& parent)
{
    auto node = parent.child(name.c_str());
    if (!node) return;
    rawValue.x = node.child("x").text().as_float();
    rawValue.y = node.child("y").text().as_float();
    rawValue.z = node.child("z").text().as_float();
}

// --- SQuaternion -------------------------------------------------------------

template<>
inline void Field<SQuaternion>::write(pugi::xml_node& parent) const
{
    auto node = parent.append_child(name.c_str());
    node.append_child("x").text().set(rawValue.x);
    node.append_child("y").text().set(rawValue.y);
    node.append_child("z").text().set(rawValue.z);
    node.append_child("w").text().set(rawValue.w);
}

template<>
inline void Field<SQuaternion>::load(const pugi::xml_node& parent)
{
    auto node = parent.child(name.c_str());
    if (!node) return;
    rawValue.x = node.child("x").text().as_float();
    rawValue.y = node.child("y").text().as_float();
    rawValue.z = node.child("z").text().as_float();
    rawValue.w = node.child("w").text().as_float();
}

// --- SString -----------------------------------------------------------------

template<>
inline void Field<SString>::write(pugi::xml_node& parent) const
{
    const auto node = parent.append_child(name.c_str());
    node.text().set(rawValue.c_str());
}

template<>
inline void Field<SString>::load(const pugi::xml_node& parent)
{
    const auto node = parent.child(name.c_str());
    if (!node) return;
    rawValue = node.text().as_string("");
}


// =============================================================================
// ListFieldElementIO<T> specialisations  (per-element serialisers for ListField)
// =============================================================================
//
// The primary template in list_field.h already handles:
//   int, float, double, bool, std::string, any enum.
//
// Add specialisations here for engine math types that can appear inside a
// DECLARE_LIST_FIELD.

// --- SVector3 ----------------------------------------------------------------
// XML per element:
//   <item>
//     <x>1.0</x>
//     <y>2.0</y>
//     <z>3.0</z>
//   </item>

template<>
struct ListFieldElementIO<SVector3>
{
    static void writeElement(pugi::xml_node& node, const SVector3& v)
    {
        node.append_child("x").text().set(v.x);
        node.append_child("y").text().set(v.y);
        node.append_child("z").text().set(v.z);
    }

    static SVector3 readElement(const pugi::xml_node& node)
    {
        return SVector3(
            node.child("x").text().as_float(),
            node.child("y").text().as_float(),
            node.child("z").text().as_float()
        );
    }
};

// --- SQuaternion -------------------------------------------------------------
// XML per element:
//   <item>
//     <x>0</x><y>0</y><z>0</z><w>1</w>
//   </item>

template<>
struct ListFieldElementIO<SQuaternion>
{
    static void writeElement(pugi::xml_node& node, const SQuaternion& q)
    {
        node.append_child("x").text().set(q.x);
        node.append_child("y").text().set(q.y);
        node.append_child("z").text().set(q.z);
        node.append_child("w").text().set(q.w);
    }

    static SQuaternion readElement(const pugi::xml_node& node)
    {
        return SQuaternion(
            node.child("w").text().as_float(1.0f), // glm quaternion: w first
            node.child("x").text().as_float(),
            node.child("y").text().as_float(),
            node.child("z").text().as_float()
        );
    }
};

// --- SString -----------------------------------------------------------------
// XML per element:
//   <item>Hello</item>

template<>
struct ListFieldElementIO<SString>
{
    static void writeElement(pugi::xml_node& node, const SString& s)
    {
        node.text().set(s.c_str());
    }

    static SString readElement(const pugi::xml_node& node)
    {
        return SString(node.text().as_string(""));
    }
};

#endif // FIELD_ENGINE_TYPES_H