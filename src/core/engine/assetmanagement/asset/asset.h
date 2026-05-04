//
// Created by ssj5v on 27-09-2024.
//

#ifndef MASSET_H
#define MASSET_H
#include "core/object/object.h"
#include "defferedloadableasset.h"

class MAsset : public MObject, IDefferedLoadableAsset
{
    DEFINE_OBJECT_CLASS(MAsset)
protected:
    SString path;
    SString assetId;
    bool valid = false;

public:
    MAsset(const SString& path);
    ~MAsset() override = default;

    SString getPath() const;
    SString getFullPath() const;
    bool isValid() const;

    SString getAssetId() const { return assetId; }
    void internal_SetAssetId(const SString& assetId);

    // tries handling of asset open request.
    virtual bool openAsset() { return false; }

    // force a reload on asset
    virtual bool requestReload() = 0;

    /// Inform that this asset needs deferred loading.\n\n
    /// By default, the assets do not need deferred loading,
    /// but some assets like material will need to depend on another asset
    /// like shader to get all available properties for a specific shader.
    /// so setting this true, will allow the asset to have another pass at loading after
    /// all independent assets are loaded, giving the dependencies to be resolved without any
    /// issue
    virtual bool hasDeferredLoad() const override { return false; }

    /// Override this method to do all loading asset after all other independent assets are ready.\n\n
    /// This does not guarantee that all deferred loads will succeed, since an asset could be dependent
    /// on another deferred load asset, at this point the order will dictate the asset-load success
    virtual void deferredAssetLoad(bool forced) override
    {
        /* Do nothing here at this level, this function is used for late
         * loading some assets, when there is dependency on other assets.
         */
    }
};

#endif // MASSET_H
