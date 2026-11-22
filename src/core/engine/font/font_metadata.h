//
// font_metadata.h
//
// Reads a font file's family, weight and style straight from its tables, so
// fonts can be described without FreeType (the player and core use it too).
//
//   family    'name' table: typographic family (name id 16), else family (1)
//   style     'name' table: typographic subfamily (17), else subfamily (2)
//   weight    'OS/2' usWeightClass (100-900)
//   italic    'OS/2' fsSelection bit 0 (italic) or bit 9 (oblique),
//             else 'head' macStyle bit 1
//
// TrueType (.ttf) and CFF OpenType (.otf). Collections (.ttc) are not read.
//

#ifndef FONT_METADATA_H
#define FONT_METADATA_H

#include <cstddef>
#include <cstdint>

#include "core/utils/sstring.h"

struct SFontMetadata
{
    SString family;            // "Open Sans"
    SString styleName;         // "Bold Italic"
    int     weight = 400;      // 100-900
    bool    italic = false;
};

class MFontMetadataReader
{
public:
    // False if the data isn't a font this reader understands; `out` then
    // keeps whatever it held.
    static bool read(const uint8_t* data, size_t size, SFontMetadata& out);
};

#endif // FONT_METADATA_H
