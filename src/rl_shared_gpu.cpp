#include "rl_shared_gpu.h"
#include "raylib.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

// NOTE: We keep this module independent of OpenGL headers.
//       Actual glDelete* calls are performed by rlgl (draining pending deletes).

namespace {

struct RLSharedGpuGroup {
    struct ProgramUseScope {
        std::recursive_mutex lockMutex;
        std::mutex phaseMutex;
        std::condition_variable phaseCv;
        uint64_t nextTicket = 0;
        uint64_t servingTicket = 0;
        void *lastFence = nullptr;
    };

    struct TextureTrace {
        std::string label;
        std::string markSource;
        std::string releaseSource;
        std::string releaseReason;
    };

    std::mutex m;
    std::unordered_map<uint64_t, uint32_t> refs;
    std::unordered_map<uint64_t, RLContext*> owners;
    std::unordered_set<uint64_t> orphanedOwners;
    std::deque<uint64_t> pending;
    std::unordered_map<uint64_t, void*> programLocs;
    std::unordered_map<uint32_t, uint64_t> framebufferDepth;  // fboId -> depthKey (type+id)
    std::unordered_map<uint32_t, std::unordered_map<int, uint64_t>> framebufferAttachments; // fboId -> (attachment -> key)
    std::unordered_map<uint32_t, TextureTrace> textureTrace;   // texture id -> trace metadata
    std::unordered_map<uint64_t, std::unique_ptr<ProgramUseScope>> programUseScopes;
    std::unordered_set<uint64_t> pendingSet; // dedupe
    size_t releaseUntrackedCount = 0;
    size_t fboAttachmentMapHitCount = 0;
    size_t fboAttachmentMapMissCount = 0;
    size_t fboAttachmentReleaseSkippedCount = 0;
    std::atomic<uint32_t> ctxRefs{1};
};

static inline uint64_t MakeKey(uint32_t type, uint32_t id)
{
    return (uint64_t(type) << 32) | uint64_t(id);
}

static std::atomic<int> gTrackingMode((int)RL_SHARED_GPU_TRACKING_MODE_STRICT);
static std::atomic<unsigned long long> gUnregisteredRetainRejectCount(0);
static std::atomic<unsigned long long> gUnregisteredReleaseRejectCount(0);

static inline bool IsStrictTrackingEnabled(void)
{
    return (gTrackingMode.load(std::memory_order_relaxed) == (int)RL_SHARED_GPU_TRACKING_MODE_STRICT);
}

static inline void SplitKey(uint64_t key, uint32_t &type, uint32_t &id)
{
    type = uint32_t(key >> 32);
    id = uint32_t(key & 0xffffffffu);
}

static inline std::string BuildSourceText(const char *sourceFile, int sourceLine)
{
    if (sourceFile == nullptr) return std::string("(unknown)");
    std::string out(sourceFile);
    out += ":";
    char lineBuf[32] = { 0 };
    std::snprintf(lineBuf, sizeof(lineBuf), "%d", sourceLine);
    out += lineBuf;
    return out;
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
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: share-group destroyed with live refs/pending deletes: live=%zu pending=%zu untrackedRelease=%zu",
                (size_t)g->refs.size(), (size_t)g->pending.size(), g->releaseUntrackedCount);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: live refs by type: tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u",
                liveByType[RL_SHARED_GPU_OBJECT_TEXTURE],
                liveByType[RL_SHARED_GPU_OBJECT_BUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
                liveByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
                liveByType[RL_SHARED_GPU_OBJECT_PROGRAM]);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: pending deletes by type: tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u",
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
        uint32_t type = 0, id = 0;
        SplitKey(key, type, id);
        if (type == RL_SHARED_GPU_OBJECT_TEXTURE) {
            auto it = g->textureTrace.find(id);
            if (it != g->textureTrace.end()) {
                if (it->second.label.find("font") != std::string::npos) {
                    RLTraceLog(RL_E_LOG_DEBUG,
                        "SHARED_GPU: font texture enqueue delete: id=%u label=%s mark=%s release=%s reason=%s",
                        id,
                        it->second.label.c_str(),
                        it->second.markSource.c_str(),
                        it->second.releaseSource.c_str(),
                        it->second.releaseReason.c_str());
                }
            }
        }
        g->pending.push_back(key);
    }
}

static void NoteUntrackedReleaseLocked(RLSharedGpuGroup *g, RLSharedGpuObjectType type, uint64_t key)
{
    if (!g || key == 0) return;
    g->releaseUntrackedCount += 1;
    uint32_t keyType = 0, keyId = 0;
    SplitKey(key, keyType, keyId);
    RLTraceLog(RL_E_LOG_WARNING,
        "SHARED_GPU: release on untracked object ignored (requestedType=%u keyType=%u id=%u untrackedCount=%zu)",
        (unsigned)type, (unsigned)keyType, (unsigned)keyId, g->releaseUntrackedCount);
}

static void RegisterObjectInGroup(RLSharedGpuGroup *g, RLSharedGpuObjectType type, unsigned int id)
{
    if (!g || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    RLContext *currentCtx = GetCurrentContextSafe();

    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->refs.find(key);
    if (it == g->refs.end())
    {
        g->refs.emplace(key, 1);
        if (currentCtx != nullptr)
        {
            g->owners[key] = currentCtx;
            g->orphanedOwners.erase(key);
        }
    }
    else
    {
        it->second += 1;
        // If owner is still unknown for an existing tracked object, lock it now to
        // the registering context. Retain/release paths must not implicitly change owner.
        auto ownerIt = g->owners.find(key);
        if ((ownerIt == g->owners.end()) || (ownerIt->second == nullptr))
        {
            if (currentCtx != nullptr)
            {
                g->owners[key] = currentCtx;
                g->orphanedOwners.erase(key);
            }
        }
    }
}

static unsigned int OrphanOwnersForContextLocked(RLSharedGpuGroup *g, RLContext *ctx)
{
    if ((g == nullptr) || (ctx == nullptr)) return 0u;

    unsigned int orphanedCount = 0u;
    for (auto &ownerEntry : g->owners)
    {
        if (ownerEntry.second != ctx) continue;
        ownerEntry.second = nullptr;
        g->orphanedOwners.insert(ownerEntry.first);
        orphanedCount++;
    }

    return orphanedCount;
}

static RLSharedGpuGroup::ProgramUseScope *GetOrCreateProgramUseScopeLocked(RLSharedGpuGroup *g, uint64_t key)
{
    if (!g || key == 0) return nullptr;
    auto it = g->programUseScopes.find(key);
    if (it != g->programUseScopes.end()) return it->second.get();

    std::unique_ptr<RLSharedGpuGroup::ProgramUseScope> scope(new RLSharedGpuGroup::ProgramUseScope());
    RLSharedGpuGroup::ProgramUseScope *scopePtr = scope.get();
    g->programUseScopes.emplace(key, std::move(scope));
    return scopePtr;
}

static void RetainObjectInGroup(RLSharedGpuGroup *g, RLSharedGpuObjectType type, unsigned int id)
{
    if (!g || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->refs.find(key);
    if (it == g->refs.end())
    {
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredRetainRejectCount.fetch_add(1, std::memory_order_relaxed);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: retain on unregistered object rejected (type=%u id=%u)",
                (unsigned)type, id);
            return;
        }

        // Compatibility mode: assume an implicit owner reference already exists.
        g->refs.emplace(key, 2);
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: retain on unregistered object accepted in compatibility mode (type=%u id=%u refs=2)",
            (unsigned)type, id);
    }
    else
    {
        it->second += 1;
    }
}

static void RetainKeyLocked(RLSharedGpuGroup *g, uint64_t key)
{
    if (!g || key == 0) return;
    auto it = g->refs.find(key);
    if (it == g->refs.end())
    {
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredRetainRejectCount.fetch_add(1, std::memory_order_relaxed);
            uint32_t type = 0, id = 0;
            SplitKey(key, type, id);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: retain on unregistered object rejected (type=%u id=%u)",
                (unsigned)type, id);
            return;
        }

        // Compatibility mode: assume an implicit owner reference already exists.
        g->refs.emplace(key, 2);
        uint32_t type = 0, id = 0;
        SplitKey(key, type, id);
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: retain on unregistered object accepted in compatibility mode (type=%u id=%u refs=2)",
            (unsigned)type, id);
    }
    else
    {
        it->second += 1;
    }
}

static void ReleaseKeyLocked(RLSharedGpuGroup *g, RLSharedGpuObjectType type, uint64_t key)
{
    if (!g || key == 0) return;
    auto it = g->refs.find(key);
    if (it == g->refs.end()) {
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredReleaseRejectCount.fetch_add(1, std::memory_order_relaxed);
            uint32_t keyType = 0, keyId = 0;
            SplitKey(key, keyType, keyId);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: release on unregistered object rejected (requestedType=%u keyType=%u id=%u)",
                (unsigned)type, (unsigned)keyType, (unsigned)keyId);
            return;
        }

        // Compatibility mode: ignore release in this group to avoid accidental cross-group deletes.
        NoteUntrackedReleaseLocked(g, type, key);
        return;
    }

    if (it->second <= 1) {
        g->refs.erase(it);
        g->owners.erase(key);
        g->orphanedOwners.erase(key);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto itL = g->programLocs.find(key);
            if (itL != g->programLocs.end()) {
                if (itL->second) RL_FREE(itL->second);
                g->programLocs.erase(itL);
            }
            g->programUseScopes.erase(key);
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
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredReleaseRejectCount.fetch_add(1, std::memory_order_relaxed);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: release on unregistered object rejected (type=%u id=%u)",
                (unsigned)type, id);
            return;
        }

        // Compatibility mode: ignore release in this group to avoid accidental cross-group deletes.
        NoteUntrackedReleaseLocked(g, type, key);
        return;
    }

    if (it->second <= 1) {
        g->refs.erase(it);
        g->owners.erase(key);
        g->orphanedOwners.erase(key);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto itL = g->programLocs.find(key);
            if (itL != g->programLocs.end()) {
                if (itL->second) RL_FREE(itL->second);
                g->programLocs.erase(itL);
            }
            g->programUseScopes.erase(key);
        }
        PushPendingDelete(g, key);
        return;
    }

    it->second -= 1;
}

static void RegisterFramebufferAttachmentInGroup(RLSharedGpuGroup *g, unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    if (!g || framebufferId == 0 || objId == 0) return;
    if (type != RL_SHARED_GPU_OBJECT_TEXTURE && type != RL_SHARED_GPU_OBJECT_RENDERBUFFER) return;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)objId);
    std::lock_guard<std::mutex> lock(g->m);
    g->framebufferAttachments[(uint32_t)framebufferId][attachment] = key;

    // Keep legacy depth mapping in sync for existing query path.
    if (attachment == 100) g->framebufferDepth[(uint32_t)framebufferId] = key;
}

static void UnregisterFramebufferAttachmentInGroup(RLSharedGpuGroup *g, unsigned int framebufferId, int attachment)
{
    if (!g || framebufferId == 0) return;
    std::lock_guard<std::mutex> lock(g->m);

    auto it = g->framebufferAttachments.find((uint32_t)framebufferId);
    if (it != g->framebufferAttachments.end())
    {
        it->second.erase(attachment);
        if (it->second.empty()) g->framebufferAttachments.erase(it);
    }

    // Keep legacy depth mapping in sync.
    if (attachment == 100) g->framebufferDepth.erase((uint32_t)framebufferId);
}

static void UnregisterFramebufferAttachmentsInGroup(RLSharedGpuGroup *g, unsigned int framebufferId)
{
    if (!g || framebufferId == 0) return;
    std::lock_guard<std::mutex> lock(g->m);
    g->framebufferAttachments.erase((uint32_t)framebufferId);
    g->framebufferDepth.erase((uint32_t)framebufferId);
}

} // namespace

extern "C" {

bool RLSharedGpuHasCurrentGroup(void)
{
    return (GetGroupForCurrentContext() != nullptr);
}

bool RLSharedGpuHasContextGroup(RLContext *ctx)
{
    return (GetGroupForContext(ctx) != nullptr);
}

void RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx)
{
    if (!ctx) return;

    RLSharedGpuGroup *desired = nullptr;
    if (shareWithCtx) desired = EnsureGroupForContext(shareWithCtx);

    // If already bound, but to a different group, move to desired group.
    if (ctx->gpuShareGroup) {
        RLSharedGpuGroup *current = (RLSharedGpuGroup *)ctx->gpuShareGroup;
        if (desired && current != desired) {
            {
                std::lock_guard<std::mutex> lock(current->m);
                (void)OrphanOwnersForContextLocked(current, ctx);
            }
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

    unsigned int orphanedCount = 0u;
    {
        std::lock_guard<std::mutex> lock(g->m);
        orphanedCount = OrphanOwnersForContextLocked(g, ctx);
    }
    if (orphanedCount > 0u)
    {
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: context unbind orphaned %u shared object owners; adopt explicitly if still needed",
            orphanedCount);
    }

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
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    RegisterFramebufferAttachmentInGroup(g, framebufferId, 100, type, objId);
}

void RLSharedGpuUnregisterFramebufferDepth(unsigned int framebufferId)
{
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    UnregisterFramebufferAttachmentInGroup(g, framebufferId, 100);
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
    auto it = g->framebufferAttachments.find((uint32_t)framebufferId);
    if (it != g->framebufferAttachments.end())
    {
        g->fboAttachmentMapHitCount += 1;
        std::unordered_set<uint64_t> dedup;
        for (const auto &slot : it->second)
        {
            if (dedup.insert(slot.second).second) RetainKeyLocked(g, slot.second);
        }
    }
    else
    {
        g->fboAttachmentMapMissCount += 1;
        // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
        auto depthIt = g->framebufferDepth.find((uint32_t)framebufferId);
        if (depthIt != g->framebufferDepth.end()) RetainKeyLocked(g, depthIt->second);
    }
}

void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;
    const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->framebufferAttachments.find((uint32_t)framebufferId);
    if (it != g->framebufferAttachments.end())
    {
        g->fboAttachmentMapHitCount += 1;
        std::unordered_set<uint64_t> dedup;
        for (const auto &slot : it->second)
        {
            if (!dedup.insert(slot.second).second) continue;
            if (g->refs.find(slot.second) == g->refs.end()) g->fboAttachmentReleaseSkippedCount += 1;
            uint32_t t = 0, id = 0;
            SplitKey(slot.second, t, id);
            ReleaseKeyLocked(g, (RLSharedGpuObjectType)t, slot.second);
        }
    }
    else
    {
        g->fboAttachmentMapMissCount += 1;
        // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
        auto depthIt = g->framebufferDepth.find((uint32_t)framebufferId);
        if (depthIt != g->framebufferDepth.end())
        {
            if (g->refs.find(depthIt->second) == g->refs.end()) g->fboAttachmentReleaseSkippedCount += 1;
            uint32_t t = 0, id = 0;
            SplitKey(depthIt->second, t, id);
            ReleaseKeyLocked(g, (RLSharedGpuObjectType)t, depthIt->second);
        }
    }
    ReleaseKeyLocked(g, RL_SHARED_GPU_OBJECT_FRAMEBUFFER, fboKey);
    // If framebuffer is no longer tracked, drop its attachment mapping to avoid staleness.
    if (g->refs.find(fboKey) == g->refs.end()) {
        g->framebufferAttachments.erase((uint32_t)framebufferId);
        g->framebufferDepth.erase((uint32_t)framebufferId);
    }
}

RLSharedGpuFramebufferMapStats RLSharedGpuGetFramebufferMapStats(void)
{
    RLSharedGpuFramebufferMapStats out = { 0 };
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return out;

    std::lock_guard<std::mutex> lock(g->m);
    out.mapHitCount = (unsigned long long)g->fboAttachmentMapHitCount;
    out.mapMissCount = (unsigned long long)g->fboAttachmentMapMissCount;
    out.releaseSkippedCount = (unsigned long long)g->fboAttachmentReleaseSkippedCount;
    return out;
}

bool RLSharedGpuGetObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext **ownerOut)
{
    if ((id == 0) || (ownerOut == nullptr)) return false;
    *ownerOut = nullptr;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    std::lock_guard<std::mutex> lock(g->m);
    if (g->refs.find(key) == g->refs.end()) return false;

    auto ownerIt = g->owners.find(key);
    if (ownerIt == g->owners.end()) return false;
    *ownerOut = ownerIt->second;
    return (*ownerOut != nullptr);
}

bool RLSharedGpuIsObjectOwnedByCurrentContext(RLSharedGpuObjectType type, unsigned int id)
{
    RLContext *owner = nullptr;
    if (!RLSharedGpuGetObjectOwner(type, id, &owner)) return false;
    return (owner == GetCurrentContextSafe());
}

bool RLSharedGpuTryTransferObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    if ((id == 0) || (targetCtx == nullptr)) return false;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;
    if (GetGroupForContext(targetCtx) != g) return false;

    RLContext *currentCtx = GetCurrentContextSafe();
    if (currentCtx == nullptr) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    std::lock_guard<std::mutex> lock(g->m);
    if (g->refs.find(key) == g->refs.end()) return false;

    auto ownerIt = g->owners.find(key);
    if ((ownerIt != g->owners.end()) && (ownerIt->second == nullptr) &&
        (g->orphanedOwners.find(key) != g->orphanedOwners.end())) return false;
    if ((ownerIt != g->owners.end()) && (ownerIt->second != nullptr) && (ownerIt->second != currentCtx)) return false;

    g->owners[key] = targetCtx;
    g->orphanedOwners.erase(key);
    return true;
}

bool RLSharedGpuTryAdoptOrphanedObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    if ((id == 0) || (targetCtx == nullptr)) return false;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;
    if (GetGroupForContext(targetCtx) != g) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    std::lock_guard<std::mutex> lock(g->m);
    if (g->refs.find(key) == g->refs.end()) return false;

    auto ownerIt = g->owners.find(key);
    if (ownerIt == g->owners.end()) return false;
    if (ownerIt->second != nullptr) return false;
    if (g->orphanedOwners.find(key) == g->orphanedOwners.end()) return false;

    ownerIt->second = targetCtx;
    g->orphanedOwners.erase(key);
    return true;
}

void RLSharedGpuRegisterFramebufferAttachment(unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    RegisterFramebufferAttachmentInGroup(g, framebufferId, attachment, type, objId);
}

void RLSharedGpuUnregisterFramebufferAttachment(unsigned int framebufferId, int attachment)
{
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    UnregisterFramebufferAttachmentInGroup(g, framebufferId, attachment);
}

void RLSharedGpuUnregisterFramebufferAttachments(unsigned int framebufferId)
{
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    UnregisterFramebufferAttachmentsInGroup(g, framebufferId);
}

bool RLSharedGpuBeginProgramUseScope(unsigned int programId, int policy)
{
    if (programId == 0) return false;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    RLSharedGpuGroup::ProgramUseScope *scope = nullptr;
    {
        std::lock_guard<std::mutex> lock(g->m);
        if (g->refs.find(key) == g->refs.end()) return false;
        scope = GetOrCreateProgramUseScopeLocked(g, key);
    }
    if (!scope) return false;

    if (policy == RL_SHARED_SHADER_USE_LOCKED)
    {
        scope->lockMutex.lock();
        return true;
    }

    if (policy == RL_SHARED_SHADER_USE_PHASED)
    {
        std::unique_lock<std::mutex> phaseLock(scope->phaseMutex);
        const uint64_t ticket = scope->nextTicket++;
        while (scope->servingTicket != ticket) scope->phaseCv.wait(phaseLock);
        return true;
    }

    return false;
}

void RLSharedGpuEndProgramUseScope(unsigned int programId, int policy)
{
    if (programId == 0) return;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    RLSharedGpuGroup::ProgramUseScope *scope = nullptr;
    {
        std::lock_guard<std::mutex> lock(g->m);
        auto it = g->programUseScopes.find(key);
        if (it == g->programUseScopes.end()) return;
        scope = it->second.get();
    }
    if (!scope) return;

    if (policy == RL_SHARED_SHADER_USE_LOCKED)
    {
        scope->lockMutex.unlock();
        return;
    }

    if (policy == RL_SHARED_SHADER_USE_PHASED)
    {
        std::lock_guard<std::mutex> phaseLock(scope->phaseMutex);
        scope->servingTicket += 1;
        scope->phaseCv.notify_all();
    }
}

bool RLSharedGpuTakeProgramFence(unsigned int programId, void **fenceOut)
{
    if ((programId == 0) || (fenceOut == nullptr)) return false;
    *fenceOut = nullptr;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->programUseScopes.find(key);
    if (it == g->programUseScopes.end()) return false;

    *fenceOut = it->second->lastFence;
    it->second->lastFence = nullptr;
    return true;
}

bool RLSharedGpuStoreProgramFence(unsigned int programId, void *fence)
{
    if (programId == 0) return false;

    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::lock_guard<std::mutex> lock(g->m);
    auto it = g->programUseScopes.find(key);
    if (it == g->programUseScopes.end()) return false;
    if (it->second->lastFence != nullptr) return false;

    it->second->lastFence = fence;
    return true;
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

    if (t == RL_SHARED_GPU_OBJECT_TEXTURE) {
        std::lock_guard<std::mutex> lock(g->m);
        auto it = g->textureTrace.find(id);
        if (it != g->textureTrace.end()) {
            if (it->second.label.find("font") != std::string::npos) {
                RLTraceLog(RL_E_LOG_DEBUG,
                    "SHARED_GPU: font texture pop delete: id=%u label=%s mark=%s release=%s reason=%s",
                    id,
                    it->second.label.c_str(),
                    it->second.markSource.c_str(),
                    it->second.releaseSource.c_str(),
                    it->second.releaseReason.c_str());
            }
        }
        g->textureTrace.erase(id);
    }

    *typeOut = (RLSharedGpuObjectType)t;
    *idOut = (unsigned int)id;
    return true;
}

void RLSharedGpuDebugDumpState(const char *label)
{
    RLSharedGpuGroup *g = GetGroupForCurrentContext();
    if (!g) {
        RLTraceLog(RL_E_LOG_INFO, "SHARED_GPU: %s: no share-group bound on current context", label ? label : "state");
        return;
    }

    uint32_t liveByType[8] = {0};
    uint32_t pendByType[8] = {0};
    size_t live = 0, pend = 0, untrackedRelease = 0;
    size_t fboMapHit = 0, fboMapMiss = 0, fboReleaseSkipped = 0;
    const unsigned long long rejectRetain = gUnregisteredRetainRejectCount.load(std::memory_order_relaxed);
    const unsigned long long rejectRelease = gUnregisteredReleaseRejectCount.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(g->m);
        live = g->refs.size();
        pend = g->pending.size();
        untrackedRelease = g->releaseUntrackedCount;
        fboMapHit = g->fboAttachmentMapHitCount;
        fboMapMiss = g->fboAttachmentMapMissCount;
        fboReleaseSkipped = g->fboAttachmentReleaseSkippedCount;
        for (auto &kv : g->refs) {
            uint32_t t=0, id=0; SplitKey(kv.first, t, id);
            if (t < 8) liveByType[t] += 1;
        }
        for (auto &k : g->pending) {
            uint32_t t=0, id=0; SplitKey(k, t, id);
            if (t < 8) pendByType[t] += 1;
        }
    }

    RLTraceLog(RL_E_LOG_INFO,
        "SHARED_GPU: %s: live=%zu pending=%zu untrackedRelease=%zu | live(tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u) "
        "pend(tex=%u buf=%u vao=%u fbo=%u rbo=%u prog=%u) fboMap(hit=%zu miss=%zu releaseSkip=%zu) reject(retain=%llu release=%llu)",
        label ? label : "state",
        live, pend, untrackedRelease,
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
        pendByType[RL_SHARED_GPU_OBJECT_PROGRAM],
        fboMapHit, fboMapMiss, fboReleaseSkipped,
        rejectRetain, rejectRelease);
}

void RLSharedGpuSetTextureDebugLabel(unsigned int id, const char *label, const char *sourceFile, int sourceLine)
{
    if (id == 0 || label == nullptr) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;

    std::lock_guard<std::mutex> lock(g->m);
    RLSharedGpuGroup::TextureTrace &trace = g->textureTrace[id];
    trace.label = label;
    trace.markSource = BuildSourceText(sourceFile, sourceLine);
}

void RLSharedGpuTraceTextureRelease(unsigned int id, const char *sourceFile, int sourceLine, const char *reason)
{
    if (id == 0) return;
    RLSharedGpuGroup *g = EnsureGroupForCurrentContext();
    if (!g) return;

    std::lock_guard<std::mutex> lock(g->m);
    RLSharedGpuGroup::TextureTrace &trace = g->textureTrace[id];
    if (trace.label.empty()) trace.label = "texture";
    trace.releaseSource = BuildSourceText(sourceFile, sourceLine);
    trace.releaseReason = (reason != nullptr) ? reason : "(none)";
}

void RLSharedGpuSetTrackingMode(RLSharedGpuTrackingModeInternal mode)
{
    if ((mode != RL_SHARED_GPU_TRACKING_MODE_COMPATIBLE) && (mode != RL_SHARED_GPU_TRACKING_MODE_STRICT))
    {
        RLTraceLog(RL_E_LOG_WARNING, "SHARED_GPU: invalid tracking mode=%d", (int)mode);
        return;
    }

    gTrackingMode.store((int)mode, std::memory_order_relaxed);
    RLTraceLog(RL_E_LOG_INFO, "SHARED_GPU: tracking mode=%s",
               (mode == RL_SHARED_GPU_TRACKING_MODE_STRICT) ? "strict" : "compatible");
}

RLSharedGpuTrackingModeInternal RLSharedGpuGetTrackingMode(void)
{
    return (RLSharedGpuTrackingModeInternal)gTrackingMode.load(std::memory_order_relaxed);
}

RLSharedGpuTrackingDiagStats RLSharedGpuGetTrackingDiagStats(void)
{
    RLSharedGpuTrackingDiagStats out = { 0 };
    out.unregisteredRetainRejectCount = gUnregisteredRetainRejectCount.load(std::memory_order_relaxed);
    out.unregisteredReleaseRejectCount = gUnregisteredReleaseRejectCount.load(std::memory_order_relaxed);
    return out;
}

void RLSharedGpuResetTrackingDiagStats(void)
{
    gUnregisteredRetainRejectCount.store(0, std::memory_order_relaxed);
    gUnregisteredReleaseRejectCount.store(0, std::memory_order_relaxed);
}

} // extern "C"
