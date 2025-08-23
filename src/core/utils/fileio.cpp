//
// Created by ssj5v on 27-09-2024.
//

#include "fileio.h"

#include <bits/fs_fwd.h>
#include <bits/fs_path.h>

bool FileIO::hasFile(const SString& path)
{
    std::ifstream ifs(path);
    return ifs.good();
}

bool FileIO::readFile(const SString& path, SString& data)
{
    std::ifstream ifs(path);
    if (ifs.is_open())
    {
        std::ostringstream oss;
        oss << ifs.rdbuf();
        data = oss.str();
        return true;
    }
    return false;
}
bool FileIO::writeFile(const SString& path, SString& data)
{
    std::ofstream ofs(path);
    if (ofs.is_open()) {
        ofs << data;
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
