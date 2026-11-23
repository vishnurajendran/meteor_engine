//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "spdlog/sinks/stdout_color_sinks.h"

#include "core/engine/logging/logging_subsystem.h"

// ---- Bootstrap state -----------------------------------------------------
//
// Used until an MLoggingSubsystem is active, and again after it is cleaned up.

struct SLoggerBootstrap
{
    static constexpr size_t MAX_BACKLOG = 10000;

    std::atomic<MLoggingSubsystem*> activeSubsystem{ nullptr };

    std::mutex               mutex;              // guards everything below
    std::vector<SLogMessage> backlog;            // replayed into the log file
    bool                     keepBacklog = true; // only until the first subsystem starts
    SString                  lastMessage;

    TMulticastDelegate<const SLogMessage&> logEvent;
    spdlog::sink_ptr consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
};

SLoggerBootstrap& MLogger::bootstrap()
{
    // Never destroyed: code may log during static destruction.
    static SLoggerBootstrap* state = new SLoggerBootstrap();
    return *state;
}

// ---- Logging functions ---------------------------------------------------

SLogMessage MLogger::makeMessage(const char* tag, const SString& text, const SLogLocation& loc)
{
    SLogMessage lm;
    lm.tag       = SString(tag);
    lm.text      = text;
    lm.location  = loc;
    lm.timestamp = std::chrono::system_clock::now();
    return lm;
}

void MLogger::log(SString msg, SLogLocation loc) {
    dispatch(makeMessage("LOG", msg, loc));
}

void MLogger::verbose(SString msg, SLogLocation loc) {
#ifdef VERBOSE_LOGGING
    log(msg, loc);
#endif
}

void MLogger::warn(SString warning, SLogLocation loc) {
    dispatch(makeMessage("WRN", warning, loc));
}

void MLogger::error(SString error, SLogLocation loc) {
    SLogMessage lm = makeMessage("ERR", error, loc);
    lm.stackTrace  = MLoggingSubsystem::captureStackTrace();
    dispatch(lm);
}

void MLogger::dispatch(const SLogMessage& msg)
{
    SLoggerBootstrap& state = bootstrap();
    if (MLoggingSubsystem* subsystem = state.activeSubsystem.load())
    {
        subsystem->write(msg);
        return;
    }

    {
        std::unique_lock<std::mutex> lock(state.mutex);
        // The subsystem may have started while we waited for the lock.
        if (MLoggingSubsystem* subsystem = state.activeSubsystem.load())
        {
            lock.unlock();
            subsystem->write(msg);
            return;
        }
        state.lastMessage = msg.text;
        if (state.keepBacklog && state.backlog.size() < SLoggerBootstrap::MAX_BACKLOG)
            state.backlog.push_back(msg);
    }

    MLoggingSubsystem::writeToSink(*state.consoleSink, msg);
    state.logEvent.broadcast(msg);
}

// ---- Subsystem hookup ----------------------------------------------------

void MLogger::setActiveSubsystem(MLoggingSubsystem* subsystem)
{
    SLoggerBootstrap& state = bootstrap();
    std::lock_guard<std::mutex> lock(state.mutex);

    if (subsystem)
    {
        // Under the lock, so no message lands in the file ahead of the backlog.
        subsystem->writeBacklog(state.backlog);
        state.backlog.clear();
        state.backlog.shrink_to_fit();
        state.keepBacklog = false;
    }
    else if (MLoggingSubsystem* previous = state.activeSubsystem.load())
    {
        state.lastMessage = previous->getLastMessage();
    }

    state.activeSubsystem.store(subsystem);
}

// ---- Queries -------------------------------------------------------------

TMulticastDelegate<const SLogMessage&>& MLogger::onLog() {
    if (MLoggingSubsystem* subsystem = bootstrap().activeSubsystem.load())
        return subsystem->onLog();
    return bootstrap().logEvent;
}

SString MLogger::getLastMessage() {
    SLoggerBootstrap& state = bootstrap();
    if (MLoggingSubsystem* subsystem = state.activeSubsystem.load())
        return subsystem->getLastMessage();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.lastMessage;
}