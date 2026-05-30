//
// Created by ssj5v on 27-09-2024.
//

#include "fileio.h"

#include <filesystem>

// -- Existing API -------------------------------------------------------------

bool FileIO::hasFile(const SString& path)
{
    std::ifstream ifs(path.c_str());
    return ifs.good();
}

bool FileIO::readFile(const SString& path, SString& data)
{
    std::ifstream ifs(path.c_str());
    if (ifs.is_open())
    {
        std::ostringstream oss;
        oss << ifs.rdbuf();
        data = oss.str();
        return true;
    }
    return false;
}

bool FileIO::writeFile(const SString& path, const SString& data)
{
    std::ofstream ofs(path.c_str());
    if (ofs.is_open())
    {
        ofs << data.c_str();
        return true;
    }
    return false;
}

SString FileIO::getFileExtension(const SString& path)
{
    std::string str = path;
    size_t dotPos = str.find_last_of('.');

    if (dotPos == std::string::npos || dotPos == 0 || dotPos == path.length() - 1)
    {
        return "";
    }

    return str.substr(dotPos + 1);
}

SString FileIO::getWorkingDir()
{
    return SString(std::filesystem::current_path().string());
}

// -- Path helpers -------------------------------------------------------------

SString FileIO::getFileName(const SString& path)
{
    std::string str = path;

    // Find the last separator -- either forward or back slash.
    size_t pos = str.find_last_of("/\\");
    if (pos == std::string::npos)
        return path;   // no directory component -- the whole thing is the name

    return str.substr(pos + 1);
}

SString FileIO::getFileNameWithoutExtension(const SString& path)
{
    SString name = getFileName(path);
    std::string str = name;

    size_t dotPos = str.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == 0)
        return name;

    return str.substr(0, dotPos);
}

SString FileIO::getParentDirectory(const SString& path)
{
    std::string str = path;

    size_t pos = str.find_last_of("/\\");
    if (pos == std::string::npos)
        return "";   // bare filename -- no parent

    return str.substr(0, pos);
}

SString FileIO::changeExtension(const SString& path, const SString& newExt)
{
    std::string str = path;

    size_t dotPos = str.find_last_of('.');
    size_t sepPos = str.find_last_of("/\\");

    // Only consider dots that come after the last directory separator
    // so that "path.d/filename" doesn't get its directory mangled.
    if (dotPos != std::string::npos && (sepPos == std::string::npos || dotPos > sepPos))
        str = str.substr(0, dotPos);

    return SString(str + "." + std::string(newExt));
}

SString FileIO::joinPath(const SString& base, const SString& relative)
{
    if (base.empty()) return relative;
    if (relative.empty()) return base;

    std::string b = base;
    std::string r = relative;

    // Strip trailing separator from base.
    bool baseEnds = (b.back() == '/' || b.back() == '\\');
    // Strip leading separator from relative.
    bool relStarts = (r.front() == '/' || r.front() == '\\');

    if (baseEnds && relStarts)
        return SString(b + r.substr(1));
    if (!baseEnds && !relStarts)
        return SString(b + "/" + r);

    return SString(b + r);
}

SString FileIO::normalizePath(const SString& path)
{
    std::string str = path;

    // Replace backslashes with forward slashes.
    for (char& c : str)
    {
        if (c == '\\')
            c = '/';
    }

    // Collapse consecutive slashes.
    std::string result;
    result.reserve(str.size());
    bool prevSlash = false;
    for (char c : str)
    {
        if (c == '/')
        {
            if (!prevSlash)
                result += c;
            prevSlash = true;
        }
        else
        {
            result += c;
            prevSlash = false;
        }
    }

    return SString(result);
}

// -- File operations ----------------------------------------------------------

int64_t FileIO::getFileSize(const SString& path)
{
    std::error_code ec;
    auto size = std::filesystem::file_size(std::filesystem::path(std::string(path)), ec);
    if (ec)
        return -1;
    return static_cast<int64_t>(size);
}

bool FileIO::deleteFile(const SString& path)
{
    std::error_code ec;
    return std::filesystem::remove(std::filesystem::path(std::string(path)), ec);
}

bool FileIO::copyFile(const SString& src, const SString& dst)
{
    std::error_code ec;
    std::filesystem::copy_file(std::filesystem::path(std::string(src)), std::filesystem::path(std::string(dst)),
                               std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

bool FileIO::directoryExists(const SString& path)
{
    return std::filesystem::is_directory(std::filesystem::path(std::string(path)));
}

bool FileIO::createDirectory(const SString& path)
{
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(std::string(path)), ec);
    return !ec;
}