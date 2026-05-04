//
// object_class_macros.h
//
// Root-level class macro. Introduces virtual typeInfo() on MObject.
// Every class in the engine inherits this virtual.
//
//  What DEFINE_MOBJECT_CLASS generates 
//
//   public:
//     // Compile-time type descriptor — name string + FNV-1a hash
//     static constexpr MTypeInfo staticTypeInfo()
//     { return MAKE_TYPE_INFO(ClassName); }
//
//     // Runtime virtual — overridden by every subclass
//     virtual MTypeInfo typeInfo() const
//     { return staticTypeInfo(); }
//
//   private:
//

#ifndef OBJECT_CLASS_MACROS_H
#define OBJECT_CLASS_MACROS_H

#include "type_info.h"

//  DEFINE_OBJECT_CLASS
//
#define DEFINE_OBJECT_CLASS(ClassName)                                        \
public:                                                                        \
    /** Compile-time type descriptor — usable without an instance */           \
    static constexpr MTypeInfo staticTypeInfo()                                \
    {                                                                          \
        return MAKE_TYPE_INFO(ClassName);                                      \
    }                                                                          \
    /** Runtime virtual — introduces the vtable slot */                        \
    virtual MTypeInfo typeInfo() const                                         \
    {                                                                          \
        return staticTypeInfo();                                               \
    }                                                                          \
private:

#endif // OBJECT_CLASS_MACROS_H