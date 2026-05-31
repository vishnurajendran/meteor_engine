//
// Created by ssj5v on 31-05-2026.
//

#ifndef SIMPLE_PROFILER_H
#define SIMPLE_PROFILER_H
#include <chrono>
#include <map>
#include "core/object/object.h"

class MSimpleProfiler : public MObject {
public:
    MSimpleProfiler() = default;
    ~MSimpleProfiler() override = default;

    // API
    static void clear() { nameToExecutionTimeMap.clear(); }
    static void recordTimes(const SString& key, const float& value);
    static const std::map<SString, float>& getExecutionTimes() {
        return nameToExecutionTimeMap;
    }
private:
    static std::map<SString, float> nameToExecutionTimeMap;
};

// Using `#Key` automatically stringifies the argument.
// This allows you to write START_PROFILING_SAMPLE(Application.Loop) without quotes.
#define START_PROFILING_SAMPLE(Key) \
{ \
auto _profile_start_time = std::chrono::high_resolution_clock::now();

#define STOP_PROFILING_SAMPLE(Key) \
auto _profile_end_time = std::chrono::high_resolution_clock::now(); \
float _profile_duration_ms = std::chrono::duration<float, std::milli>(_profile_end_time - _profile_start_time).count(); \
MSimpleProfiler::recordTimes(Key, _profile_duration_ms); \
}

#endif //SIMPLE_PROFILER_H