//
// Created by ssj5v on 05-06-2026.
//

#include "lua_script_asset.h"

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MLuaScriptAsset::MLuaScriptAsset(const SString& path) : MAsset(path)
{
    loadSourceFromFile();
}


void MLuaScriptAsset::loadSourceFromFile()
{
    valid = FileIO::readFile(path, scriptSrc);
    if (!valid)
    {
        MERROR("Failed to read file");
    }
}
