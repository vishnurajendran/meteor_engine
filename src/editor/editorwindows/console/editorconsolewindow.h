//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORCONSOLEWINDOW_H
#define METEOR_ENGINE_EDITORCONSOLEWINDOW_H

#include <array>
#include "core/utils/logger.h"
#include "editor/editorwindows/editor_window_ids.h"
#include "editor/window/imgui/imguisubwindow.h"

class MEditorConsoleWindow : public MImGuiSubWindow {
public:
    unsigned int getWindowId() override { return EditorWindowIds::ID_CONSOLE_WINDOW; }

private:
    DEFINE_OBJECT_SUBCLASS(MEditorConsoleWindow)
public:
    MEditorConsoleWindow();
    explicit MEditorConsoleWindow(int x, int y);
    ~MEditorConsoleWindow();
    void onGui(float deltaTime) override;

private:
    // -- Log storage -------------------------------------------------------
    enum class ELogLevel { Log = 0, Warning = 1, Error = 2 };

    struct SLogEntry {
        SString      tag;          // "LOG" / "WRN" / "ERR"
        SString      text;         // message body
        ELogLevel    level;
        SString      shortFile;    // filename only, for inline display
        SString      fullFile;     // full path, shown in the detail pane
        int          line = 0;
        SString      function;
        SString      stackTrace;   // populated only for errors
        SString      timestamp;    // "HH:MM:SS.mmm"
    };

    std::vector<SLogEntry> entries;
    int                    selectedIndex = -1;   // index into entries; -1 = none
    std::array<int, 3>     levelCounts   = {};   // [Log, Warn, Err] for badge counts

    void onLogReceived(const SLogMessage& msg);
    int  subscriptionId = -1;

    // -- Filters -----------------------------------------------------------
    int  activeFilter = -1;       // -1 = All; 0/1/2 = ELogLevel cast to int
    char searchBuf[256] = {};

    // -- Display -----------------------------------------------------------
    bool autoScroll = true;

    // -- Helpers -----------------------------------------------------------
    static ImVec4      levelColor(ELogLevel level);
    static ImU32       levelBgColor(ELogLevel level);
    static const char* levelTag(ELogLevel level);
    static SString     shortenPath(const SString& fullPath);
    bool               passesFilter(const SLogEntry& e) const;
    void               drawToolbar();
    void               drawLogList();
    void               drawDetailPane();
};

#endif //METEOR_ENGINE_EDITORCONSOLEWINDOW_H