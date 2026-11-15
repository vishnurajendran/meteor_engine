// assetthumbnailrequest.h
// Suggested location: editor/editorassetmanager/thumbnails/

#pragma once
#ifndef ASSET_THUMBNAIL_REQUEST_H
#define ASSET_THUMBNAIL_REQUEST_H

#include "core/object/object.h" // for SString

class MAsset;

enum class EThumbnailAssetType
{
    Unknown,
    StaticMesh,
    SkeletalMesh,  // drawn in bind pose
    Material,  // material asset - rendered onto a UV sphere
};

struct SAssetThumbnailRequest
{
    SString            assetId;
    MAsset*            asset    = nullptr;
    EThumbnailAssetType type    = EThumbnailAssetType::Unknown;
};

#endif // ASSET_THUMBNAIL_REQUEST_H