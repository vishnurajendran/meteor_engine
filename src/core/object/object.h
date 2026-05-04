//
// object.h
//
#pragma once
#ifndef METEOR_ENGINE_OBJECT_H
#define METEOR_ENGINE_OBJECT_H

#include <cassert>


#include "core/utils/logger.h"
#include "core/utils/sstring.h"
#include "object_class_macros.h" // DEFINE_MOBJECT_CLASS

class MObject
{
    DEFINE_OBJECT_CLASS(MObject)  // introduces virtual MTypeInfo typeInfo() const

public:
    MObject();
    virtual ~MObject() = default;

    [[nodiscard]] SString getGUID() const;
    [[nodiscard]] virtual SString toString() const;
    [[nodiscard]] virtual bool equals(const MObject* obj) const;

    void setName(const SString& newName) { name = newName; }
    [[nodiscard]] SString getName() const { return name; }

    // delete operation
    void operator delete(void* ptr);

protected:
    SString guid;
    SString name;
};

#endif // METEOR_ENGINE_OBJECT_H