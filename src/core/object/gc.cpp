//
// Created by Vishnu Rajendran on 2024-09-18.
//

#include "gc.h"
#include "object.h"
#include "core/utils/logger.h"

std::map<SString, int> MGarbageCollector::objReferenceMap;

void MGarbageCollector::reference(MObject* obj) {
    if(obj == nullptr)
        return;

    auto guid = obj->getGUID();
    if(!objReferenceMap.contains(guid)){
        objReferenceMap[guid] = 1;
        return;
    }

    auto cnt = objReferenceMap[guid] + 1;
    objReferenceMap[guid] = cnt;
}

void MGarbageCollector::dereference(MObject* obj) {
    if(obj == nullptr) {
        return;
    }

    auto guid = obj->getGUID();
    //MLOG(TEXT("De-Referencing...") + obj->toString());
    if(!objReferenceMap.contains(guid)) {
        return;
    }

    auto cnt = objReferenceMap[guid] - 1;
    if(cnt <= 0){
        //MLOG(TEXT("Killing instance"));
        delete obj;
        //MLOG(TEXT("Instance killed"));
        objReferenceMap.erase(guid);
        return;
    }
    objReferenceMap[guid] = cnt;
}