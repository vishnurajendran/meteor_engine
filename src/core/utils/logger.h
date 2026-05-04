//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_LOGGER_H
#define METEOR_ENGINE_LOGGER_H
#include "spdlog/spdlog.h"
#include "sstring.h"
#include "map"

#define MLOG(...) MLogger::log(__VA_ARGS__)
#define MWARN(...) MLogger::warn(__VA_ARGS__)
#define MERROR(...) MLogger::error(__VA_ARGS__)

#define MVERBOSE(...) MLogger::verbose(__VA_ARGS__)

using LogEventHandler = std::function<void(SString)>;
class MLogger {
private:
    // Construct-on-first-use accessors — avoids static initialization order
    // fiasco when MObject (or any other static-init code) logs before main().
    static std::map<int,LogEventHandler>& getListeners();
    static int& getNextId();
    static SString& getLastMsg();

    static void notify(SString msg);
public:
    static void log(SString msg);
    static void verbose(SString msg);
    static void warn(SString warning);
    static void error(SString error);

    static int subscribe(const LogEventHandler& handler);
    static void unsubscribe(const int& id);

    static SString getLastMessage();
};


#endif //METEOR_ENGINE_LOGGER_H