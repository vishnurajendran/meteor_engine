//
// Created by ssj5v on 31-05-2026.
//
#include "editor_utility.h"
#include <filesystem>
#include "core/utils/logger.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <shellapi.h>
#endif

void MEditorUtility::openInFilExplorer(const std::string& path, bool isRelative)
{
    MLOG(SString::format("EditorUtility:: Opening {0} in system file explorer", path));
    auto resolvedPath = path;
    if (isRelative)
    {
        auto workingDir = std::filesystem::current_path();
        workingDir = workingDir / path;
        resolvedPath = workingDir.string();
        MVERBOSE(SString::format("MEditorUtility::Resolved Path: {0}", resolvedPath));
    }

#if defined(_WIN32) || defined(_WIN64)
    // Windows: Convert string to wide string for Unicode path support
    std::wstring wpath(resolvedPath.begin(), resolvedPath.end());
    ShellExecuteW(NULL, L"open", wpath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif defined(__APPLE__)
    // macOS: Use the 'open' command
    std::string command = "open \"" + resolvedPath + "\"";
    std::system(command.c_str());
#elif defined(__linux__)
    // Linux: Use 'xdg-open' for the default file manager
    std::string command = "xdg-open \"" + resolvedPath + "\"";
    std::system(command.c_str());
#else
    std::cerr << "Unsupported operating system." << std::endl;
#endif
}

void MEditorUtility::openUrl(const std::string& url)
{
    MLOG(SString::format("EditorUtility:: Opening URL {0}", url));

#if defined(_WIN32) || defined(_WIN64)
    // Windows: ShellExecuteA handles URLs natively via the default browser
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // macOS: 'open' routes URLs to the default browser
    std::string command = "open \"" + url + "\"";
    std::system(command.c_str());
#elif defined(__linux__)
    // Linux: 'xdg-open' handles URLs via the default browser
    std::string command = "xdg-open \"" + url + "\"";
    std::system(command.c_str());
#else
    std::cerr << "Unsupported operating system." << std::endl;
#endif
}

// ---------------------------------------------------------------------------
// VS Code
// ---------------------------------------------------------------------------

bool MEditorUtility::openInVsCode(const std::string& filePath, const std::string& workspaceDir)
{
    MLOG(SString::format("EditorUtility:: Opening {0} in VS Code ({1})", filePath, workspaceDir));

    // Expects `code` on PATH. Passing the folder and the file together opens
    // the folder as the workspace and the file in it; an existing window for
    // that folder is reused.
#if defined(_WIN32) || defined(_WIN64)
    // `code` is code.cmd; ShellExecute resolves it through PATH/PATHEXT, and
    // SW_HIDE keeps its console window from flashing up.
    const std::string params = "\"" + workspaceDir + "\" \"" + filePath + "\"";
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteA(NULL, "open", "code", params.c_str(), workspaceDir.c_str(), SW_HIDE));
    if (result <= 32)
    {
        MWARN("EditorUtility:: failed to start VS Code - is `code` on PATH?");
        return false;
    }
    return true;
#else
    const std::string command = "code \"" + workspaceDir + "\" \"" + filePath + "\" &";
    if (std::system(command.c_str()) != 0)
    {
        MWARN("EditorUtility:: failed to start VS Code - is `code` on PATH?");
        return false;
    }
    return true;
#endif
}
