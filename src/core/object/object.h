//
// object.h
//
#pragma once
#ifndef METEOR_ENGINE_OBJECT_H
#define METEOR_ENGINE_OBJECT_H

#include "core/utils/sstring.h"
#include "object_class_macros.h" // DEFINE_MOBJECT_CLASS
#include "tools/lua_binding_generator/stubs/script_binding_macros.h"

SCRIPT_BIND_CLASS()
class MObject
{
    DEFINE_OBJECT_CLASS(MObject)  // introduces virtual MTypeInfo typeInfo() const

public:
    MObject();
    virtual ~MObject() = default;

    SCRIPT_BIND_FUNC()
    SString getGUID() const;

    SCRIPT_BIND_FUNC()
    virtual SString toString() const;

    SCRIPT_BIND_FUNC()
    virtual bool equals(const MObject* obj) const;

    static void operator delete(void* ptr);

    SCRIPT_BIND_FUNC()
    void setName(const SString& newName) { name = newName; }

    SCRIPT_BIND_FUNC()
    SString getName() const { return name; }

    // Call markDirty() when the object's state changes and needs saving.
    // The editor uses isDirty() to show `*` indicators and to determine
    // which assets to save on Ctrl+Shift+S.
    SCRIPT_BIND_FUNC()
    void markDirty()         { dirty = true; }

    SCRIPT_BIND_FUNC()
    void clearDirty()        { dirty = false; }

    SCRIPT_BIND_FUNC()
    bool isDirty() const { return dirty; }

protected:
    SString guid;
    SString name;

private:
    bool dirty = false;
};

#endif // METEOR_ENGINE_OBJECT_H