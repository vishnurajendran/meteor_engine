//
// Created by ssj5v on 06-10-2024.
//

#ifndef DEFFEREDLOADABLEASSET_H
#define DEFFEREDLOADABLEASSET_H

class IDefferedLoadableAsset {
public:
    virtual void defferedAssetLoad(bool forced=false) = 0;
};

#endif //DEFFEREDLOADABLEASSET_H
