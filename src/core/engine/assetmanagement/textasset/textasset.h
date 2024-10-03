//
// Created by ssj5v on 27-09-2024.
//

#ifndef TEXTASSET_H
#define TEXTASSET_H
#include "core/engine/assetmanagement/asset/asset.h"


class MTextAsset : public MAsset {
private:
    SString text;
public:
    MTextAsset(const SString& path);
    SString getText() const;
private:
    bool loadFromPath(const SString& path);
};



#endif //TEXTASSET_H
