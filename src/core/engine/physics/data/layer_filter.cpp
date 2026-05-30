//
// layer_filter.cpp
//

#include "layer_filter.h"
#include "core/engine/engine_statics.h"

void SLayerFilter::addLayerByName(const SString& layerName)
{
    const auto* layers = MEngineStatics::getPhysicsLayers();
    if (!layers)
    {
        MWARN("SLayerFilter::addLayerByName — physics layers asset not loaded; ignoring");
        return;
    }
    addLayerIndex(layers->getIndexForName(layerName));
}

SLayerFilter SLayerFilter::createFromName(const SString& layerName)
{
    const auto* layers = MEngineStatics::getPhysicsLayers();
    if (!layers)
    {
        MWARN("SLayerFilter::createFromName — physics layers asset not loaded; returning Default layer filter");
        return createFromIndex(0);
    }
    return createFromIndex(layers->getIndexForName(layerName));
}