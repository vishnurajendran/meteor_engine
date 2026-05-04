#include "object.h"
#include "core/utils/guid.h"
#include "core/object/gc.h"

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
    // Record object delete operation.
    auto inst = static_cast<MObject*>(ptr);
    if (inst != nullptr)
    {
        MVERBOSE(SString::format("Deleting MObject {0} ({1})", inst->getName(), inst->getGUID()));
    }

    ::operator delete(ptr);
}
