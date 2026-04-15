//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef MBUFFERREGISTRY_H
#define MBUFFERREGISTRY_H

#include <map>
#include <type_traits>

#include "buffer.h"
#include "buffers/render/renderbuffer.h"
#include "core/object/object.h"

class MBufferRegistery : public MObject
{
public:
    ~MBufferRegistery() override;

    // -----------------------------------------------------------------------
    // Buffer creation
    //
    // Allocates a new T, calls makeBuffer(name), stores it, and returns a
    // typed pointer.  Returns nullptr if makeBuffer fails.
    // Replaces any existing buffer registered under the same name.
    // -----------------------------------------------------------------------
    template<typename T>
    T* createBuffer(const SString& name)
    {
        static_assert(std::is_base_of_v<SBuffer, T>,
                      "T must derive from SBuffer");

        // Evict and destroy any existing registration under this name.
        auto it = mBuffers.find(name);
        if (it != mBuffers.end())
        {
            it->second->unbind();
            delete it->second;
            mBuffers.erase(it);
        }

        T* inst = new T();
        if (!inst->makeBuffer(name))
        {
            delete inst;
            return nullptr;
        }

        mBuffers[name] = inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Buffer retrieval
    //
    // Returns nullptr if the name is not registered or the stored buffer
    // cannot be cast to T.
    // -----------------------------------------------------------------------
    template<typename T = SBuffer>
    T* getBuffer(const SString& name) const
    {
        static_assert(std::is_base_of_v<SBuffer, T>,
                      "T must derive from SBuffer");

        auto it = mBuffers.find(name);
        if (it == mBuffers.end()) return nullptr;
        return dynamic_cast<T*>(it->second);
    }

    // -----------------------------------------------------------------------
    // Bulk resize
    //
    // Called by the pipeline when the render resolution changes.
    // Forwards to every registered buffer's resize() virtual.
    // -----------------------------------------------------------------------
    void resizeAll(int width, int height);

    // -----------------------------------------------------------------------
    // Render target — set externally by the application, not owned here.
    // -----------------------------------------------------------------------
    void           setRenderBuffer(SRenderBuffer* renderBuffer);
    SRenderBuffer* getRenderBuffer() const { return renderBufferHandle; }

private:
    std::map<SString, SBuffer*> mBuffers;
    SRenderBuffer*              renderBufferHandle = nullptr;
};

#endif // MBUFFERREGISTRY_H