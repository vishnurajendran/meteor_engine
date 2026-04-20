#pragma once

#include "gc.h"
#include "object.h"

template<typename T>
class MObjectPtr
{
    static_assert(std::is_base_of<MObject, T>::value, "T must derive from MObject");

public:
    // ── Construction ──────────────────────────────────────────────────────────

    MObjectPtr(T* obj = nullptr) : ptr(obj)
    {
        if (ptr) MGarbageCollector::reference(ptr);
    }

    ~MObjectPtr()
    {
        release();
    }

    // Copy
    MObjectPtr(const MObjectPtr& other) : ptr(other.ptr)
    {
        if (ptr) MGarbageCollector::reference(ptr);
    }

    // Move — ownership transfers; ref-count stays the same.
    MObjectPtr(MObjectPtr&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }

    MObjectPtr& operator=(const MObjectPtr& other)
    {
        if (this != &other)
            assign(other.ptr);
        return *this;
    }

    MObjectPtr& operator=(MObjectPtr&& other) noexcept
    {
        if (this != &other)
        {
            release();
            ptr       = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // Assign from a raw pointer (e.g. MObjectPtr<Foo> p; p = new Foo();)
    MObjectPtr& operator=(T* raw)
    {
        if (ptr != raw)
            assign(raw);
        return *this;
    }

    // Assign nullptr explicitly
    MObjectPtr& operator=(std::nullptr_t)
    {
        release();
        return *this;
    }

    // ── Accessors ─────────────────────────────────────────────────────────────

    T* get() const                  { return ptr; }

    T& operator*()  const           { return *ptr; }
    T* operator->() const           { return ptr; }

    explicit operator bool() const  { return ptr != nullptr; }

    // ── Comparison ────────────────────────────────────────────────────────────

    bool operator==(std::nullptr_t)             const { return ptr == nullptr; }
    bool operator!=(std::nullptr_t)             const { return ptr != nullptr; }

    bool operator==(const MObjectPtr& other)    const { return ptr == other.ptr; }
    bool operator!=(const MObjectPtr& other)    const { return ptr != other.ptr; }

    bool operator==(const T* raw)               const { return ptr == raw; }
    bool operator!=(const T* raw)               const { return ptr != raw; }

    void reset(T* newPtr = nullptr)
    {
        if (ptr != newPtr)
            assign(newPtr);
    }

private:
    T* ptr = nullptr;
    void release()
    {
        if (ptr)
        {
            MGarbageCollector::dereference(ptr);
            ptr = nullptr;
        }
    }

    void assign(T* raw)
    {
        if (raw) MGarbageCollector::reference(raw);
        release();
        ptr = raw;
    }
};