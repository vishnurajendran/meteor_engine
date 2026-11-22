//
// font_metadata.cpp
//

#include "font_metadata.h"

#include <string>

namespace
{
    uint16_t readU16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }
    uint32_t readU32(const uint8_t* p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16)
             | (static_cast<uint32_t>(p[2]) << 8)  |  static_cast<uint32_t>(p[3]);
    }

    struct STable
    {
        uint32_t offset = 0;
        uint32_t length = 0;
        bool     found  = false;
    };

    STable findTable(const uint8_t* data, size_t size, const char tag[4])
    {
        STable table;
        if (size < 12)
            return table;
        const uint16_t numTables = readU16(data + 4);
        for (uint16_t i = 0; i < numTables; ++i)
        {
            const size_t record = 12 + static_cast<size_t>(i) * 16;
            if (record + 16 > size)
                break;
            if (data[record] == tag[0] && data[record + 1] == tag[1] &&
                data[record + 2] == tag[2] && data[record + 3] == tag[3])
            {
                table.offset = readU32(data + record + 8);
                table.length = readU32(data + record + 12);
                table.found  = static_cast<size_t>(table.offset) + table.length <= size;
                return table;
            }
        }
        return table;
    }

    // UTF-16BE -> UTF-8 (surrogate pairs included).
    std::string utf16beToUtf8(const uint8_t* p, size_t bytes)
    {
        std::string out;
        for (size_t i = 0; i + 1 < bytes; i += 2)
        {
            uint32_t cp = readU16(p + i);
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < bytes)
            {
                const uint32_t low = readU16(p + i + 2);
                if (low >= 0xDC00 && low <= 0xDFFF)
                {
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    i += 2;
                }
            }
            if (cp < 0x80)
                out += static_cast<char>(cp);
            else if (cp < 0x800)
            {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000)
            {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        return out;
    }

    // Mac Roman, ASCII range only - anything else becomes '?'. Family names
    // in the Mac records are practically always ASCII.
    std::string macRomanToUtf8(const uint8_t* p, size_t bytes)
    {
        std::string out;
        for (size_t i = 0; i < bytes; ++i)
            out += p[i] < 0x80 ? static_cast<char>(p[i]) : '?';
        return out;
    }

    // Best record for a name id: Windows English (US) first, then any Windows
    // Unicode record, then Unicode platform, then Mac Roman. Empty if absent.
    std::string readName(const uint8_t* data, const STable& table, uint16_t nameId)
    {
        if (!table.found || table.length < 6)
            return {};
        const uint8_t* base = data + table.offset;
        const uint16_t count        = readU16(base + 2);
        const uint16_t stringOffset = readU16(base + 4);

        int bestScore = -1;
        std::string best;
        for (uint16_t i = 0; i < count; ++i)
        {
            const size_t record = 6 + static_cast<size_t>(i) * 12;
            if (record + 12 > table.length)
                break;
            const uint8_t* r = base + record;
            const uint16_t platform = readU16(r);
            const uint16_t encoding = readU16(r + 2);
            const uint16_t language = readU16(r + 4);
            const uint16_t id       = readU16(r + 6);
            const uint16_t length   = readU16(r + 8);
            const uint16_t offset   = readU16(r + 10);
            if (id != nameId)
                continue;
            if (static_cast<size_t>(stringOffset) + offset + length > table.length)
                continue;

            int score = -1;
            if (platform == 3 && (encoding == 1 || encoding == 10))
                score = language == 0x0409 ? 4 : 3;
            else if (platform == 0)
                score = 2;
            else if (platform == 1 && encoding == 0)
                score = 1;
            if (score <= bestScore)
                continue;

            const uint8_t* text = base + stringOffset + offset;
            best      = platform == 1 ? macRomanToUtf8(text, length) : utf16beToUtf8(text, length);
            bestScore = score;
        }
        return best;
    }
}

bool MFontMetadataReader::read(const uint8_t* data, size_t size, SFontMetadata& out)
{
    if (!data || size < 12)
        return false;

    // 0x00010000 TrueType, 'true' (old Apple TrueType), 'OTTO' CFF OpenType.
    const uint32_t version = readU32(data);
    if (version != 0x00010000 && version != 0x74727565 && version != 0x4F54544F)
        return false;

    SFontMetadata result;

    const STable name = findTable(data, size, "name");
    std::string family = readName(data, name, 16);
    if (family.empty())
        family = readName(data, name, 1);
    std::string style = readName(data, name, 17);
    if (style.empty())
        style = readName(data, name, 2);
    if (family.empty())
        return false;
    result.family    = SString(family);
    result.styleName = SString(style.empty() ? std::string("Regular") : style);

    const STable os2 = findTable(data, size, "OS/2");
    bool italicKnown = false;
    if (os2.found && os2.length >= 64)
    {
        const uint8_t* base = data + os2.offset;
        const uint16_t weightClass = readU16(base + 4);
        if (weightClass >= 1 && weightClass <= 1000)
            result.weight = weightClass < 100 ? 100 : (weightClass > 900 ? 900 : weightClass);
        const uint16_t fsSelection = readU16(base + 62);
        result.italic = (fsSelection & 0x0001) != 0 || (fsSelection & 0x0200) != 0;
        italicKnown = true;
    }

    const STable head = findTable(data, size, "head");
    if (!italicKnown && head.found && head.length >= 46)
    {
        const uint16_t macStyle = readU16(data + head.offset + 44);
        result.italic = (macStyle & 0x0002) != 0;
        if (!os2.found && (macStyle & 0x0001) != 0)
            result.weight = 700;
    }

    out = result;
    return true;
}
