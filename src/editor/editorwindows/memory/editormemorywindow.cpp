#include "editormemorywindow.h"

#include <algorithm>
#include <cstdio>

#include "core/engine/subsystem/subsystem_registry.h"
#include "core/memory/memory_subsystem_interface.h"
#include "editor/editorwindows/registry/editor_subwindow_registry.h"

EDITOR_REGISTER_SUBWINDOW("Memory", "Window/Debug/Memory", MEditorMemoryWindow)

namespace
{
    // "512 B", "12.4 KB", "3.1 MB"
    std::string formatBytes(const size_t bytes)
    {
        char buf[32];
        if (bytes < 1024)
            snprintf(buf, sizeof(buf), "%zu B", bytes);
        else if (bytes < 1024 * 1024)
            snprintf(buf, sizeof(buf), "%.1f KB", static_cast<double>(bytes) / 1024.0);
        else
            snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
        return buf;
    }
}

MEditorMemoryWindow::MEditorMemoryWindow() : MEditorMemoryWindow(420, 400) {}

MEditorMemoryWindow::MEditorMemoryWindow(const int x, const int y) : MImGuiSubWindow(x, y)
{
    title = "Memory";
}

void MEditorMemoryWindow::refresh()
{
    rows.clear();

    auto* memory = MEngineSubsystemRegistry::getSubsystem<IMemoryManagementSubsystem>();
    if (!memory)
        return;

    const SMemoryStats stats = memory->getStats();
    rows.push_back({ "Object pool", stats.liveAllocations, stats.pagesInUse * stats.pageSize, true });

    for (const auto& e : memory->getExternalUsage())
        rows.push_back({ e.name, e.liveAllocations, e.liveBytes, true });

    // Per type, biggest first, live types only.
    std::vector<SMemoryPoolStats> pools = stats.pools;
    std::sort(pools.begin(), pools.end(), [](const auto& a, const auto& b)
    {
        return a.liveCount * a.slotSize > b.liveCount * b.slotSize;
    });

    for (const auto& pool : pools)
    {
        if (pool.liveCount > 0)
            rows.push_back({ pool.typeName, pool.liveCount, pool.liveCount * pool.slotSize, false });
    }
}

void MEditorMemoryWindow::onGui(const float deltaTime)
{
    timeSinceRefresh += deltaTime;
    if (timeSinceRefresh >= REFRESH_INTERVAL)
    {
        refresh();
        timeSinceRefresh = 0.0f;
    }

    constexpr ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                      ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("##memory", 3, flags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Name",  ImGuiTableColumnFlags_WidthStretch, 3.0f);
    ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableHeadersRow();

    bool inTypes = false;
    for (const auto& row : rows)
    {
        // Label row between the summary and the per-type rows.
        if (!row.header && !inTypes)
        {
            inTypes = true;
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Object types");
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (row.header) ImGui::TextUnformatted(row.name.c_str());
        else            ImGui::Text("  %s", row.name.c_str());

        ImGui::TableNextColumn();
        if (row.count > 0) ImGui::Text("%zu", row.count);

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(formatBytes(row.bytes).c_str());
    }

    ImGui::EndTable();
}
