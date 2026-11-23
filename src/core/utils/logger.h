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
#include "delegate.h"

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

class MLoggingSubsystem;

// Bootstrap state used while no MLoggingSubsystem is active. Defined in logger.cpp.
struct SLoggerBootstrap;

// Front end for logging. The work is done by MLoggingSubsystem
// (core/engine/logging/logging_subsystem.h) once it is registered.
//
// Before the subsystem is registered (static init, path setup, application
// construction) and after it is cleaned up, messages go straight to the
// console. Messages from before registration are also kept (up to 10000) and
// written to the log file when the subsystem starts.
class MLogger {
private:
    // Construct-on-first-use, so logging from static-init code is safe.
    static SLoggerBootstrap& bootstrap();

    static SLogMessage makeMessage(const char* tag, const SString& text, const SLogLocation& loc);
    static void dispatch(const SLogMessage& msg);
public:
    static void log(SString msg,      SLogLocation loc = {});
    static void verbose(SString msg,   SLogLocation loc = {});
    static void warn(SString warning,  SLogLocation loc = {});
    static void error(SString error,   SLogLocation loc = {});

    // Fired for every log/warn/error, on the thread that logged it - which
    // may be a worker thread (asset watcher, audio decoding, splash screen).
    // While the logging subsystem is active this is the subsystem's event;
    // subscribe after MEngineSubsystemRegistry::init().
    static TMulticastDelegate<const SLogMessage&>& onLog();

    static SString getLastMessage();

    // Called by MLoggingSubsystem::init() / cleanup(). Activating writes the
    // messages logged so far into the subsystem's log file.
    static void setActiveSubsystem(MLoggingSubsystem* subsystem);
};

#endif //METEOR_ENGINE_LOGGER_H