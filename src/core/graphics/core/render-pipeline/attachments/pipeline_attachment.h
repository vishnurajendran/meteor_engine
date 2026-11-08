//
// Pipeline attachment - a long-lived object owned by a render pipeline.
//

#pragma once
#ifndef PIPELINE_ATTACHMENT_H
#define PIPELINE_ATTACHMENT_H

class IRenderPipeline;

// Anything a pipeline owns that isn't a buffer or a stage: per-pipeline
// systems that stages read and that the rest of the engine may configure
// (e.g. MPostProcessStack). One instance per type per pipeline.
//
// Lifecycle (driven by the pipeline):
//   onAttach   - right after addAttachment<T>(); the pipeline is initialised
//                but stages may not be yet. GL is available.
//   onResize   - whenever the pipeline's render resolution changes.
//   onDetach   - before the attachment is deleted (pipeline cleanup or
//                removeAttachment<T>()). Free GPU resources here.
//
// Stages look attachments up with pipeline->getAttachment<T>(), usually once
// in init(), and may keep the pointer: attachments live until the pipeline
// is cleaned up, after every stage's cleanup().
class IPipelineAttachment
{
public:
    virtual ~IPipelineAttachment() = default;

    virtual void onAttach(IRenderPipeline* pipeline) {}
    virtual void onDetach(IRenderPipeline* pipeline) {}
    virtual void onResize(int width, int height)     {}
};

#endif // PIPELINE_ATTACHMENT_H
