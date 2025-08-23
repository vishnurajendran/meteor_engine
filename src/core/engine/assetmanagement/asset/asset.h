//
// Created by ssj5v on 27-09-2024.
//

#ifndef MASSET_H
#define MASSET_H
#include "core/object/object.h"

class MAsset : public MObject {
protected:
    SString path;
    SString assetId;
    bool valid = false;
public:
    MAsset(const SString& path);
    SString getPath() const;
    SString getFullPath() const;
    bool isValid() const;

    SString getAssetId() const {return assetId;}
    void internal_SetAssetId(const SString& assetId);
};

#endif //MASSET_H
