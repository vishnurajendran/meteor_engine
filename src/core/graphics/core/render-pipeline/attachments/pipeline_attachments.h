//
// Pipeline attachments - the per-pipeline container of IPipelineAttachment.
//

#pragma once
#ifndef PIPELINE_ATTACHMENTS_H
#define PIPELINE_ATTACHMENTS_H

#include <memory>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include "pipeline_attachment.h"

// Keyed by C++ type, one instance per type. Kept in insertion order so
// attachments are detached in reverse of how they were added.
// Owned by IRenderPipeline; use the pipeline's addAttachment / getAttachment /
// removeAttachment rather than this class directly.
class SPipelineAttachments
{
public:
    SPipelineAttachments() = default;
    SPipelineAttachments(const SPipelineAttachments&) = delete;
    SPipelineAttachments& operator=(const SPipelineAttachments&) = delete;

    // Creates T, calls onAttach, returns it. If T is already attached,
    // returns the existing instance instead.
    template <typename T, typename... Args>
    T* add(IRenderPipeline* pipeline, Args&&... args)
    {
        static_assert(std::is_base_of_v<IPipelineAttachment, T>,
                      "T must derive from IPipelineAttachment");
        if (T* existing = get<T>()) return existing;

        auto owned = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = owned.get();
        entries.push_back({ std::type_index(typeid(T)), std::move(owned) });
        raw->onAttach(pipeline);
        return raw;
    }

    template <typename T>
    T* get() const
    {
        const std::type_index key(typeid(T));
        for (const auto& e : entries)
            if (e.type == key) return static_cast<T*>(e.attachment.get());
        return nullptr;
    }

    // Calls onDetach and deletes T. Returns false if T wasn't attached.
    template <typename T>
    bool remove(IRenderPipeline* pipeline)
    {
        const std::type_index key(typeid(T));
        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            if (it->type != key) continue;
            it->attachment->onDetach(pipeline);
            entries.erase(it);
            return true;
        }
        return false;
    }

    void resizeAll(int width, int height)
    {
        for (auto& e : entries) e.attachment->onResize(width, height);
    }

    // Detaches and deletes everything, newest first.
    void clear(IRenderPipeline* pipeline)
    {
        while (!entries.empty())
        {
            entries.back().attachment->onDetach(pipeline);
            entries.pop_back();
        }
    }

private:
    struct SEntry
    {
        std::type_index                      type;
        std::unique_ptr<IPipelineAttachment> attachment;
    };
    std::vector<SEntry> entries;
};

#endif // PIPELINE_ATTACHMENTS_H
