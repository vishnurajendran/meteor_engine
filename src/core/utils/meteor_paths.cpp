//
// meteor_paths.cpp
//

#include "meteor_paths.h"

#include <string>
#include <system_error>
#include <vector>

#include "core/utils/logger.h"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <climits>
#endif

namespace fs = std::filesystem;

namespace
{
fs::path& engineRootSlot()  { static fs::path p; return p; }
fs::path& projectRootSlot() { static fs::path p; return p; }
fs::path& projectFileSlot() { static fs::path p; return p; }

fs::path normalizeDir(const fs::path& p)
{
    std::error_code ec;
    fs::path abs = fs::absolute(p, ec);
    if (ec) abs = p;
    abs = abs.lexically_normal();
    // lexically_normal keeps a trailing separator for "dir/"; drop it.
    if (abs.has_filename() == false && abs.has_parent_path() && abs != abs.root_path())
        abs = abs.parent_path();
    return abs;
}

// Single *.mtproj in `dir`, or empty.
fs::path findProjectFileIn(const fs::path& dir)
{
    std::vector<fs::path> found;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (entry.is_regular_file(ec) && entry.path().extension() == SMeteorPaths::PROJECT_FILE_EXTENSION)
            found.push_back(entry.path());
    }
    if (found.size() == 1)
        return found.front();
    if (found.size() > 1)
        MWARN(SString::format("MeteorPaths:: {0} project files in {1}; pass one with --p",
                              SString::fromInt((int)found.size()), SString(dir.generic_string())));
    return {};
}
} // namespace

// ---------------------------------------------------------------------------

fs::path SMeteorPaths::findExecutableDir(const char* argv0)
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;)
    {
        const DWORD len = GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
        if (len == 0) break;
        if (len < buffer.size()) { buffer.resize(len); return fs::path(buffer).parent_path(); }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0)
        return fs::weakly_canonical(fs::path(buf)).parent_path();
#elif defined(__linux__)
    std::error_code ec;
    const fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return self.parent_path();
#endif
    if (argv0 && *argv0)
        return normalizeDir(fs::path(argv0).parent_path());
    return normalizeDir(fs::current_path());
}

bool SMeteorPaths::initialise(int argc, char** argv)
{
    engineRootSlot() = normalizeDir(findExecutableDir(argc > 0 ? argv[0] : nullptr));

    fs::path requestedProject;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if ((arg == "--p" || arg == "--project") && i + 1 < argc)
            requestedProject = argv[++i];
    }

    std::error_code ec;
    fs::path projectFilePath;
    if (!requestedProject.empty())
    {
        projectFilePath = fs::absolute(requestedProject, ec).lexically_normal();
        if (!fs::exists(projectFilePath, ec))
        {
            MERROR(SString("MeteorPaths:: project file not found: ") + SString(projectFilePath.generic_string()));
            projectRootSlot() = normalizeDir(fs::current_path());
            projectFileSlot().clear();
            return false;
        }
    }
    else
    {
        projectFilePath = findProjectFileIn(fs::current_path());
    }

    projectFileSlot() = projectFilePath;
    projectRootSlot() = projectFilePath.empty() ? normalizeDir(fs::current_path())
                                                : normalizeDir(projectFilePath.parent_path());

    // Existing code resolves project data relative to the working directory
    // (.engine_data/settings, assets/, caches). Keep that true.
    fs::current_path(projectRootSlot(), ec);
    if (ec)
        MERROR(SString("MeteorPaths:: cannot enter project folder: ") + SString(ec.message()));

    MLOG(SString::format("MeteorPaths:: engine  = {0}", SString(engineRootSlot().generic_string())));
    MLOG(SString::format("MeteorPaths:: project = {0}{1}", SString(projectRootSlot().generic_string()),
                         hasProjectFile() ? SString(" (" + projectFilePath.filename().string() + ")")
                                          : SString(" (no project file)")));
    return true;
}

void SMeteorPaths::internal_setRoots(const fs::path& engine, const fs::path& project,
                                     const fs::path& projectFilePath)
{
    engineRootSlot()  = normalizeDir(engine);
    projectRootSlot() = normalizeDir(project);
    projectFileSlot() = projectFilePath;
}

const fs::path& SMeteorPaths::engineRoot()
{
    if (engineRootSlot().empty())
        engineRootSlot() = normalizeDir(findExecutableDir(nullptr));
    return engineRootSlot();
}

const fs::path& SMeteorPaths::projectRoot()
{
    if (projectRootSlot().empty())
        projectRootSlot() = normalizeDir(fs::current_path());
    return projectRootSlot();
}

const fs::path& SMeteorPaths::projectFile()
{
    return projectFileSlot();
}

bool SMeteorPaths::isEngineProject()
{
    std::error_code ec;
    return fs::equivalent(engineRoot(), projectRoot(), ec);
}

SString SMeteorPaths::join(const fs::path& root, const SString& relative)
{
    if (relative.empty())
        return SString(root.generic_string());

    const fs::path rel(relative.str());
    if (rel.is_absolute())
        return SString(rel.lexically_normal().generic_string());
    return SString((root / rel).lexically_normal().generic_string());
}

SString ENGINE_PATH(const SString& relative)
{
    return SMeteorPaths::join(SMeteorPaths::engineRoot(), relative);
}

SString PROJECT_PATH(const SString& relative)
{
    return SMeteorPaths::join(SMeteorPaths::projectRoot(), relative);
}
