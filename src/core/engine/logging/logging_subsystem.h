//
// logging_subsystem.h
//
// The engine's logger. Every message from MLOG / MWARN / MERROR (MLogger)
// goes to:
//   - the console
//   - a log file:
//       <project root>/logs/<YYYY-MM-DD_HH-MM-SS>_Editor.log   (editor build)
//       <project root>/logs/<YYYY-MM-DD_HH-MM-SS>_Player.log   (player build)
//     or <working directory>/logs/... when no project is open
//   - MLogger::onLog() listeners (e.g. the editor console)
//
// Registered first by MEngineSubsystemRegistry::init(), so it is cleaned up
// last. MLogger handles messages logged before init() and after cleanup();
// the ones from before init() are written to the top of the log file.
//
// Code should keep using MLogger / the macros rather than this class.
//

#ifndef METEOR_ENGINE_LOGGING_SUBSYSTEM_H
#define METEOR_ENGINE_LOGGING_SUBSYSTEM_H

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "core/utils/logger.h"
#include "spdlog/common.h"
#include "spdlog/sinks/sink.h"

class MLoggingSubsystem : public IEngineSubSystem
{
public:
    MLoggingSubsystem() = default;
    ~MLoggingSubsystem() override;

    void init() override;
    void cleanup() override;

    // Writes to the console and the log file, then notifies onLog() listeners.
    void write(const SLogMessage& msg);

    TMulticastDelegate<const SLogMessage&>& onLog() { return logEvent; }
    [[nodiscard]] SString getLastMessage() const;

    // Empty if the log file could not be opened.
    [[nodiscard]] const std::filesystem::path& getLogFilePath() const { return logFilePath; }

    // Shared with MLogger, which uses them before the subsystem exists.
    static SString captureStackTrace();
    static void writeToSink(spdlog::sinks::sink& sink, const SLogMessage& msg);

private:
    friend class MLogger;

    // Writes messages logged before init() to the log file (not the console,
    // which already showed them).
    void writeBacklog(const std::vector<SLogMessage>& messages);

    void openLogFile();

    static std::filesystem::path getLogDirectory();
    static std::string makeLogFileName();
    static const char* getLogFileSuffix();
    static spdlog::level::level_enum toSpdlogLevel(const SString& tag);

    spdlog::sink_ptr      consoleSink;
    spdlog::sink_ptr      fileSink;
    std::filesystem::path logFilePath;

    TMulticastDelegate<const SLogMessage&> logEvent;

    mutable std::mutex lastMessageMutex;
    SString            lastMessage;
};

#endif // METEOR_ENGINE_LOGGING_SUBSYSTEM_H
