//
// Created by ssj5v on 04-06-2026.
//

#ifndef SCRIPT_ASSET_H
#define SCRIPT_ASSET_H

class IScriptAsset
{
public:
    IScriptAsset() = default;
    virtual ~IScriptAsset() = default;

    virtual SString getScriptSrc() const = 0;
};

#endif //SCRIPT_ASSET_H
