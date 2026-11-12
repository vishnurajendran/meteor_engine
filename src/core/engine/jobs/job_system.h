//
// job_system.h
//
// A small fork-join thread pool. One call — parallelFor(count, fn) — runs
// fn(0) .. fn(count-1) spread over the worker threads AND the calling
// thread, and returns only when every index has finished.
//
//   auto* jobs = MEngineSubsystemRegistry::getSubsystem<MJobSystem>();
//   jobs->parallelFor(chunkCount, [&](uint32_t chunk) { ... });
//
// Most code should not call this directly — use MParallel::forEach
// (core/utils/parallel_for.h), which chunks a range and falls back to a
// plain loop when the job system isn't registered or the range is small.
//
// -- How it works --------------------------------------------------------------
//
//   parallelFor() puts one SBatch on the queue (the batch lives on the
//   caller's stack) and wakes the workers. Workers and the caller then claim
//   indices from that batch one at a time until all are claimed. The caller
//   waits until every claimed index has finished — the batch must outlive
//   every task, because tasks touch it when they finish.
//
//   All batch bookkeeping happens under one mutex. Claiming an index is a
//   lock + increment, so tasks should be coarse (a chunk of hundreds of
//   items) — never one task per tiny item.
//
// -- Nested calls ---------------------------------------------------------------
//
//   A task may call parallelFor itself (e.g. a parallel module inside
//   parallel emitters). That can't deadlock: the inner caller claims its own
//   batch's indices until none are left, so the inner batch finishes even if
//   every worker is busy. The caller only waits for tasks that other threads
//   have already started — and those always run to completion.
//
// -- Rules for tasks ---------------------------------------------------------------
//
//   - Touch only data that no other task touches (e.g. your own chunk).
//   - Don't call into Lua, the scene, the asset manager, OpenGL, or anything
//     else that isn't thread-safe.
//   - Don't throw. An exception escaping a task on a worker thread ends the
//     program (std::terminate).
//
#pragma once
#ifndef METEOR_ENGINE_JOB_SYSTEM_H
#define METEOR_ENGINE_JOB_SYSTEM_H

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "core/engine/subsystem/subsystem_interface.h"
#include "core/object/object.h"

class MJobSystem : public MObject, public IEngineSubSystem
{
    DEFINE_OBJECT_SUBCLASS(MJobSystem)
public:
    MJobSystem() = default;
    ~MJobSystem() override;

    // Starts hardware_concurrency() - 1 workers — the thread that calls
    // parallelFor is the remaining one.
    void init() override;
    // Stops and joins every worker. Safe to call twice.
    void cleanup() override;

    [[nodiscard]] uint32_t getWorkerCount() const { return static_cast<uint32_t>(workers.size()); }

    // Runs fn(i) for every i in [0, count) and returns when all are done.
    // fn is called concurrently from several threads — it must be safe for
    // that (see the rules above). fn is taken by reference: it lives on the
    // caller's stack, which is fine because this call blocks until done.
    template <typename Fn>
    void parallelFor(uint32_t count, const Fn& fn)
    {
        if (count == 0) return;
        if (count == 1 || workers.empty())
        {
            // Nothing to share — skip the queue and its locking.
            for (uint32_t i = 0; i < count; ++i) fn(i);
            return;
        }
        dispatch(count, &invokeTask<Fn>, &fn);
    }

private:
    // Type-erased task: a plain function pointer + the address of the
    // caller's callable. Avoids std::function — no heap allocation per call.
    using TaskFn = void (*)(const void* context, uint32_t index);

    template <typename Fn>
    static void invokeTask(const void* context, uint32_t index)
    {
        (*static_cast<const Fn*>(context))(index);
    }

    struct SBatch
    {
        TaskFn      fn        = nullptr;
        const void* context   = nullptr;
        uint32_t    count     = 0;
        uint32_t    next      = 0;   // next unclaimed index      — guarded by mutex
        uint32_t    remaining = 0;   // claimed-or-not, unfinished — guarded by mutex
    };

    void dispatch(uint32_t count, TaskFn fn, const void* context);
    void workerLoop();

    // Under the lock: claims the next index of `batch`. Pops the batch off
    // the queue when its last index is claimed. False when none are left.
    bool claimIndex(SBatch& batch, uint32_t& outIndex);
    // Under the lock: marks one task of `batch` finished. True when it was
    // the last one — the caller must then notify doneCv after unlocking.
    static bool finishTask(SBatch& batch);

    std::vector<std::thread> workers;

    std::mutex              mutex;    // guards queue, stopping and every SBatch's next / remaining
    std::condition_variable workCv;   // workers sleep here while the queue is empty
    std::condition_variable doneCv;   // parallelFor callers sleep here until their batch is done
    std::deque<SBatch*>     queue;    // batches that still have unclaimed indices
    bool                    stopping = false;
};

#endif // METEOR_ENGINE_JOB_SYSTEM_H
