#include "rl_shared_gpu.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// NOTE: We keep this module independent of OpenGL headers.
//       Actual glDelete* calls are performed by rlgl (draining pending deletes).

namespace {

struct RLSharedGpuGroup {
    std::mutex m;
    std::unordered_map<uint64_t, uint32_t> refs;
    std::deque<uint64_t> pending;
    std::unordered_map<uint64_t, void*> programLocs;
    std::unordered_map<uint32_t, uint64_t> framebufferDepth;  // fboId -> depthKey (type+id)
    std::unordered_set<uint64_t> pendingSet; // dedupe
    std::atomic<uint32_t> ctxRefs{1};
};

static inline uint64_t MakeKey(uint32_t type, uint32_t id)
{
    return (uint64_t(type) << 32) | uint64_t(id);
}

static inline void SplitKey(uint64_t key, uint32_t &type, uint32_t &id)
{
    type = uint32_t(key >> 32);
    id = uint32_t(key & 0xffffffffu);
}

static RLSharedGpuGroup *NewGroup()
{
    return new RLSharedGpuGroup();
}

static void DeleteGroup(RLSharedGpuGroup *g)
{
    if (!g) return;
    {
        std::lock_guard<std::mutex> lock(g->m);

        // Leak diagnostics (GPU objects are tracked by refcounts; pending holds deferred deletes).
        // We do not call any GL APIs here.
        if (!g->refs.empty() || !g->pending.empty()) {
            uint32_t liveByType[8] = {0};
            for (auto &kv : g->refs) {
                uint32_t t=0, id=0; SplitKey(kv.first, t, id);
                if (t < 8) liveByType[t] += 1;
            }
            uint32_t pendByType[8] = {0};
            for (auto &k : g->pending) {
                uint32_t t=0, id=0; SplitKey(k, t, id);
                if (t < 8) pendByType[t] += 1;
            }
            std::fprintf(stderr,
                "[rl_shared_gpu] WARNING: share-group destroyed with live GPU refs/pending deletes. "
                "live=%zu pending=%zu\n",
                (size_t)g->refs.size(), (size_t)g->pending.size());
            std::fprintf(stderr,
                "  live: tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u\n",
                liveByType[RL_SHARED_GPU_OBJECT_TEXTURE],
                liveByType[RL_SHARED_GPU_OBJECT_BUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
                liveByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_PROGRAM]);
            std::fprintf(stderr,
                "  pend: tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u\n",
                pendByType[RL_SHARED_GPU_OBJECT_TEXTURE],
                pendByType[RL_SHARED_GPU_OBJECT_BUFFER],
                pendByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
                pendByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
                pendByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
                pendByType[RL_SHARED_GPU_OBJECT_PROGRAM]);
        }

        for (auto &kv : g->programLocs) {
            if (kv.second) RL_FREE(kv.second);
        }
        g->programLocs.clear();
    }
    delete g;
}

static RLSharedGpuGroup *EnsureGroupForContext(RLContext *ctx)
{
    if (!ctx) return nullptr;
    if (!ctx->gpuShareGroup) {
        ctx->gpuShareGroup = (void *)NewGroup();
    }
    return (RLSharedGpuGroup *)ctx->gpuShareGroup;
}

static RLSharedGpuGroup *GetGroupForContext(RLContext *ctx)
{
    if (!ctx) return nullptr;
    return (RLSharedGpuGroup *)ctx->gpuShareGroup;
}

static RLContext *GetCurrentContextSafe()
{
    // RLGetCurrentContext is part of the public API and safe to call here.
    return RLGetCurrentContext();
}

static RLSharedGpuGroup *EnsureGroupForCurrentContext()
{
    RLContext *ctx = GetCurrentContextSafe();
    return EnsureGroupForContext(ctx);
}

static RLSharedGpuGroup *GetGroupForCurrentContext()
{
    RLContext *ctx = GetCurrentContextSafe();
    return GetGroupForContext(ctx);
}

static void PushPendingDelete(RLSharedGpuGroup *g, uint64_t key)
{
    if (!g) return;
    if (g->pendingSet.insert(key).second) {
        g->pending.push_back(key);
    }
}

static void RegisterObjectInGroup(RLSharedGpuGroup *g, RLSharedGpuObjectType type, unsigned int id)
{
    if (!g || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->refs.find(key);
    if (it == g->refs.end()) g->refs.emplace(key, 1);
    else it->second += 1;
}

static void RetainObjectInGroup(RLSharedGpuGroup *g, RLSharedGpuObjectType type, unsigned int id)
{
    if (!g || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->refs.find(key);
    if (it == g->refs.end()) {
        // If this object was never registered, assume an implicit owner reference already exists.
        g->refs.emplace(key, 2);
    } else {
        it->second += 1;
    }
}

static void RetainKeyLocked(RLSharedGpuGroup *g, uint64_t key)
{
    if (!g || key == 0) return;
    auto it = g->refs.find(key);
    if (it == g->refs.end()) {
        // If this object was never registered, assume an implicit owner reference already exists.
        g->refs.emplace(key, 2);
    } else {
        it->second += 1;
    }
}

static void ReleaseKeyLocked(RLSharedGpuGroup *g, RLSharedGpuObjectType type, uint64_t key)
{
    if (!g || key == 0) return;
    auto it = g->refs.find(key);
    if (it == g->refs.end()) {
        // Not tracked => treat as single-owner object; enqueue deletion.
        PushPendingDelete(g, key);
        return;
    }

    if (it->second <= 1) {
        g->refs.erase(it);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto itL = g->programLocs.find(key);
            if (itL != g->programLocs.end()) {
                if (itL->second) RL_FREE(itL->second);
                g->programLocs.erase(itL);
            }
        }
        PushPendingDelete(g, key);
        return;
    }

    it->second -= 1;
}

static void ReleaseObjectInGroup(RLSharedGpuGroup *g, RLSharedGpuObjectType type, unsigned int id)
{
    if (!g || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->refs.find(key);
    if (it == g->refs.end()) {
        // Not tracked => treat as single-owner object; enqueue deletion.
        PushPendingDelete(g, key);
        return;
    }

    if (it->second <= 1) {
        g->refs.erase(it);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto itL = g->programLocs.find(key);
            if (itL != g->programLocs.end()) {
                if (itL->second) RL_FREE(itL->second);
                g->programLocs.erase(itL);
            }
        }
        PushPendingDelete(g, key);
        return;
    }

    it->second -= 1;
}

} // namespace

extern "C" {

void RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx)
{
    if (!ctx) return;

    RLSharedGpuGroup *desired = nullptr;
    if (shareWithCtx) desired = EnsureGroupForContext(shareWithCtx);

    // If already bound, but to a different group, move to desired group.
    if (ctx->gpuShareGroup) {
        RLSharedGpuGroup *current = (RLSharedGpuGroup *)ctx->gpuShareGroup;
        if (desired && current != desired) {
            // Drop current binding
            uint32_t prev = current->ctxRefs.fetch_sub(1, std::memory_order_acq_rel);
            if (prev == 1) DeleteGroup(current);

            // Bind to desired
            desired->ctxRefs.fetch_add(1, std::memory_order_acq_rel);
            ctx->gpuShareGroup = (void *)desired;
        }
        return;
    }

    if (desired) {
        desired->ctxRefs.fetch_add(1, std::memory_order_acq_rel);
        ctx->gpuShareGroup = (void *)desired;
    } else {
        ctx->gpuShareGroup = (void *)NewGroup();
    }
}

void RLSharedGpuContextUnbindShareGroup(RLContext *ctx)
{
    if (!ctx) return;
    RLSharedGpuGroup *g = (RLSharedGpuGroup *)ctx->gpuShareGroup;
    if (!g) return;

    ctx->gpuShareGroup = nullptr;
    uint32_t prev = g->ctxRefs.fetch_sub(1, std::memory_order_acq_rel);
    if (prev == 1) DeleteGroup(g);
}

void RLSharedGpuRegisterObject(RLSharedGpuObjectType type, unsigned int id)
{
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    RegisterObjectInGroup(g, type, id);
}

void RLSharedGpuRegisterProgramLocs(unsigned int programId, int *locs)
{
    if (programId == 0 || locs == nullptr) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;
    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->programLocs.find(key);
    if (it == g->programLocs.end()) {
        g->programLocs.emplace(key, (void*)locs);
    } else {
        // Should not happen; keep existing pointer, free the new one to avoid leaks.
        RL_FREE(locs);
    }
}

void RLSharedGpuRegisterFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType type, unsigned int objId)
{
    if (framebufferId == 0 || objId == 0) return;
    if (type != RL_SHARED_GPU_OBJECT_TEXTURE && type != RL_SHARED_GPU_OBJECT_RENDERBUFFER) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;
    const uint64_t depthKey = MakeKey((uint32_t)type, (uint32_t)objId);
    std::lock_guard<std::mutex> lock(g->m);
    g->framebufferDepth[(uint32_t)framebufferId] = depthKey;
}

void RLSharedGpuUnregisterFramebufferDepth(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return;
    std::lock_guard<std::mutex> lock(g->m);
    g->framebufferDepth.erase((uint32_t)framebufferId);
}

bool RLSharedGpuQueryFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType *typeOut, unsigned int *objIdOut)
{
    if (framebufferId == 0 || !typeOut || !objIdOut) return false;
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->framebufferDepth.find((uint32_t)framebufferId);
    if (it == g->framebufferDepth.end()) return false;

    uint32_t t = 0, id = 0;
    SplitKey(it->second, t, id);
    *typeOut = (RLSharedGpuObjectType)t;
    *objIdOut = (unsigned int)id;
    return true;
}

void RLSharedGpuRetainFramebufferTree(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;
    const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
    std::lock_guard<std::mutex> lock(g->m);
    RetainKeyLocked(g, fboKey);
    auto it = g->framebufferDepth.find((uint32_t)framebufferId);
    if (it != g->framebufferDepth.end()) {
        RetainKeyLocked(g, it->second);
    }
}

void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;
    const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->framebufferDepth.find((uint32_t)framebufferId);
    if (it != g->framebufferDepth.end()) {
        uint32_t t=0, id=0;
        SplitKey(it->second, t, id);
        ReleaseKeyLocked(g, (RLSharedGpuObjectType)t, it->second);
    }
    ReleaseKeyLocked(g, RL_SHARED_GPU_OBJECT_FRAMEBUFFER, fboKey);
    // If framebuffer is no longer tracked, drop its attachment mapping to avoid staleness.
    if (g->refs.find(fboKey) == g->refs.end()) {
        g->framebufferDepth.erase((uint32_t)framebufferId);
    }
}


void RLSharedGpuRetainObject(RLSharedGpuObjectType type, unsigned int id)
{
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    RetainObjectInGroup(g, type, id);
}

void RLSharedGpuReleaseObject(RLSharedGpuObjectType type, unsigned int id)
{
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    ReleaseObjectInGroup(g, type, id);
}

void RLSharedGpuRetainObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    RLSharedGpuGroup *g = EnsureGroupForContext(ctx);
    RetainObjectInGroup(g, type, id);
}

void RLSharedGpuReleaseObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    RLSharedGpuGroup *g = EnsureGroupForContext(ctx);
    ReleaseObjectInGroup(g, type, id);
}

bool RLSharedGpuPopPendingDelete(RLSharedGpuObjectType *typeOut, unsigned int *idOut)
{
    if (!typeOut || !idOut) return false;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;

    uint64_t key = 0;
    {
        std::lock_guard<std::mutex> lock(g->m);
        if (g->pending.empty()) return false;
        key = g->pending.front();
        g->pending.pop_front();
        g->pendingSet.erase(key);
    }

    uint32_t t = 0, id = 0;
    SplitKey(key, t, id);
    *typeOut = (RLSharedGpuObjectType)t;
    *idOut = (unsigned int)id;
    return true;
}

void RLSharedGpuDebugDumpState(const char *label)
{
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) {
        std::fprintf(stderr, "[rl_shared_gpu] %s: (no share-group bound on current context)\n", label ? label : "state");
        return;
    }

    uint32_t liveByType[8] = {0};
    uint32_t pendByType[8] = {0};
    size_t live = 0, pend = 0;

    {
        std::lock_guard<std::mutex> lock(g->m);
        live = g->refs.size();
        pend = g->pending.size();
        for (auto &kv : g->refs) {
            uint32_t t=0, id=0; SplitKey(kv.first, t, id);
            if (t < 8) liveByType[t] += 1;
        }
        for (auto &k : g->pending) {
            uint32_t t=0, id=0; SplitKey(k, t, id);
            if (t < 8) pendByType[t] += 1;
        }
    }

    std::fprintf(stderr,
        "[rl_shared_gpu] %s: live=%zu pending=%zu | live(tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u) "
        "pend(tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u)\n",
        label ? label : "state",
        live, pend,
        liveByType[RL_SHARED_GPU_OBJECT_TEXTURE],
        liveByType[RL_SHARED_GPU_OBJECT_BUFFER],
        liveByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
        liveByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
        liveByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
        liveByType[RL_SHARED_GPU_OBJECT_PROGRAM],
        pendByType[RL_SHARED_GPU_OBJECT_TEXTURE],
        pendByType[RL_SHARED_GPU_OBJECT_BUFFER],
        pendByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
        pendByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
        pendByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
        pendByType[RL_SHARED_GPU_OBJECT_PROGRAM]);
}

} // extern "C"
