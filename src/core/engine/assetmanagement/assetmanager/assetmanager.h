//
// Created by ssj5v on 27-09-2024.
//

#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <map>
#include "core/object/object.h"

class MAsset;

namespace pugi {
    class xml_document;
}

class MAssetManager : public MObject {
public:
    static MAssetManager* getInstance();
    void refresh();
    void cleanup();
    template <typename T>
    T* getAsset(SString path) {
        static_assert(std::is_base_of<MAsset, T>::value,"T must inherit from MAsset");

        if(assetMap.contains(path)) {
            return dynamic_cast<T*>(assetMap[path]);
        }

        return nullptr;
    }

private:
    std::map<SString, MAsset*> assetMap;
    static MAssetManager* instance;
    void loadAssetRecursive(SString path);
    bool loadAsset(SString path);
};

#endif //ASSETMANAGER_H
