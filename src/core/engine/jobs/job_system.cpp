//
// job_system.cpp
//

#include "job_system.h"

#include <algorithm>
#include "core/utils/logger.h"

MJobSystem::~MJobSystem()
{
    // The registry calls cleanup() first; this only covers an instance that
    // was created but never registered.
    cleanup();
}

void MJobSystem::init()
{
    if (!workers.empty()) return;   // already running

    // hardware_concurrency() may return 0 when unknown — treat as 1 core.
    const uint32_t cores       = std::max(1u, std::thread::hardware_concurrency());
    const uint32_t workerCount = cores - 1;   // the calling thread is the last "worker"

    stopping = false;
    workers.reserve(workerCount);
    for (uint32_t i = 0; i < workerCount; ++i)
        workers.emplace_back(&MJobSystem::workerLoop, this);

    MLOG(SString::format("MJobSystem: started {0} worker thread(s)", workerCount));
}

void MJobSystem::cleanup()
{
    {
        std::lock_guard lock(mutex);
        if (workers.empty()) return;
        stopping = true;
    }
    workCv.notify_all();

    for (std::thread& t : workers)
        if (t.joinable()) t.join();
    workers.clear();

    // No parallelFor can be in flight here — every caller blocks until its
    // batch is done, and cleanup runs at shutdown from the main thread.
    queue.clear();
}

// -----------------------------------------------------------------------------
// Batch bookkeeping — always called with `mutex` held
// -----------------------------------------------------------------------------

bool MJobSystem::claimIndex(SBatch& batch, uint32_t& outIndex)
{
    if (batch.next >= batch.count) return false;

    outIndex = batch.next++;

    // Last index claimed — nobody needs to find this batch on the queue any
    // more. It is usually at the front, so this is cheap.
    if (batch.next == batch.count)
    {
        const auto it = std::find(queue.begin(), queue.end(), &batch);
        if (it != queue.end()) queue.erase(it);
    }
    return true;
}

bool MJobSystem::finishTask(SBatch& batch)
{
    return --batch.remaining == 0;
}

// -----------------------------------------------------------------------------
// parallelFor — the calling thread's side
// -----------------------------------------------------------------------------

void MJobSystem::dispatch(uint32_t count, TaskFn fn, const void* context)
{
    SBatch batch;
    batch.fn        = fn;
    batch.context   = context;
    batch.count     = count;
    batch.remaining = count;

    {
        std::lock_guard lock(mutex);
        queue.push_back(&batch);
    }
    workCv.notify_all();

    // Help with our own batch — never other batches, so this call can't get
    // stuck running someone else's long task while ours is already done.
    for (;;)
    {
        uint32_t index = 0;
        {
            std::lock_guard lock(mutex);
            if (!claimIndex(batch, index)) break;
        }

        batch.fn(batch.context, index);

        std::lock_guard lock(mutex);
        finishTask(batch);   // our own wait below re-checks `remaining` — no notify needed
    }

    // Every index is claimed. Wait for the ones still running on workers.
    // `batch` must stay alive until then — workers touch it when they finish.
    std::unique_lock lock(mutex);
    doneCv.wait(lock, [&batch] { return batch.remaining == 0; });
}

// -----------------------------------------------------------------------------
// Worker threads
// -----------------------------------------------------------------------------

void MJobSystem::workerLoop()
{
    std::unique_lock lock(mutex);
    for (;;)
    {
        workCv.wait(lock, [this] { return stopping || !queue.empty(); });
        if (stopping) return;

        SBatch*  batch = queue.front();
        uint32_t index = 0;
        if (!claimIndex(*batch, index))
            continue;   // can't happen — a batch leaves the queue when its last index is claimed

        lock.unlock();
        batch->fn(batch->context, index);
        lock.lock();

        // After the decrement the caller may return and destroy `batch` as
        // soon as it can take the lock — so don't touch `batch` past here.
        // doneCv belongs to the job system, not the batch, so notifying it
        // after unlocking is safe.
        if (finishTask(*batch))
        {
            lock.unlock();
            doneCv.notify_all();
            lock.lock();
        }
    }
}
