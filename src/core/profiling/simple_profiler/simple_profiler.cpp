//
// Created by ssj5v on 31-05-2026.
//

#include "simple_profiler.h"
std::map<SString, float> MSimpleProfiler::nameToExecutionTimeMap;

void MSimpleProfiler::recordTimes(const SString& key, const float& value)
{
    if (nameToExecutionTimeMap.contains(key))
        nameToExecutionTimeMap[key] = value;
    else
        nameToExecutionTimeMap.insert(std::make_pair(key, value));
}
