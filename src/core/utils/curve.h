//
// curve.h
//
// SCurve    — a float that changes over a normalised time t in [0, 1].
// SGradient — an RGBA colour that changes over t in [0, 1].
//
// Both are lists of keys, sorted by time, linearly interpolated between
// neighbouring keys and held flat before the first / after the last key.
//
//   SCurve size = SCurve::linear(1.0f, 0.0f);      // shrink to nothing
//   float  s    = size.evaluate(particle.normalizedAge());
//
// Both work as DECLARE_FIELD types — the Field<> specialisations at the
// bottom of this file save / load them:
//
//   <sizeCurve>
//     <key t="0" v="1"/>
//     <key t="1" v="0"/>
//   </sizeCurve>
//   <colorGradient>
//     <key t="0" r="1" g="1" b="1" a="1"/>
//     <key t="1" r="1" g="1" b="1" a="0"/>
//   </colorGradient>
//
// Gradient colours are authored values (sRGB, like every colour picked in
// the editor). Convert to linear before lighting maths — the particle
// shader does this per vertex.
//
// -- Cost -------------------------------------------------------------------------
//   evaluate() walks the keys from the start: O(keys). Curves usually have
//   2-5 keys, so this is a handful of compares per call. If profiling ever
//   shows curves hot (many keys × many particles), bake them into a small
//   lookup table on edit and evaluate in O(1).
//
#pragma once
#ifndef METEOR_ENGINE_CURVE_H
#define METEOR_ENGINE_CURVE_H

#include <initializer_list>
#include <vector>

#include "data/field.h"
#include "core/utils/glmhelper.h"
#include "pugixml.hpp"

// ---- SCurve -------------------------------------------------------------------

struct SCurveKey
{
    float time  = 0.0f;   // 0..1
    float value = 0.0f;

    bool operator==(const SCurveKey&) const = default;
};

struct SCurve
{
    std::vector<SCurveKey> keys;   // keep sorted by time — call sortKeys() after editing times

    SCurve() = default;
    SCurve(std::initializer_list<SCurveKey> init) : keys(init) { sortKeys(); }

    static SCurve constant(float value)          { return SCurve{ { 0.0f, value } }; }
    static SCurve linear(float from, float to)   { return SCurve{ { 0.0f, from }, { 1.0f, to } }; }

    // Value at t. t outside the keys' range holds the end value. No keys —
    // returns 0.
    [[nodiscard]] float evaluate(float t) const;

    // Index of the key inserted for (time, value).
    int  addKey(float time, float value);
    void removeKey(int index);
    void sortKeys();

    // Smallest / largest key value — editor uses it to frame the graph.
    void getValueRange(float& outMin, float& outMax) const;

    bool operator==(const SCurve&) const = default;
};

// ---- SGradient ----------------------------------------------------------------

struct SGradientKey
{
    float    time  = 0.0f;               // 0..1
    SVector4 color = SVector4(1.0f);     // rgba, sRGB colour + linear alpha

    bool operator==(const SGradientKey&) const = default;
};

struct SGradient
{
    std::vector<SGradientKey> keys;   // keep sorted by time

    SGradient() = default;
    SGradient(std::initializer_list<SGradientKey> init) : keys(init) { sortKeys(); }

    static SGradient constant(const SVector4& color)                 { return SGradient{ { 0.0f, color } }; }
    static SGradient linear(const SVector4& from, const SVector4& to) { return SGradient{ { 0.0f, from }, { 1.0f, to } }; }

    // Colour at t. No keys — returns opaque white.
    [[nodiscard]] SVector4 evaluate(float t) const;

    int  addKey(float time, const SVector4& color);
    void removeKey(int index);
    void sortKeys();

    bool operator==(const SGradient&) const = default;
};

// ---- Field<> specialisations (save / load as DECLARE_FIELD members) -----------
// Defined here, next to the types, so no translation unit can use
// Field<SCurve> without seeing them.

template<>
inline void Field<SCurve>::write(pugi::xml_node& parent) const
{
    pugi::xml_node node = parent.append_child(name.c_str());
    for (const SCurveKey& k : rawValue.keys)
    {
        pugi::xml_node key = node.append_child("key");
        key.append_attribute("t").set_value(k.time);
        key.append_attribute("v").set_value(k.value);
    }
}

template<>
inline void Field<SCurve>::load(const pugi::xml_node& parent)
{
    const pugi::xml_node node = parent.child(name.c_str());
    if (!node) return;   // missing — keep the default

    rawValue.keys.clear();
    for (const pugi::xml_node key : node.children("key"))
        rawValue.keys.push_back({ key.attribute("t").as_float(), key.attribute("v").as_float() });
    rawValue.sortKeys();
}

template<>
inline void Field<SGradient>::write(pugi::xml_node& parent) const
{
    pugi::xml_node node = parent.append_child(name.c_str());
    for (const SGradientKey& k : rawValue.keys)
    {
        pugi::xml_node key = node.append_child("key");
        key.append_attribute("t").set_value(k.time);
        key.append_attribute("r").set_value(k.color.r);
        key.append_attribute("g").set_value(k.color.g);
        key.append_attribute("b").set_value(k.color.b);
        key.append_attribute("a").set_value(k.color.a);
    }
}

template<>
inline void Field<SGradient>::load(const pugi::xml_node& parent)
{
    const pugi::xml_node node = parent.child(name.c_str());
    if (!node) return;

    rawValue.keys.clear();
    for (const pugi::xml_node key : node.children("key"))
    {
        SGradientKey k;
        k.time  = key.attribute("t").as_float();
        k.color = SVector4(key.attribute("r").as_float(1.0f),
                           key.attribute("g").as_float(1.0f),
                           key.attribute("b").as_float(1.0f),
                           key.attribute("a").as_float(1.0f));
        rawValue.keys.push_back(k);
    }
    rawValue.sortKeys();
}

#endif // METEOR_ENGINE_CURVE_H
