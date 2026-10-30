#include "object.h"
#include "core/object/gc.h"
#include "core/memory/mnew.h"
#include "core/utils/guid.h"
#include "core/utils/logger.h"

MObject::MObject()
{
    guid = SGuid::newGUID();
    name = "MObject";
    MVERBOSE(SString::format("Created MObject {0} ({1})", name, guid));
}

SString MObject::getGUID() const
{
    return guid;
}

SString MObject::toString() const
{
    return name + STR(" (GUID: ") + guid + STR(")");
}

bool MObject::equals(const MObject* obj) const
{
    return obj && obj->guid == guid;
}

void MObject::operator delete(void* ptr)
{
    // The destructor has already run, so the object's members (name, guid)
    // must not be read here.
    // Objects from mnew<T>() go back to their type pool; anything else
    // (plain `new`, or mnew before the memory subsystem started) goes back to
    // the global heap.
    MMemory::releaseObject(ptr);
}
