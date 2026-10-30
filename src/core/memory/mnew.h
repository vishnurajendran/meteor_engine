//
// mnew.h
//
// Creating MObjects through the memory subsystem.
//
//   auto* light = mnew<MSpotLight>();               // instead of new MSpotLight()
//   auto* asset = mnew<MTextureAsset>(path);        // constructor args forwarded
//   delete light;                                   // still fine: MObject::operator delete
//   mdelete(light);                                 // same thing, reads as the pair to mnew
//
// Only MObject-derived types are allowed (static_assert). Everything else
// keeps using new/delete for now.
//
// Before the memory subsystem is registered (static initialisers, the
// application object) and after it is cleaned up, mnew falls back to the
// global heap. Deletion works either way: MObject::operator delete asks the
// allocator whether it owns the pointer and otherwise uses the heap.
//

#ifndef METEOR_ENGINE_MNEW_H
#define METEOR_ENGINE_MNEW_H

#include <cstddef>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <utility>

class MObject;
class IAllocator;
class IMemoryManagementSubsystem;

namespace MMemory
{
    // Set by the memory subsystem. mnew allocates through it while it is set.
    void setActiveSubsystem(IMemoryManagementSubsystem* subsystem);
    IMemoryManagementSubsystem* getActiveSubsystem();

    // The allocator frees are routed to. Outlives the subsystem when objects
    // are still alive at shutdown, so their later deletes still find it.
    void setRoutingAllocator(IAllocator* allocator);
    IAllocator* getRoutingAllocator();

    // Used by mnew: subsystem if active, else global heap.
    void* allocateObject(size_t size, size_t alignment, const std::type_info& type);

    // Used by MObject::operator delete: pool if it owns ptr, else global heap.
    void releaseObject(void* ptr);

    // How many mnew calls happened while no memory subsystem was active.
    size_t getHeapFallbackCount();
}

template<typename T, typename... Args>
T* mnew(Args&&... args)
{
    static_assert(std::is_base_of_v<MObject, T>, "mnew<T>: T must derive from MObject");
    static_assert(!std::is_abstract_v<T>, "mnew<T>: T is abstract");

    void* mem = MMemory::allocateObject(sizeof(T), alignof(T), typeid(T));
    try
    {
        return ::new (mem) T(std::forward<Args>(args)...);
    }
    catch (...)
    {
        MMemory::releaseObject(mem);
        throw;
    }
}

template<typename T>
void mdelete(T* obj)
{
    static_assert(std::is_base_of_v<MObject, T>, "mdelete<T>: T must derive from MObject");
    delete obj; // virtual destructor, then MObject::operator delete -> MMemory::releaseObject
}

#endif // METEOR_ENGINE_MNEW_H
