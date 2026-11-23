#include "logging_subsystem.h"

#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <system_error>

#include "spdlog/details/log_msg.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "core/utils/meteor_paths.h"

// ---- Lifetime --------------------------------------------------------------

MLoggingSubsystem::~MLoggingSubsystem()
{
    // cleanup() normally ran already; this covers a subsystem deleted without it.
    if (consoleSink)
        cleanup();
}

void MLoggingSubsystem::init()
{
    if (consoleSink)
        return;

    consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    openLogFile();

    MLogger::setActiveSubsystem(this);

    if (fileSink)
        MLOG(SString("MLoggingSubsystem:: writing log to ") + SString(logFilePath.generic_string()));
}

void MLoggingSubsystem::cleanup()
{
    if (!consoleSink)
        return;

    MLOG("MLoggingSubsystem:: shutting down");
    // Later messages go to MLogger's console fallback.
    MLogger::setActiveSubsystem(nullptr);

    if (fileSink)
        fileSink->flush();
    fileSink.reset();
    consoleSink.reset();
}

void MLoggingSubsystem::openLogFile()
{
    // Not active yet, so MERROR here goes through MLogger's fallback.
    const std::filesystem::path logDir = getLogDirectory();
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);
    if (ec)
    {
        MERROR(SString("MLoggingSubsystem:: cannot create log folder ") + SString(logDir.generic_string()) +
               SString(": ") + SString(ec.message()));
        return;
    }

    const std::filesystem::path path = logDir / makeLogFileName();
    try
    {
        fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
        logFilePath = path;
    }
    catch (const spdlog::spdlog_ex& e)
    {
        MERROR(SString("MLoggingSubsystem:: cannot open log file: ") + SString(e.what()));
    }
}

// ---- Writing ---------------------------------------------------------------

void MLoggingSubsystem::write(const SLogMessage& msg)
{
    {
        std::lock_guard<std::mutex> lock(lastMessageMutex);
        lastMessage = msg.text;
    }

    if (consoleSink)
        writeToSink(*consoleSink, msg);
    if (fileSink)
    {
        writeToSink(*fileSink, msg);
        // Flush every message so the file is complete even after a crash.
        fileSink->flush();
    }

    logEvent.broadcast(msg);
}

void MLoggingSubsystem::writeBacklog(const std::vector<SLogMessage>& messages)
{
    if (!messages.empty())
    {
        std::lock_guard<std::mutex> lock(lastMessageMutex);
        lastMessage = messages.back().text;
    }

    if (!fileSink)
        return;
    for (const SLogMessage& msg : messages)
        writeToSink(*fileSink, msg);
    fileSink->flush();
}

void MLoggingSubsystem::writeToSink(spdlog::sinks::sink& sink, const SLogMessage& msg)
{
    const spdlog::source_loc where(msg.location.file, msg.location.line, msg.location.function);
    const spdlog::level::level_enum level = toSpdlogLevel(msg.tag);

    const std::string text = msg.text.str();
    sink.log(spdlog::details::log_msg(msg.timestamp, where, "", level, text));

    std::string stackTrace = msg.stackTrace.str();
    while (!stackTrace.empty() && (stackTrace.back() == '\n' || stackTrace.back() == '\r'))
        stackTrace.pop_back();
    if (!stackTrace.empty())
    {
        const std::string traceText = "Stack trace:\n" + stackTrace;
        sink.log(spdlog::details::log_msg(msg.timestamp, where, "", level, traceText));
    }
}

SString MLoggingSubsystem::getLastMessage() const
{
    std::lock_guard<std::mutex> lock(lastMessageMutex);
    return lastMessage;
}

spdlog::level::level_enum MLoggingSubsystem::toSpdlogLevel(const SString& tag)
{
    if (tag == "ERR") return spdlog::level::err;
    if (tag == "WRN") return spdlog::level::warn;
    return spdlog::level::info;
}

// ---- Log file naming -------------------------------------------------------

std::filesystem::path MLoggingSubsystem::getLogDirectory()
{
    if (SMeteorPaths::hasProjectFile())
        return SMeteorPaths::projectRoot() / "logs";

    std::error_code ec;
    return std::filesystem::current_path(ec) / "logs";
}

std::string MLoggingSubsystem::makeLogFileName()
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream oss;
    oss << std::put_time(&local, "%Y-%m-%d_%H-%M-%S") << "_" << getLogFileSuffix() << ".log";
    return oss.str();
}

const char* MLoggingSubsystem::getLogFileSuffix()
{
#if METEOR_EDITOR
    return "Editor";
#else
    return "Player";
#endif
}

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

SString MLoggingSubsystem::captureStackTrace() {
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

SString MLoggingSubsystem::captureStackTrace() {
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

SString MLoggingSubsystem::captureStackTrace() {
    return SString("[stack trace unavailable on this platform]");
}

#endif
