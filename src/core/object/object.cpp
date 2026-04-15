#include "object.h"
#include "core/utils/guid.h"
#include "core/object/gc.h"

MObject::MObject()
{
    guid = SGuid::newGUID();
    name = "MObject";
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