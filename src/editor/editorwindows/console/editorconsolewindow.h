//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORCONSOLEWINDOW_H
#define METEOR_ENGINE_EDITORCONSOLEWINDOW_H

#include <array>
#include "editor/meteorite_minimal.h"
#include "editor/window/imgui/imguisubwindow.h"

class MEditorConsoleWindow : public MImGuiSubWindow {
public:
    MEditorConsoleWindow();
    explicit MEditorConsoleWindow(int x, int y);
    ~MEditorConsoleWindow();
    void onGui() override;

private:
    // ── Log storage ───────────────────────────────────────────────────────────
    enum class ELogLevel { Log = 0, Warning = 1, Error = 2 };

    struct SLogEntry {
        SString   tag;    // "LOG" / "WRN" / "ERR"
        SString   text;   // message body after the tag prefix
        ELogLevel level;
    };

    std::vector<SLogEntry> entries;
    int                    selectedIndex = -1;   // index into entries; -1 = none
    std::array<int, 3>     levelCounts   = {};   // [Log, Warn, Err] for badge counts

    void onLogReceived(SString raw);
    int  subscriptionId = -1;

    // ── Filters ───────────────────────────────────────────────────────────────
    int  activeFilter = -1;       // -1 = All; 0/1/2 = ELogLevel cast to int
    char searchBuf[256] = {};

    // ── Display ───────────────────────────────────────────────────────────────
    bool autoScroll = true;

    // ── Helpers ───────────────────────────────────────────────────────────────
    static ImVec4      levelColor(ELogLevel level);
    static ImU32       levelBgColor(ELogLevel level);
    static const char* levelTag(ELogLevel level);
    bool               passesFilter(const SLogEntry& e) const;
    void               drawToolbar();
    void               drawLogList();
};

#endif //METEOR_ENGINE_EDITORCONSOLEWINDOW_H