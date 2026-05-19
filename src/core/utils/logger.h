//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_LOGGER_H
#define METEOR_ENGINE_LOGGER_H
#include "core/object/object.h"
#include "map"
#include "spdlog/spdlog.h"
#include "sstring.h"
#include <chrono>
#include <functional>

// Source location captured at the macro call site.
// Has defaults so direct MLogger::log() calls still compile.
struct SLogLocation {
    const char* file     = "";
    int         line     = 0;
    const char* function = "";
};

// Structured log message delivered to subscribers
struct SLogMessage {
    SString      tag;         // "LOG", "WRN", "ERR"
    SString      text;
    SLogLocation location;
    SString      stackTrace;  // non-empty only for errors
    std::chrono::system_clock::time_point timestamp;
};

// Macros inject source location transparently.
// Call-site usage is unchanged: MLOG("something happened").
#define MLOG(msg) MLogger::log(msg, SLogLocation{__FILE__, __LINE__, __FUNCTION__})
#define MWARN(msg) MLogger::warn(msg, SLogLocation{__FILE__, __LINE__, __FUNCTION__})
#define MERROR(msg) MLogger::error(msg, SLogLocation{__FILE__, __LINE__, __FUNCTION__})
#define MVERBOSE(msg) MLogger::verbose(msg, SLogLocation{__FILE__, __LINE__, __FUNCTION__})

using LogEventHandler = std::function<void(const SLogMessage&)>;

class MLogger {
private:
    // Construct-on-first-use accessors to avoid static initialization order
    // issues when MObject (or any other static-init code) logs before main().
    static std::map<int,LogEventHandler>& getListeners();
    static int& getNextId();
    static SString& getLastMsg();

    static void notify(const SLogMessage& msg);
    static SString captureStackTrace();
public:
    static void log(SString msg,      SLogLocation loc = {});
    static void verbose(SString msg,   SLogLocation loc = {});
    static void warn(SString warning,  SLogLocation loc = {});
    static void error(SString error,   SLogLocation loc = {});

    static int subscribe(const LogEventHandler& handler);
    static void unsubscribe(const int& id);

    static SString getLastMessage();
};

#endif //METEOR_ENGINE_LOGGER_H