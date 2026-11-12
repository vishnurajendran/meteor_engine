//
// parallel_for.h
//
// MParallel::forEach(items, fn) — calls fn(item) for every item of a span,
// split into chunks across MJobSystem's threads.
//
//   MParallel::forEach(particles, [dt](SParticleData& p) {
//       p.position += p.velocity * dt;
//   });
//
// Runs as a plain loop on the calling thread when
//   - the range is smaller than `threshold` (dispatch + wait would cost more
//     than the work), or
//   - no MJobSystem is registered (tools, tests, early startup).
//
// fn is a template parameter, so it is inlined into the chunk loop — no
// virtual call or std::function per item.
//
// fn must only touch the item it is given (plus read-only shared data).
// See job_system.h for what tasks must not do.
//
#pragma once
#ifndef METEOR_ENGINE_PARALLEL_FOR_H
#define METEOR_ENGINE_PARALLEL_FOR_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/engine/jobs/job_system.h"
#include "core/engine/subsystem/subsystem_registry.h"

namespace MParallel
{
    inline constexpr size_t DEFAULT_CHUNK_SIZE = 512;    // items per task — big enough to amortise the claim lock
    inline constexpr size_t DEFAULT_THRESHOLD  = 2048;   // below this — run serially

    template <typename T, typename Fn>
    void forEach(std::span<T> items, const Fn& fn,
                 size_t chunkSize = DEFAULT_CHUNK_SIZE,
                 size_t threshold = DEFAULT_THRESHOLD)
    {
        chunkSize = std::max<size_t>(chunkSize, 1);

        MJobSystem* jobs = nullptr;
        if (items.size() >= threshold && items.size() > chunkSize)
            jobs = MEngineSubsystemRegistry::getSubsystem<MJobSystem>();

        if (!jobs || jobs->getWorkerCount() == 0)
        {
            for (T& item : items) fn(item);
            return;
        }

        const size_t chunkCount = (items.size() + chunkSize - 1) / chunkSize;
        jobs->parallelFor(static_cast<uint32_t>(chunkCount), [&](uint32_t chunk)
        {
            const size_t begin = static_cast<size_t>(chunk) * chunkSize;
            const size_t end   = std::min(begin + chunkSize, items.size());
            for (size_t i = begin; i < end; ++i)
                fn(items[i]);
        });
    }
}

#endif // METEOR_ENGINE_PARALLEL_FOR_H
