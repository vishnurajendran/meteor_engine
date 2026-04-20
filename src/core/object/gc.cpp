#include "gc.h"
#include "object.h"
#include "core/utils/logger.h"

std::unordered_map<MObject*, int> MGarbageCollector::refMap;

void MGarbageCollector::reference(MObject* obj)
{
    if (!obj) return;
    refMap[obj]++;
}

void MGarbageCollector::dereference(MObject* obj)
{
    if (!obj) return;

    auto it = refMap.find(obj);
    if (it == refMap.end()) return;

    if (--it->second <= 0)
    {
        MLOG(SString::format("GC: dereference {0}", obj->getName()));
        refMap.erase(it);
        delete obj;
    }
}