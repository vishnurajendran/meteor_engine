//
// process_utils.cpp
//

#include "process_utils.h"

#include <filesystem>
#include <string>

#include "core/utils/logger.h"

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace fs = std::filesystem;

#if defined(_WIN32)
// Quote one argument per the CommandLineToArgvW rules.
static std::wstring quoteArg(const std::wstring& arg)
{
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos)
        return arg;

    std::wstring out = L"\"";
    size_t backslashes = 0;
    for (wchar_t c : arg)
    {
        if (c == L'\\') { ++backslashes; continue; }
        if (c == L'"') out.append(backslashes * 2 + 1, L'\\');
        else           out.append(backslashes, L'\\');
        backslashes = 0;
        out.push_back(c);
    }
    out.append(backslashes * 2, L'\\');
    out.push_back(L'"');
    return out;
}
#endif

bool SProcessUtils::launchDetached(const SString& executable,
                                   const std::vector<SString>& args,
                                   const SString& workingDirectory)
{
    const fs::path exe(executable.str());
    std::error_code ec;
    if (!fs::exists(exe, ec))
    {
        MWARN(SString("ProcessUtils:: executable not found: ") + executable);
        return false;
    }

#if defined(_WIN32)
    std::wstring cmdLine = quoteArg(exe.wstring());
    for (const auto& a : args)
        cmdLine += L" " + quoteArg(fs::path(a.str()).wstring());

    const std::wstring cwd = workingDirectory.empty() ? std::wstring()
                                                      : fs::path(workingDirectory.str()).wstring();

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    // CreateProcessW may modify the command-line buffer, so pass a copy.
    std::wstring mutableCmd = cmdLine;
    const BOOL ok = CreateProcessW(exe.wstring().c_str(), mutableCmd.data(), nullptr, nullptr, FALSE,
                                   DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr,
                                   cwd.empty() ? nullptr : cwd.c_str(), &si, &pi);
    if (!ok)
    {
        MERROR(SString::format("ProcessUtils:: failed to start {0} (error {1})",
                               executable, SString::fromInt((int)GetLastError())));
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
#else
    const pid_t pid = fork();
    if (pid < 0)
    {
        MERROR(SString("ProcessUtils:: fork failed for ") + executable);
        return false;
    }
    if (pid == 0)
    {
        setsid();   // detach from the parent's session
        if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0)
            _exit(127);

        std::vector<std::string> storage;
        storage.push_back(exe.string());
        for (const auto& a : args) storage.push_back(a.str());

        std::vector<char*> argv;
        for (auto& s : storage) argv.push_back(s.data());
        argv.push_back(nullptr);

        execv(storage[0].c_str(), argv.data());
        _exit(127);
    }
    return true;
#endif
}
