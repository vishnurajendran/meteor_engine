//
// Created by ssj5v on 27-09-2024.
//

#include "asset.h"

#include <bits/fs_fwd.h>
#include <iostream>
#include <ostream>

#include "core/utils/fileio.h"
#include "core/utils/logger.h"

MAsset::MAsset(const SString& path) {
    name = SString::format("Asset_{0}", FileIO::getFileName(path));
    this->path = path;
}

SString MAsset::getPath() const { return path; }
SString MAsset::getFullPath() const
{
    auto workinDir = FileIO::getWorkingDir();
    return workinDir + "/" + path;
}

bool MAsset::isValid() const { return valid; }

void MAsset::internal_SetAssetId(const SString& assetId)
{
    this->assetId = assetId;
}
