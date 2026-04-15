//
// Created by ssj5v on 05-10-2024.
//

#pragma once
#ifndef IDRAWCALLSUBMITABLE_H
#define IDRAWCALLSUBMITABLE_H

class IRenderItemCollector;
// A drawable builds one (or more) SRenderItems and submits them to the
// collector.  It has no knowledge of which stages exist or how the item
// will be rendered - that is entirely the stage's responsibility.
class IMeteorDrawable
{
public:
    virtual ~IMeteorDrawable() = default;

    // Build and submit render item(s) for this frame.
    virtual void submitRenderItem(IRenderItemCollector* collector) = 0;

    // Return false to skip submission entirely (hidden, culled, not ready…).
    virtual bool canDraw() = 0;
};

#endif // IDRAWCALLSUBMITABLE_H