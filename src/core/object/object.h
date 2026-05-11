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

/// Base class for all objects within meteor.
/// it provides essential functions to be used within the engine
class MObject
{
    // introduces virtual MTypeInfo typeInfo() const
    DEFINE_OBJECT_CLASS(MObject)

public:
    MObject();
    virtual ~MObject() = default;

    /// returns the unique instance id for this object
    [[nodiscard]] SString getGUID() const;
    /// returns string representation of this object.
    [[nodiscard]] virtual SString toString() const;
    /// checks if two object instances are equal. default impl check guid equality
    [[nodiscard]] virtual bool equals(const MObject* obj) const;

    /// sets the name of this object
    void setName(const SString& newName) { name = newName; }
    /// gets the name of this object
    [[nodiscard]] SString getName() const { return name; }

    // delete operation
    void operator delete(void* ptr);

protected:
    SString guid;
    SString name;
};

#endif // METEOR_ENGINE_OBJECT_H