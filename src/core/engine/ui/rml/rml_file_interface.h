//
// rml_file_interface.h
//
// RmlUi reads every file (.rml, .rcss, <link>ed sheets, TGA images, fonts
// named in documents) through the active asset source: the project folders in
// the editor, the package in a published game. Paths are asset paths;
// "guid:<id>" references resolve through the asset manager.
//

#ifndef RML_FILE_INTERFACE_H
#define RML_FILE_INTERFACE_H

#include <RmlUi/Core/FileInterface.h>

#include "core/utils/sstring.h"

class MRmlFileInterface : public Rml::FileInterface
{
public:
    Rml::FileHandle Open(const Rml::String& path) override;
    void            Close(Rml::FileHandle file) override;
    size_t          Read(void* buffer, size_t size, Rml::FileHandle file) override;
    bool            Seek(Rml::FileHandle file, long offset, int origin) override;
    size_t          Tell(Rml::FileHandle file) override;
    size_t          Length(Rml::FileHandle file) override;

    // "guid:<id>" -> the asset's path; anything else normalised. Empty if a
    // GUID doesn't resolve.
    static SString resolvePath(const Rml::String& path);
};

#endif // RML_FILE_INTERFACE_H
