//
// object_class_macros.h
//
// Root-level class macro. Introduces virtual typeInfo() on MObject.
// Every class in the engine inherits this virtual.
//
//  What DEFINE_MOBJECT_CLASS generates 
//
//   public:
//     // Compile-time type descriptor - name string + FNV-1a hash
//     static constexpr MTypeInfo staticTypeInfo()
//     { return MAKE_TYPE_INFO(ClassName); }
//
//     // Runtime virtual - overridden by every subclass
//     virtual MTypeInfo typeInfo() const
//     { return staticTypeInfo(); }
//
//   private:
//

#ifndef OBJECT_CLASS_MACROS_H
#define OBJECT_CLASS_MACROS_H

#include "type_info.h"

// ── DEFINE_OBJECT_CLASS ───────────────────────────────────────────────────
// ROOT ONLY - used by MObject to *introduce* the virtual typeInfo() slot.
// No other class should use this macro.
//
#define DEFINE_OBJECT_CLASS(ClassName)                                         \
public:                                                                        \
    /** Compile-time type descriptor - usable without an instance */           \
    static constexpr MTypeInfo staticTypeInfo()                                \
    {                                                                          \
        return MAKE_TYPE_INFO(ClassName);                                      \
    }                                                                          \
    /** Runtime virtual - introduces the vtable slot */                        \
    virtual MTypeInfo typeInfo() const                                         \
    {                                                                          \
        return staticTypeInfo();                                               \
    }                                                                          \
private:

// ── DEFINE_OBJECT_SUBCLASS ────────────────────────────────────────────────
// For any non-spatial class that inherits MObject (directly or indirectly).
// Uses `override` instead of `virtual` so the compiler enforces that the
// base-class signature matches.
//
// Usage:
//   class MStaticMesh : public MObject {
//       DEFINE_OBJECT_SUBCLASS(MStaticMesh)
//   };
//
#define DEFINE_OBJECT_SUBCLASS(ClassName)                                      \
public:                                                                        \
    static constexpr MTypeInfo staticTypeInfo()                                \
    {                                                                          \
        return MAKE_TYPE_INFO(ClassName);                                      \
    }                                                                          \
    virtual MTypeInfo typeInfo() const override                                \
    {                                                                          \
        return staticTypeInfo();                                               \
    }                                                                          \
private:

#endif // OBJECT_CLASS_MACROS_H