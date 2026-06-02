//
// Created by ssj5v on 12-10-2024.
//

#include "textureasset.h"

#include "GL/glew.h"
#include "SFML/Graphics/Image.hpp"
#include "core/utils/fileio.h"
#include "core/utils/logger.h"
#include "pugixml.hpp"
#include <algorithm>
#include <sstream>

// -- String <-> enum helpers (file-local) ----------------------------------------

static const char* filterMinToString(ETextureFilterMin f)
{
    switch (f) {
        case ETextureFilterMin::Nearest:              return "nearest";
        case ETextureFilterMin::Linear:               return "linear";
        case ETextureFilterMin::NearestMipmapNearest: return "nearest_mipmap_nearest";
        case ETextureFilterMin::LinearMipmapNearest:  return "linear_mipmap_nearest";
        case ETextureFilterMin::NearestMipmapLinear:  return "nearest_mipmap_linear";
        case ETextureFilterMin::LinearMipmapLinear:   return "linear_mipmap_linear";
    }
    return "linear";
}

static ETextureFilterMin stringToFilterMin(const char* s)
{
    SString str(s);
    if (str == "nearest")                return ETextureFilterMin::Nearest;
    if (str == "nearest_mipmap_nearest") return ETextureFilterMin::NearestMipmapNearest;
    if (str == "linear_mipmap_nearest")  return ETextureFilterMin::LinearMipmapNearest;
    if (str == "nearest_mipmap_linear")  return ETextureFilterMin::NearestMipmapLinear;
    if (str == "linear_mipmap_linear")   return ETextureFilterMin::LinearMipmapLinear;
    return ETextureFilterMin::Linear;
}

static const char* filterMagToString(ETextureFilterMag f)
{
    switch (f) {
        case ETextureFilterMag::Nearest: return "nearest";
        case ETextureFilterMag::Linear:  return "linear";
    }
    return "linear";
}

static ETextureFilterMag stringToFilterMag(const char* s)
{
    if (SString(s) == "nearest") return ETextureFilterMag::Nearest;
    return ETextureFilterMag::Linear;
}

static const char* wrapToString(ETextureWrap w)
{
    switch (w) {
        case ETextureWrap::Repeat:         return "repeat";
        case ETextureWrap::MirroredRepeat: return "mirrored_repeat";
        case ETextureWrap::ClampToEdge:    return "clamp_to_edge";
        case ETextureWrap::ClampToBorder:  return "clamp_to_border";
    }
    return "repeat";
}

static ETextureWrap stringToWrap(const char* s)
{
    SString str(s);
    if (str == "mirrored_repeat") return ETextureWrap::MirroredRepeat;
    if (str == "clamp_to_edge")   return ETextureWrap::ClampToEdge;
    if (str == "clamp_to_border") return ETextureWrap::ClampToBorder;
    return ETextureWrap::Repeat;
}

static const char* compressionToString(ETextureCompression c)
{
    switch (c) {
        case ETextureCompression::None: return "none";
        case ETextureCompression::DXT1: return "dxt1";
        case ETextureCompression::DXT5: return "dxt5";
    }
    return "none";
}

static ETextureCompression stringToCompression(const char* s)
{
    SString str(s);
    if (str == "dxt1") return ETextureCompression::DXT1;
    if (str == "dxt5") return ETextureCompression::DXT5;
    return ETextureCompression::None;
}

// -- GL enum helpers -----------------------------------------------------------

static GLenum toGLFilterMin(ETextureFilterMin f)
{
    switch (f) {
        case ETextureFilterMin::Nearest:              return GL_NEAREST;
        case ETextureFilterMin::Linear:               return GL_LINEAR;
        case ETextureFilterMin::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        case ETextureFilterMin::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
        case ETextureFilterMin::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
        case ETextureFilterMin::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

static GLenum toGLFilterMag(ETextureFilterMag f)
{
    switch (f) {
        case ETextureFilterMag::Nearest: return GL_NEAREST;
        case ETextureFilterMag::Linear:  return GL_LINEAR;
    }
    return GL_LINEAR;
}

static GLenum toGLWrap(ETextureWrap w)
{
    switch (w) {
        case ETextureWrap::Repeat:         return GL_REPEAT;
        case ETextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case ETextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case ETextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
    }
    return GL_REPEAT;
}

static bool needsMipmaps(ETextureFilterMin f)
{
    return f == ETextureFilterMin::NearestMipmapNearest
        || f == ETextureFilterMin::LinearMipmapNearest
        || f == ETextureFilterMin::NearestMipmapLinear
        || f == ETextureFilterMin::LinearMipmapLinear;
}

// -- Bilinear resize helper ----------------------------------------------------
// CPU-side bilinear interpolation.  Runs once at import time.
// For a 4096→1024 downscale this is ~1 M pixel iterations - sub-millisecond.

static sf::Image resizeImage(const sf::Image& src, unsigned int newW, unsigned int newH)
{
    sf::Image dst({newW, newH});

    const auto srcSize = src.getSize();
    const float scaleX = (float)srcSize.x / (float)newW;
    const float scaleY = (float)srcSize.y / (float)newH;

    for (unsigned int y = 0; y < newH; ++y)
    {
        for (unsigned int x = 0; x < newW; ++x)
        {
            float fx = (x + 0.5f) * scaleX - 0.5f;
            float fy = (y + 0.5f) * scaleY - 0.5f;

            unsigned int x0 = std::max(0, (int)fx);
            unsigned int y0 = std::max(0, (int)fy);
            unsigned int x1 = std::min((int)x0 + 1, (int)srcSize.x - 1);
            unsigned int y1 = std::min((int)y0 + 1, (int)srcSize.y - 1);

            float dx = fx - (float)x0;
            float dy = fy - (float)y0;

            auto c00 = src.getPixel({x0, y0});
            auto c10 = src.getPixel({x1, y0});
            auto c01 = src.getPixel({x0, y1});
            auto c11 = src.getPixel({x1, y1});

            auto lerp = [](uint8_t a, uint8_t b, float t) -> uint8_t {
                return static_cast<uint8_t>(a + (b - a) * t);
            };

            sf::Color result(
                lerp(lerp(c00.r, c10.r, dx), lerp(c01.r, c11.r, dx), dy),
                lerp(lerp(c00.g, c10.g, dx), lerp(c01.g, c11.g, dx), dy),
                lerp(lerp(c00.b, c10.b, dx), lerp(c01.b, c11.b, dx), dy),
                lerp(lerp(c00.a, c10.a, dx), lerp(c01.a, c11.a, dx), dy)
            );

            dst.setPixel({ x, y} , result);
        }
    }
    return dst;
}

// -- Constructors --------------------------------------------------------------

MTextureAsset::MTextureAsset() : MAsset("") {}

MTextureAsset::MTextureAsset(const SString& path) : MAsset(path.c_str())
{
    valid = loadWithSettings();
}

MTextureAsset::MTextureAsset(const SString& path, const pugi::xml_node& importSettings)
    : MAsset(path.c_str())
{
    loadImportSettings(importSettings);
    valid = loadWithSettings();
}

MTexture* MTextureAsset::getTexture() { return &texture; }

bool MTextureAsset::requestReload()
{
    valid = loadWithSettings();
    return valid;
}

// -- Core load path ------------------------------------------------------------

bool MTextureAsset::loadWithSettings()
{
    bool needsProcessing = (maxImportSize > 0)
                        || (compression != ETextureCompression::None);

    if (!needsProcessing)
    {
        // Fast path - load directly via SFML, then apply GL params
        if (!texture.loadFromPath(path))
            return false;

        applyGLParams();
        return true;
    }

    // Slow path - load to CPU (sf::Image) so we can resize / compress
    sf::Image img;
    if (!img.loadFromFile(path.c_str()))
    {
        MERROR("MTextureAsset: failed to load image file " + path);
        return false;
    }

    // -- Resize ------------------------------------------------------------
    if (maxImportSize > 0)
    {
        auto sz = img.getSize();
        if (sz.x > (unsigned int)maxImportSize || sz.y > (unsigned int)maxImportSize)
        {
            float scale = (float)maxImportSize / (float)std::max(sz.x, sz.y);
            unsigned int newW = std::max(1u, (unsigned int)(sz.x * scale));
            unsigned int newH = std::max(1u, (unsigned int)(sz.y * scale));
            img = resizeImage(img, newW, newH);
        }
    }

    // Upload to sf::Texture (GPU as RGBA8)
    if (!texture.getCoreTexture()->loadFromImage(img))
    {
        MERROR("MTextureAsset: failed to create texture from image " + path);
        return false;
    }

    // -- Compression -------------------------------------------------------
    // Re-upload the same pixel data with a compressed internal format on the
    // same GL handle.  The driver compresses on the fly.
    //
    // CAVEAT: This modifies the GL texture behind SFML's back.  sf::Texture
    //         still thinks the format is RGBA8.  Calling copyToImage() on a
    //         compressed texture would produce incorrect results.  Nothing in
    //         the current codebase does that for regular textures, but this
    //         is worth knowing for future work.
    if (compression != ETextureCompression::None)
    {
        auto sz = img.getSize();
        GLenum internalFmt = GL_RGBA8;
        if (compression == ETextureCompression::DXT1)
            internalFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        else if (compression == ETextureCompression::DXT5)
            internalFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

        glBindTexture(GL_TEXTURE_2D, texture.getTextureID());
        glTexImage2D(GL_TEXTURE_2D, 0, internalFmt,
                     sz.x, sz.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    applyGLParams();
    return true;
}

// -- GL parameter application --------------------------------------------------

void MTextureAsset::applyGLParams()
{
    unsigned int texId = texture.getTextureID();
    if (texId == 0) return;

    glBindTexture(GL_TEXTURE_2D, texId);

    // Generate mipmaps before setting the filter if a mipmap mode is selected
    if (needsMipmaps(filterMin))
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGLFilterMin(filterMin));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGLFilterMag(filterMag));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     toGLWrap(wrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     toGLWrap(wrapT));

    glBindTexture(GL_TEXTURE_2D, 0);
}

// -- Import settings persistence -----------------------------------------------

void MTextureAsset::loadImportSettings(const pugi::xml_node& node)
{
    if (!node) return;

    if (auto n = node.child("filterMin"))   filterMin   = stringToFilterMin(n.text().as_string());
    if (auto n = node.child("filterMag"))   filterMag   = stringToFilterMag(n.text().as_string());
    if (auto n = node.child("wrapS"))       wrapS       = stringToWrap(n.text().as_string());
    if (auto n = node.child("wrapT"))       wrapT       = stringToWrap(n.text().as_string());
    if (auto n = node.child("maxSize"))     maxImportSize = n.text().as_int(0);
    if (auto n = node.child("compression")) compression = stringToCompression(n.text().as_string());
}

bool MTextureAsset::saveImportSettings()
{
    // Read existing meta file
    SString metaPath = path + ".meta";
    SString data;
    pugi::xml_document doc;

    if (FileIO::readFile(metaPath, data))
        doc.load_string(data.c_str());

    auto root = doc.child("asset_id");
    if (!root)
    {
        MERROR("MTextureAsset::saveImportSettings - meta file missing <asset_id> root: " + metaPath);
        return false;
    }

    // Remove old settings node if present, then write fresh
    root.remove_child("textureImport");
    auto settings = root.append_child("textureImport");

    settings.append_child("filterMin").text().set(filterMinToString(filterMin));
    settings.append_child("filterMag").text().set(filterMagToString(filterMag));
    settings.append_child("wrapS").text().set(wrapToString(wrapS));
    settings.append_child("wrapT").text().set(wrapToString(wrapT));
    settings.append_child("maxSize").text().set(maxImportSize);
    settings.append_child("compression").text().set(compressionToString(compression));

    std::ostringstream oss;
    doc.save(oss);
    SString str = oss.str();
    return FileIO::writeFile(metaPath, str);
}