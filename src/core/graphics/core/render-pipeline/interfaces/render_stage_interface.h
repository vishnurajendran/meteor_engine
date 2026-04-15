//
// Created by ssj5v on 11-04-2026.
//

#ifndef IRENDERSTAGE_H
#define IRENDERSTAGE_H


class IRenderPipeline;
class IRenderStage
{
public:
    virtual ~IRenderStage() = default;

    virtual int getSortingOrder() = 0;
    virtual void init(IRenderPipeline* const pipeline) = 0;
    virtual void cleanup(IRenderPipeline* const pipeline) = 0;
    virtual void preRender(IRenderPipeline* const pipeline) = 0;
    virtual void render(IRenderPipeline* const pipeline) = 0;
    virtual void postRender(IRenderPipeline* const pipeline) = 0;
};

#endif //IRENDERSTAGE_H
