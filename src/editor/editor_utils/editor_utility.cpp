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