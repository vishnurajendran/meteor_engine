//
// object.h
//
#pragma once
#ifndef METEOR_ENGINE_OBJECT_H
#define METEOR_ENGINE_OBJECT_H

#include "core/utils/sstring.h"
#include "object_class_macros.h"   // DEFINE_MOBJECT_CLASS

class MObject
{
    DEFINE_OBJECT_CLASS(MObject)  // introduces virtual MTypeInfo typeInfo() const

public:
    MObject();
    virtual ~MObject() = default;

    [[nodiscard]] SString getGUID() const;
    [[nodiscard]] virtual SString toString() const;
    [[nodiscard]] virtual bool equals(const MObject* obj) const;

    static void operator delete(void* ptr);

    void setName(const SString& newName) { name = newName; }
    [[nodiscard]] SString getName() const { return name; }

    // Call markDirty() when the object's state changes and needs saving.
    // The editor uses isDirty() to show `*` indicators and to determine
    // which assets to save on Ctrl+Shift+S.
    void markDirty()         { dirty = true; }
    void clearDirty()        { dirty = false; }
    [[nodiscard]] bool isDirty() const { return dirty; }

protected:
    SString guid;
    SString name;

private:
    bool dirty = false;
};

#endif // METEOR_ENGINE_OBJECT_H