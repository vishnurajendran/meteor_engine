//
// Created by ssj5v on 27-09-2024.
//

#pragma once
#ifndef FILEIO_H
#define FILEIO_H

#include <cstdint>
#include <fstream>
#include <sstream>

#include "sstring.h"
#include "core/object/object.h"

class FileIO : public MObject {
    DEFINE_OBJECT_SUBCLASS(FileIO);
public:
    // -- Existing API ---------------------------------------------------------

    static bool hasFile(const SString& path);
    static bool readFile(const SString& path, SString& data);
    static bool writeFile(const SString& path, const SString& data);
    static SString getFileExtension(const SString& path);
    static SString getWorkingDir();

    // -- Path helpers ---------------------------------------------------------

    // "assets/audio/test.mp3"  ->  "test.mp3"
    static SString getFileName(const SString& path);

    // "assets/audio/test.mp3"  ->  "test"
    static SString getFileNameWithoutExtension(const SString& path);

    // "assets/audio/test.mp3"  ->  "assets/audio"
    // Returns empty string for a bare filename with no directory.
    static SString getParentDirectory(const SString& path);

    // "assets/audio/test.mp3", "meta"  ->  "assets/audio/test.meta"
    // Replaces everything after the last dot. If no dot exists, appends one.
    static SString changeExtension(const SString& path, const SString& newExt);

    // Joins two path segments with a forward slash.
    // Avoids double separators if either side already has one.
    static SString joinPath(const SString& base, const SString& relative);

    // Replaces backslashes with forward slashes and collapses
    // consecutive separators. Does not resolve ".." or symlinks.
    static SString normalizePath(const SString& path);

    // -- File operations ------------------------------------------------------

    // Returns file size in bytes, or -1 on failure.
    static int64_t getFileSize(const SString& path);

    static bool deleteFile(const SString& path);
    static bool copyFile(const SString& src, const SString& dst);

    // Creates the directory and any missing parents -- like mkdir -p.
    static bool createDirectory(const SString& path);
};

#endif //FILEIO_H