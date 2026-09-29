//
// project_descriptor.h
//
// The <name>.mtproj file at the root of every project.
//
// Written by the launcher when it creates a project; read by the editor
// (and later the player / packager). Format:
//
//   <?xml version="1.0"?>
//   <project formatVersion="1" name="MyGame" id="{guid}" engineVersion="">
//     <assetFolders>
//       <folder path="assets/"/>
//     </assetFolders>
//     <startupScene path="assets/scenes/main.scml"/>
//   </project>
//
// Only `name` is required. Missing values fall back to the defaults below,
// so a launcher can start with a minimal file and the engine fills in the
// rest on save.
//

#ifndef PROJECT_DESCRIPTOR_H
#define PROJECT_DESCRIPTOR_H

#include <filesystem>
#include <vector>

#include "core/utils/sstring.h"

struct SProjectDescriptor
{
    static constexpr int CURRENT_FORMAT_VERSION = 1;

    int                  formatVersion = CURRENT_FORMAT_VERSION;
    SString              name;
    SString              id;             // GUID, stable across renames/moves
    SString              engineVersion;  // informational for now
    std::vector<SString> assetFolders = { "assets/" };   // relative to the project root
    SString              startupScene;   // asset path, optional

    // Load from / save to a .mtproj file. load() leaves defaults for anything
    // missing and returns false only if the file is unreadable or not a
    // <project> document.
    bool load(const std::filesystem::path& file);
    bool save(const std::filesystem::path& file) const;

    // Descriptor for a folder opened without a .mtproj (name = folder name).
    static SProjectDescriptor makeDefault(const std::filesystem::path& projectRoot);
};

#endif // PROJECT_DESCRIPTOR_H
