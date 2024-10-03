//
// Created by ssj5v on 27-09-2024.
//

#ifndef MASSET_H
#define MASSET_H
#include "core/object/object.h"

class MAsset : public MObject {
protected:
    SString path;
    bool valid;
public:
    MAsset(const SString& path);
    SString getPath() const;
    bool isValid() const;
};

#endif //MASSET_H
