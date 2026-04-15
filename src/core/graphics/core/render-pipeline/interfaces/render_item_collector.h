//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef RENDER_ITEM_COLLECTOR_H
#define RENDER_ITEM_COLLECTOR_H

#include "core/graphics/core/render-pipeline/render_item.h"

// Drawables depend only on this, they know nothing about stages or the pipeline.
class IRenderItemCollector
{
public:
    virtual ~IRenderItemCollector() = default;
    virtual void submitRenderItem(const SRenderItem& item) = 0;
};

#endif // RENDER_ITEM_COLLECTOR_H