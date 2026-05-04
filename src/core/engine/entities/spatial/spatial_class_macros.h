//
// spatial_class_macros.h
//
// ── Usage ─────────────────────────────────────────────────────────────────
//
//   // MyEntity.h
//   class MCamera : public MSpatialEntity {
//       DEFINE_CLASS(MCamera)
//   private:
//       DECLARE_FIELD(fov, float, 90.0f)
//   };
//
//   // MyEntity.cpp  — ONE call per class, in the .cpp only, never in a header
//   IMPLEMENT_CLASS(MCamera)
//
// ── Why the split matters ─────────────────────────────────────────────────
//
//   DEFINE_CLASS puts only a *declaration* of _classRegistered in the header.
//   IMPLEMENT_CLASS puts the *definition* (with the factory lambda) in the .cpp.
//
//   At the point IMPLEMENT_CLASS expands in the .cpp the class is already
//   fully defined (its header was already included), so ClassName* → MSpatialEntity*
//   is a valid implicit conversion and the lambda compiles cleanly.
//
//   inline static with a body-level lambda fails because the class is still
//   *incomplete* when the class body is being parsed — the compiler cannot yet
//   verify the inheritance relationship needed for the pointer conversion.
//
//   One IMPLEMENT_CLASS per class per binary → exactly one definition of
//   _classRegistered → no ODR violations.
//

#ifndef SPATIAL_CLASS_MACROS_H
#define SPATIAL_CLASS_MACROS_H

#include "core/object/type_info.h"
#include "core/engine/entities/entity_type_registry.h"

// ── DEFINE_CLASS ──────────────────────────────────────────────────────────
// Place as the first item inside any concrete MSpatialEntity subclass body.
// Injects typeInfo(), staticTypeInfo(), create(), and a *declaration* of
// _classRegistered. The matching IMPLEMENT_CLASS in the .cpp provides the
// definition.
//
#define DEFINE_CLASS(ClassName)                                                \
public:                                                                        \
    static constexpr MTypeInfo staticTypeInfo()                                \
    {                                                                          \
        return MAKE_TYPE_INFO(ClassName);                                      \
    }                                                                          \
    MTypeInfo typeInfo() const override                                        \
    {                                                                          \
        return staticTypeInfo();                                               \
    }                                                                          \
    static ClassName* create(const SString& name = {})                        \
    {                                                                          \
        return MSpatialEntity::createInstance<ClassName>(name);               \
    }                                                                          \
private:                                                                       \
    /* Declaration only — definition is in IMPLEMENT_CLASS in the .cpp */     \
    static const bool _classRegistered;

// ── IMPLEMENT_CLASS ───────────────────────────────────────────────────────
// Place exactly once per class in its .cpp file.
// At this point the class is complete, so ClassName* → MSpatialEntity* is
// valid and the factory lambda compiles without the "incomplete type" error.
//
#define IMPLEMENT_CLASS(ClassName)                                             \
    const bool ClassName::_classRegistered = []() {                            \
        MEntityTypeRegistry::get().registerType(                               \
            ClassName::staticTypeInfo(),                                       \
            []() -> MSpatialEntity* { return ClassName::create(SString{}); }   \
        );                                                                     \
        return true;                                                           \
    }();

// ── DEFINE_ABSTRACT_CLASS ─────────────────────────────────────────────────
// Use for abstract intermediate classes (MLightEntity, MDynamicLight, etc.)
// Provides typeInfo() and staticTypeInfo() but no factory and no registry
// entry — abstract classes cannot be directly instantiated.
//
#define DEFINE_ABSTRACT_CLASS(ClassName)                                       \
public:                                                                        \
    static constexpr MTypeInfo staticTypeInfo()                                \
    {                                                                          \
        return MAKE_TYPE_INFO(ClassName);                                      \
    }                                                                          \
    MTypeInfo typeInfo() const override                                        \
    {                                                                          \
        return staticTypeInfo();                                               \
    }                                                                          \
private:

#endif // SPATIAL_CLASS_MACROS_H