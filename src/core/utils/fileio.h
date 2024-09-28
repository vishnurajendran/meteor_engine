//
// Created by ssj5v on 27-09-2024.
//

#pragma once
#ifndef FILEIO_H
#define FILEIO_H

#include <fstream>
#include <sstream>

#include "sstring.h"
#include "core/object/object.h"

class FileIO : MObject {
    public:
    static bool readFile(const SString& path, SString& data);
    static SString getFileExtension(const SString& path);
};

#endif //FILEIO_H
