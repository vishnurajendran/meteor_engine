//
// package_format.h
//
// On-disk layout of a published Meteor game. Shared by the editor (writes
// it, see editor/publishing) and the player (reads it, see
// packaged_asset_manager.h), so the two sides cannot drift apart.
//
// A published game folder holds:
//
//   .manifest        everything the player needs to know, in one file
//   00.pak, 01.pak   asset bytes
//
// All numbers are little-endian. Strings are a u32 byte length followed by
// UTF-8 bytes (no terminator).
//
// .manifest
// ---------
//   Header   magic "MTMF" | formatVersion u32 | buildId str | pakCount u32 | assetCount u32
//   Sections, repeated to end of file:
//            tag u32 | size u32 | <size bytes>
//
//     INFO    name, id, version, publisher, engineVersion       (str x5)
//     STRT    startup scene guid, startup scene path            (str x2)
//     SETT    physics layers xml, engine settings xml           (str x2)
//             Project settings baked into the build (physics, rendering).
//             Players cannot change them.
//     CORE    count u32, then per core asset (sorted by name):
//               name str | pakId u32 | offset u64 | size u64
//             Engine files looked up by name, with no meta and no GUID
//             (e.g. "shader_utils/meteor_vertex_utils.glsl").
//     ASST    count u32, then per asset (sorted by guid):
//               guid str | path str | pakId u32 | offset u64 | metaSize u32 | dataSize u64
//     PATH    count u32, then per asset (sorted by path):
//               path str | assetIndex u32   (index into ASST)
//
//   Unknown section tags are skipped, so new sections can be added without
//   breaking older players. Changing an existing section's layout needs a
//   FORMAT_VERSION bump.
//
// NN.pak
// ------
//   Header   magic "MTPK" | formatVersion u32 | buildId str | pakIndex u32
//   Entries  [meta bytes][data bytes], back to back. Offsets in ASST are
//            absolute file offsets of the meta bytes; data follows at
//            offset + metaSize. Core assets are [data bytes] only.
//
// The build ID is generated on every publish and written to the manifest
// and every pak. The player refuses to mount if any of them disagree.
//

#ifndef PACKAGE_FORMAT_H
#define PACKAGE_FORMAT_H

#include <cstdint>
#include <cstring>
#include <vector>

#include "core/utils/sstring.h"

constexpr uint32_t FORMAT_VERSION = 1;

constexpr const char* MANIFEST_FILE_NAME = ".manifest";
constexpr const char* PAK_EXTENSION      = ".pak";

constexpr uint32_t fourCC(char a, char b, char c, char d)
{
    return  static_cast<uint32_t>(static_cast<uint8_t>(a))
         | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8)
         | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16)
         | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

constexpr uint32_t MANIFEST_MAGIC = fourCC('M', 'T', 'M', 'F');
constexpr uint32_t PAK_MAGIC      = fourCC('M', 'T', 'P', 'K');

enum ESection : uint32_t
{
    SECTION_INFO     = fourCC('I', 'N', 'F', 'O'),
    SECTION_START    = fourCC('S', 'T', 'R', 'T'),
    SECTION_SETTINGS = fourCC('S', 'E', 'T', 'T'),
    SECTION_CORE     = fourCC('C', 'O', 'R', 'E'),
    SECTION_ASSETS   = fourCC('A', 'S', 'S', 'T'),
    SECTION_PATHS    = fourCC('P', 'A', 'T', 'H'),
};

// "00.pak", "01.pak", ... "100.pak"
inline SString pakFileName(uint32_t index)
{
    return SString::fromUInt(index).padLeft(2, '0') + PAK_EXTENSION;
}

// -----------------------------------------------------------------------
// Data
// -----------------------------------------------------------------------

struct SGameInfo
{
    SString name;
    SString id;
    SString version;
    SString publisher;
    SString engineVersion;
};

struct SStartupScene
{
    SString guid;
    SString path;
};

// Project settings baked into the build. Read-only for players.
struct SEmbeddedSettings
{
    SString physicsLayersXml;   // MPhysicsLayersSettings document
    SString engineSettingsXml;  // engine settings document (rendering, physics); window
                                // settings in it are ignored, those come from PlayerSettings.xml
};

// -----------------------------------------------------------------------
// Byte buffer helpers
// -----------------------------------------------------------------------

class SByteWriter
{
public:
    void u32(uint32_t v) { for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>(v >> (8 * i))); }
    void u64(uint64_t v) { for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<uint8_t>(v >> (8 * i))); }

    void str(const SString& s)
    {
        u32(static_cast<uint32_t>(s.size()));
        bytes.insert(bytes.end(), s.begin(), s.end());
    }

    void raw(const void* data, size_t size)
    {
        const auto* p = static_cast<const uint8_t*>(data);
        bytes.insert(bytes.end(), p, p + size);
    }

    // Appends a tagged section: tag | size | body.
    void section(uint32_t tag, const SByteWriter& body)
    {
        u32(tag);
        u32(static_cast<uint32_t>(body.bytes.size()));
        raw(body.bytes.data(), body.bytes.size());
    }

    std::vector<uint8_t> bytes;
};

// Bounds-checked reader. Every read fails (returns false) once the data
// runs out; callers check ok() or the return values.
class SByteReader
{
public:
    SByteReader(const uint8_t* inData, size_t inSize) : data(inData), size(inSize) {}

    bool u32(uint32_t& out)
    {
        if (!has(4)) return fail();
        out = 0;
        for (int i = 0; i < 4; ++i) out |= static_cast<uint32_t>(data[pos + i]) << (8 * i);
        pos += 4;
        return true;
    }

    bool u64(uint64_t& out)
    {
        if (!has(8)) return fail();
        out = 0;
        for (int i = 0; i < 8; ++i) out |= static_cast<uint64_t>(data[pos + i]) << (8 * i);
        pos += 8;
        return true;
    }

    bool str(SString& out)
    {
        uint32_t len = 0;
        if (!u32(len) || !has(len)) return fail();
        out = SString(reinterpret_cast<const char*>(data + pos), len);
        pos += len;
        return true;
    }

    bool skip(size_t n)
    {
        if (!has(n)) return fail();
        pos += n;
        return true;
    }

    [[nodiscard]] bool   ok()        const { return good; }
    [[nodiscard]] bool   atEnd()     const { return pos >= size; }
    [[nodiscard]] size_t position()  const { return pos; }
    [[nodiscard]] size_t remaining() const { return size - pos; }
    [[nodiscard]] const uint8_t* cursor() const { return data + pos; }

private:
    bool has(size_t n) const { return good && n <= size - pos; }
    bool fail() { good = false; return false; }

    const uint8_t* data = nullptr;
    size_t         size = 0;
    size_t         pos  = 0;
    bool           good = true;
};

#endif // PACKAGE_FORMAT_H
