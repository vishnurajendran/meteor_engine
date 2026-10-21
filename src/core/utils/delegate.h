//
// delegate.h
//
// The engine's one way to do events: something happens, and any number of
// listeners are told.
//
//   TMulticastDelegate<const SCollisionData&> onCollisionStart;   // the owner declares it
//
//   SCallbackDelegateHandle handle = body->onCollisionStart.add(   // a listener subscribes
//       [](const SCollisionData& data) { ... });
//   handle.invalidate();                                           // ...and unsubscribes
//
//   onCollisionStart.broadcast(data);                              // the owner fires it
//
// A listener object that subscribes in its constructor should keep an
// SScopedCallbackDelegateHandle instead: it unsubscribes on destruction.
//
// Guarantees:
//   - Listeners are called in the order they subscribed.
//   - Handles never dangle. A handle holds only a weak reference to the
//     event; if the event's owner is destroyed first, invalidate() and
//     isValid() are safe no-ops. Copies of a handle always agree.
//   - Listeners may add or remove listeners - themselves included - while
//     being called. A removed listener is not called again, even later in
//     the same broadcast; an added one is first called on the next one.
//   - Thread-safe: add, remove and broadcast may run on different threads.
//     Listeners run on the broadcasting thread with no lock held, so they
//     may use the delegate freely. A listener removed on one thread while
//     another is mid-broadcast may still receive the call already starting.
//
// Single functions that compute or do something on request (factories,
// visitors, predicates, draw callbacks) are not events: keep those as
// std::function.
//

#pragma once
#ifndef METEOR_DELEGATE_H
#define METEOR_DELEGATE_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

// Lets a handle reach any delegate without knowing its signature.
class IDelegateRegistry
{
public:
    virtual ~IDelegateRegistry() = default;
    virtual void remove(uint64_t id) = 0;
    [[nodiscard]] virtual bool contains(uint64_t id) const = 0;
};

// Identifies one subscription. Cheap to copy.
class SCallbackDelegateHandle
{
public:
    SCallbackDelegateHandle() = default;
    SCallbackDelegateHandle(std::weak_ptr<IDelegateRegistry> registry, uint64_t id)
        : registry(std::move(registry)), id(id) {}

    // Unsubscribes. Safe to call twice, on a default handle, or after the
    // event's owner has been destroyed.
    void invalidate()
    {
        if (const auto r = registry.lock())
            r->remove(id);
        registry.reset();
        id = 0;
    }

    // True while the subscription is live.
    [[nodiscard]] bool isValid() const
    {
        const auto r = registry.lock();
        return r && r->contains(id);
    }

private:
    std::weak_ptr<IDelegateRegistry> registry;
    uint64_t id = 0;
};

// Owns a subscription and ends it when destroyed. Move-only.
//
//   class MConsoleWindow {
//       SScopedCallbackDelegateHandle logSubscription;
//       MConsoleWindow() { logSubscription = MLogger::onLog().add(...); }
//   };   // no destructor needed
class SScopedCallbackDelegateHandle
{
public:
    SScopedCallbackDelegateHandle() = default;
    SScopedCallbackDelegateHandle(SCallbackDelegateHandle handle)
        : handle(std::move(handle)) {}
    ~SScopedCallbackDelegateHandle() { handle.invalidate(); }

    SScopedCallbackDelegateHandle(const SScopedCallbackDelegateHandle&)            = delete;
    SScopedCallbackDelegateHandle& operator=(const SScopedCallbackDelegateHandle&) = delete;

    SScopedCallbackDelegateHandle(SScopedCallbackDelegateHandle&& other) noexcept
        : handle(std::exchange(other.handle, {})) {}

    SScopedCallbackDelegateHandle& operator=(SScopedCallbackDelegateHandle&& other) noexcept
    {
        if (this != &other)
        {
            handle.invalidate();
            handle = std::exchange(other.handle, {});
        }
        return *this;
    }

    void invalidate() { handle.invalidate(); }
    [[nodiscard]] bool isValid() const { return handle.isValid(); }

private:
    SCallbackDelegateHandle handle;
};

template <typename... Args>
class TMulticastDelegate
{
public:
    using Callback = std::function<void(Args...)>;

    TMulticastDelegate() = default;

    // An event belongs to its owner. Copying would leave two events sharing
    // (or silently splitting) one set of listeners.
    TMulticastDelegate(const TMulticastDelegate&)            = delete;
    TMulticastDelegate& operator=(const TMulticastDelegate&) = delete;

    // Subscribes. Keep the handle to unsubscribe later; an empty callback
    // gives an invalid handle and subscribes nothing.
    [[nodiscard]] SCallbackDelegateHandle add(Callback callback)
    {
        if (!callback)
            return {};
        std::lock_guard lock(registry->mutex);
        auto entry = std::make_shared<Entry>(registry->nextId++, std::move(callback));
        registry->entries.push_back(entry);
        return { registry, entry->id };
    }

    // Calls every listener, in subscription order.
    void broadcast(Args... args) const
    {
        // Held locally: a listener may destroy this delegate's owner.
        const std::shared_ptr<Registry> keepAlive = registry;

        // Call a snapshot, not the live list - listeners may add or remove
        // listeners. Each entry stays alive while its callback runs, even
        // if it unsubscribes itself.
        std::vector<std::shared_ptr<Entry>> snapshot;
        {
            std::lock_guard lock(keepAlive->mutex);
            snapshot = keepAlive->entries;
        }
        for (const auto& entry : snapshot)
            if (entry->alive.load(std::memory_order_acquire))
                entry->callback(args...);
    }

    // Unsubscribes every listener; their handles become invalid.
    void clear()
    {
        std::lock_guard lock(registry->mutex);
        for (const auto& entry : registry->entries)
            entry->alive.store(false, std::memory_order_release);
        registry->entries.clear();
    }

    [[nodiscard]] std::size_t size() const
    {
        std::lock_guard lock(registry->mutex);
        return registry->entries.size();
    }

    [[nodiscard]] bool empty() const { return size() == 0; }

private:
    struct Entry
    {
        Entry(uint64_t id, Callback callback) : id(id), callback(std::move(callback)) {}
        const uint64_t    id;
        Callback          callback;
        std::atomic<bool> alive{ true };
    };

    struct Registry final : IDelegateRegistry
    {
        mutable std::mutex                   mutex;
        std::vector<std::shared_ptr<Entry>>  entries;   // subscription order
        uint64_t                             nextId = 1;

        void remove(uint64_t id) override
        {
            std::lock_guard lock(mutex);
            std::erase_if(entries, [id](const std::shared_ptr<Entry>& entry)
            {
                if (entry->id != id)
                    return false;
                entry->alive.store(false, std::memory_order_release);
                return true;
            });
        }

        [[nodiscard]] bool contains(uint64_t id) const override
        {
            std::lock_guard lock(mutex);
            return std::ranges::any_of(entries, [id](const std::shared_ptr<Entry>& entry)
            {
                return entry->id == id;
            });
        }
    };

    std::shared_ptr<Registry> registry = std::make_shared<Registry>();
};

#endif // METEOR_DELEGATE_H
