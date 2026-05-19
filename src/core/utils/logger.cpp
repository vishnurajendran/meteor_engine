//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "logger.h"
#include <sstream>

// ---- Platform-specific stack trace capture --------------------------------
//
// backtrace() on POSIX gives mangled symbol names. If you want readable
// output, add demangling via abi::__cxa_demangle() from <cxxabi.h>.
//
// On Windows, CaptureStackBackTrace returns raw addresses. Full symbol
// resolution requires linking dbghelp.lib and calling SymInitialize /
// SymFromAddr. The stub below gives frame addresses as a starting point.

#if defined(__linux__) || defined(__APPLE__)
#include <execinfo.h>

SString MLogger::captureStackTrace() {
    constexpr int MAX_FRAMES = 32;
    void* frames[MAX_FRAMES];
    int count = backtrace(frames, MAX_FRAMES);
    char** symbols = backtrace_symbols(frames, count);
    if (!symbols) return SString("");

    std::ostringstream oss;
    // Skip first 3 frames (captureStackTrace, error, notify internals)
    for (int i = 3; i < count; ++i) {
        oss << "  " << symbols[i] << "\n";
    }
    free(symbols);
    return SString(oss.str());
}

#elif defined(_WIN32)
#include <windows.h>

SString MLogger::captureStackTrace() {
    constexpr int MAX_FRAMES = 32;
    void* frames[MAX_FRAMES];
    USHORT count = CaptureStackBackTrace(3, MAX_FRAMES, frames, nullptr);

    std::ostringstream oss;
    for (USHORT i = 0; i < count; ++i) {
        oss << "  [" << i << "] 0x" << frames[i] << "\n";
    }
    return SString(oss.str());
}

#else

SString MLogger::captureStackTrace() {
    return SString("[stack trace unavailable on this platform]");
}

#endif

// ---- Construct-on-first-use statics --------------------------------------

std::map<int,LogEventHandler>& MLogger::getListeners() {
    static std::map<int,LogEventHandler> listeners;
    return listeners;
}

int& MLogger::getNextId() {
    static int nextId = 0;
    return nextId;
}

SString& MLogger::getLastMsg() {
    static SString lastMsg = "";
    return lastMsg;
}

// ---- Logging functions ---------------------------------------------------

void MLogger::log(SString msg, SLogLocation loc) {
    getLastMsg() = msg;
    spdlog::info(msg.str());

    SLogMessage lm;
    lm.tag       = STR("LOG");
    lm.text      = msg;
    lm.location  = loc;
    lm.timestamp = std::chrono::system_clock::now();
    notify(lm);
}

void MLogger::verbose(SString msg, SLogLocation loc) {
#ifdef VERBOSE_LOGGING
    log(msg, loc);
#endif
}

void MLogger::warn(SString warning, SLogLocation loc) {
    getLastMsg() = warning;
    spdlog::warn(warning.str());

    SLogMessage lm;
    lm.tag       = STR("WRN");
    lm.text      = warning;
    lm.location  = loc;
    lm.timestamp = std::chrono::system_clock::now();
    notify(lm);
}

void MLogger::error(SString error, SLogLocation loc) {
    getLastMsg() = error;
    spdlog::error(error.str());

    SLogMessage lm;
    lm.tag        = STR("ERR");
    lm.text       = error;
    lm.location   = loc;
    lm.stackTrace = captureStackTrace();
    lm.timestamp  = std::chrono::system_clock::now();
    notify(lm);
}

// ---- Subscription --------------------------------------------------------

int MLogger::subscribe(const LogEventHandler &handler) {
    auto id = getNextId()++;
    getListeners()[id] = handler;
    return id;
}

void MLogger::unsubscribe(const int& id) {
    getListeners().erase(id);
}

SString MLogger::getLastMessage() {
    return getLastMsg();
}

void MLogger::notify(const SLogMessage& msg) {
    for (const auto& [id, listener] : getListeners()) {
        if (listener)
            listener(msg);
    }
}