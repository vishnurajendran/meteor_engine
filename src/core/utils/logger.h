//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_LOGGER_H
#define METEOR_ENGINE_LOGGER_H
#include "spdlog/spdlog.h"
#include "sstring.h"

#define MLOG(...) MLogger::log(__VA_ARGS__)
#define MWARN(...) MLogger::warn(__VA_ARGS__)
#define MERROR(...) MLogger::error(__VA_ARGS__)

class MLogger {
public:
    static void log(SString msg);
    static void warn(SString warning);
    static void error(SString error);
};


#endif //METEOR_ENGINE_LOGGER_H
