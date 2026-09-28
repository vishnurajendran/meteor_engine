//
// Created by ssj5v on 31-05-2026.
//
#include <iostream>
#include <string>
#include <cstdlib>

#ifndef EDITOR_FILE_EXPLORERER_UTILS_H
#define EDITOR_FILE_EXPLORERER_UTILS_H

class MEditorUtility
{
public:
    static void openInFilExplorer(const std::string& path, bool isRelative=true);
    static void openUrl(const std::string& url);

    // Opens `workspaceDir` as a VS Code workspace and `filePath` inside it.
    // Requires `code` on PATH. Returns false if it could not be started.
    static bool openInVsCode(const std::string& filePath, const std::string& workspaceDir);
};


#endif //EDITOR_FILE_EXPLORERER_UTILS_H
