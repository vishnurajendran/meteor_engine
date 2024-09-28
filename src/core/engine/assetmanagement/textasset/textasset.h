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
    MTextAsset(SString path);
    SString getText() const;
    bool loadFromPath(const SString& path) override;
};



#endif //TEXTASSET_H
