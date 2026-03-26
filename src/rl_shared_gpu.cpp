#include "rl_shared_gpu.h"
#include "raylib.h"
#include "rl_object_tracker.h"

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

struct RLTraceCallbackIsolationScope {
    RLTraceCallbackIsolationScope() { RLTraceCallbackIsolationEnter(); }
    ~RLTraceCallbackIsolationScope() { RLTraceCallbackIsolationLeave(); }
};

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

    std::mutex groupStateMutex;
    std::unordered_map<uint64_t, uint32_t> refs;
    std::unordered_map<uint64_t, RLContext*> owners;
    std::unordered_set<uint64_t> orphanedOwners;
    std::deque<uint64_t> pending;
    std::unordered_map<uint64_t, void*> programLocs;
    std::unordered_map<uint32_t, uint64_t> framebufferDepth;  // fboId -> depthKey (type+id)
    std::unordered_map<uint32_t, std::unordered_map<int, uint64_t>> framebufferAttachments; // fboId -> (attachment -> key)
    std::unordered_map<uint32_t, TextureTrace> textureTrace;   // texture id -> trace metadata
    std::unordered_map<uint64_t, std::shared_ptr<ProgramUseScope>> programUseScopes;
    std::deque<void*> pendingProgramFences;
    std::unordered_set<uint64_t> pendingSet; // dedupe
    size_t releaseUntrackedCount = 0;
    size_t fboAttachmentMapHitCount = 0;
    size_t fboAttachmentMapMissCount = 0;
    size_t fboAttachmentReleaseSkippedCount = 0;
    std::atomic<uint32_t> ctxRefs{1};
    std::atomic<int> trackedScopePolicy{RL_SHARED_GPU_TRACKED_SCOPE_CONTEXT};
};

class RLSharedGpuGroupLockScope {
public:
    explicit RLSharedGpuGroupLockScope(RLSharedGpuGroup *shareGroup)
        : callbackIsolation(), shareGroupLock(shareGroup->groupStateMutex) {}

private:
    RLTraceCallbackIsolationScope callbackIsolation;
    std::unique_lock<std::mutex> shareGroupLock;
};

static inline uint64_t MakeKey(uint32_t type, uint32_t id)
{
    return (uint64_t(type) << 32) | uint64_t(id);
}

static std::atomic<int> gTrackingMode((int)RL_SHARED_GPU_TRACKING_MODE_STRICT);
static std::atomic<unsigned long long> gUnregisteredRetainRejectCount(0);
static std::atomic<unsigned long long> gUnregisteredReleaseRejectCount(0);
static std::mutex gSharedGpuBindingMutex;

class RLSharedGpuBindingLockScope {
public:
    RLSharedGpuBindingLockScope()
        : callbackIsolation(), bindingLock(gSharedGpuBindingMutex) {}

private:
    RLTraceCallbackIsolationScope callbackIsolation;
    std::unique_lock<std::mutex> bindingLock;
};

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

static void AddGroupRef(RLSharedGpuGroup *shareGroup)
{
    if (shareGroup == nullptr) return;
    shareGroup->ctxRefs.fetch_add(1, std::memory_order_acq_rel);
}

static void ReleaseGroupRef(RLSharedGpuGroup *shareGroup);

class PinnedGroup
{
public:
    PinnedGroup() : group(nullptr) {}
    explicit PinnedGroup(RLSharedGpuGroup *shareGroup) : group(shareGroup) {}
    ~PinnedGroup() { reset(); }

    PinnedGroup(const PinnedGroup&) = delete;
    PinnedGroup& operator=(const PinnedGroup&) = delete;

    PinnedGroup(PinnedGroup&& other) noexcept : group(other.group)
    {
        other.group = nullptr;
    }

    PinnedGroup& operator=(PinnedGroup&& other) noexcept
    {
        if (this != &other)
        {
            reset();
            group = other.group;
            other.group = nullptr;
        }
        return *this;
    }

    RLSharedGpuGroup *get() const { return group; }
    explicit operator bool() const { return (group != nullptr); }

    void reset()
    {
        if (group != nullptr)
        {
            ReleaseGroupRef(group);
            group = nullptr;
        }
    }

private:
    RLSharedGpuGroup *group;
};

static void DeleteGroup(RLSharedGpuGroup *shareGroup)
{
    if (!shareGroup) return;
    {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);

        // Leak diagnostics (GPU objects are tracked by refcounts; pending holds deferred deletes).
        // We do not call any GL APIs here.
        if (!shareGroup->refs.empty() || !shareGroup->pending.empty()) {
            uint32_t liveByType[8] = {0};
            for (auto &refEntry : shareGroup->refs) {
                uint32_t objectTypeBits = 0, objectId = 0;
                SplitKey(refEntry.first, objectTypeBits, objectId);
                if (objectTypeBits < 8) liveByType[objectTypeBits] += 1;
            }
            uint32_t pendingByType[8] = {0};
            for (auto &pendingKey : shareGroup->pending) {
                uint32_t objectTypeBits = 0, objectId = 0;
                SplitKey(pendingKey, objectTypeBits, objectId);
                if (objectTypeBits < 8) pendingByType[objectTypeBits] += 1;
            }
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: share-group destroyed with live refs/pending deletes: live=%zu pending=%zu untrackedRelease=%zu",
                (size_t)shareGroup->refs.size(), (size_t)shareGroup->pending.size(), shareGroup->releaseUntrackedCount);
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
                pendingByType[RL_SHARED_GPU_OBJECT_TEXTURE],
                pendingByType[RL_SHARED_GPU_OBJECT_BUFFER],
                pendingByType[RL_SHARED_GPU_OBJECT_VERTEX_ARRAY],
                pendingByType[RL_SHARED_GPU_OBJECT_FRAMEBUFFER],
                pendingByType[RL_SHARED_GPU_OBJECT_RENDERBUFFER],
                pendingByType[RL_SHARED_GPU_OBJECT_PROGRAM]);
        }

        for (auto &programLocEntry : shareGroup->programLocs) {
            if (programLocEntry.second) RL_FREE(programLocEntry.second);
        }
        shareGroup->programLocs.clear();
    }
    delete shareGroup;
}

static void ReleaseGroupRef(RLSharedGpuGroup *shareGroup)
{
    if (shareGroup == nullptr) return;
    const uint32_t prev = shareGroup->ctxRefs.fetch_sub(1, std::memory_order_acq_rel);
    if (prev == 1u) DeleteGroup(shareGroup);
}

static PinnedGroup PinExistingGroupForContext(RLContext *ctx)
{
    if (!ctx) return PinnedGroup();

    RLSharedGpuBindingLockScope bindingLock;
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
    if (shareGroup != nullptr) AddGroupRef(shareGroup);
    return PinnedGroup(shareGroup);
}

static RLContext *GetCurrentContextSafe()
{
    // RLGetCurrentContext is part of the public API and safe to call here.
    return RLGetCurrentContext();
}

static PinnedGroup EnsurePinnedGroupForContext(RLContext *ctx)
{
    if (!ctx) return PinnedGroup();

    RLSharedGpuBindingLockScope bindingLock;
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
    if (shareGroup == nullptr)
    {
        shareGroup = NewGroup();         // Initial binding ref.
        ctx->gpuShareGroup = (void *)shareGroup;
    }

    AddGroupRef(shareGroup);             // Temporary pin for caller use.
    return PinnedGroup(shareGroup);
}

static PinnedGroup EnsurePinnedGroupForCurrentContext()
{
    RLContext *ctx = GetCurrentContextSafe();
    return EnsurePinnedGroupForContext(ctx);
}

static PinnedGroup PinExistingGroupForCurrentContext()
{
    RLContext *ctx = GetCurrentContextSafe();
    return PinExistingGroupForContext(ctx);
}

static inline bool GroupUsesSharedTrackedScope(const RLSharedGpuGroup *shareGroup)
{
    if (shareGroup == nullptr) return false;
    return (shareGroup->trackedScopePolicy.load(std::memory_order_acquire) == (int)RL_SHARED_GPU_TRACKED_SCOPE_SHARE_GROUP);
}

static void PushPendingDelete(RLSharedGpuGroup *shareGroup, uint64_t key)
{
    if (!shareGroup) return;
    if (shareGroup->pendingSet.insert(key).second) {
        uint32_t objectTypeBits = 0, objectId = 0;
        SplitKey(key, objectTypeBits, objectId);
        if (objectTypeBits == RL_SHARED_GPU_OBJECT_TEXTURE) {
            auto traceIt = shareGroup->textureTrace.find(objectId);
            if (traceIt != shareGroup->textureTrace.end()) {
                if (traceIt->second.label.find("font") != std::string::npos) {
                    RLTraceLog(RL_E_LOG_DEBUG,
                        "SHARED_GPU: font texture enqueue delete: id=%u label=%s mark=%s release=%s reason=%s",
                        objectId,
                        traceIt->second.label.c_str(),
                        traceIt->second.markSource.c_str(),
                        traceIt->second.releaseSource.c_str(),
                        traceIt->second.releaseReason.c_str());
                }
            }
        }
        shareGroup->pending.push_back(key);
    }
}

static void NoteUntrackedReleaseLocked(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, uint64_t key)
{
    if (!shareGroup || key == 0) return;
    shareGroup->releaseUntrackedCount += 1;
    uint32_t keyType = 0, keyId = 0;
    SplitKey(key, keyType, keyId);
    RLTraceLog(RL_E_LOG_WARNING,
        "SHARED_GPU: release on untracked object ignored (requestedType=%u keyType=%u id=%u untrackedCount=%zu)",
        (unsigned)type, (unsigned)keyType, (unsigned)keyId, shareGroup->releaseUntrackedCount);
}

static void RegisterObjectInGroup(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, unsigned int id)
{
    if (!shareGroup || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    RLContext *currentCtx = GetCurrentContextSafe();

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end())
    {
        shareGroup->refs.emplace(key, 1);
        if (currentCtx != nullptr)
        {
            shareGroup->owners[key] = currentCtx;
            shareGroup->orphanedOwners.erase(key);
        }
    }
    else
    {
        refIt->second += 1;
        // If owner is still unknown for an existing tracked object, lock it now to
        // the registering context. Retain/release paths must not implicitly change owner.
        auto ownerIt = shareGroup->owners.find(key);
        if ((ownerIt == shareGroup->owners.end()) || (ownerIt->second == nullptr))
        {
            if (currentCtx != nullptr)
            {
                shareGroup->owners[key] = currentCtx;
                shareGroup->orphanedOwners.erase(key);
            }
        }
    }
}

static unsigned int OrphanOwnersForContextLocked(RLSharedGpuGroup *shareGroup, RLContext *ctx)
{
    if ((shareGroup == nullptr) || (ctx == nullptr)) return 0u;

    unsigned int orphanedCount = 0u;
    for (auto &ownerEntry : shareGroup->owners)
    {
        if (ownerEntry.second != ctx) continue;
        ownerEntry.second = nullptr;
        shareGroup->orphanedOwners.insert(ownerEntry.first);
        orphanedCount++;
    }

    return orphanedCount;
}

static std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> GetOrCreateProgramUseScopeLocked(RLSharedGpuGroup *shareGroup, uint64_t key)
{
    if (!shareGroup || key == 0) return std::shared_ptr<RLSharedGpuGroup::ProgramUseScope>();
    auto scopeIt = shareGroup->programUseScopes.find(key);
    if (scopeIt != shareGroup->programUseScopes.end()) return scopeIt->second;

    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope(new RLSharedGpuGroup::ProgramUseScope());
    shareGroup->programUseScopes.emplace(key, scope);
    return scope;
}

static void QueueProgramFenceForDeleteLocked(RLSharedGpuGroup *shareGroup, void *fence)
{
    if ((shareGroup == nullptr) || (fence == nullptr)) return;
    shareGroup->pendingProgramFences.push_back(fence);
}

static void RemoveProgramScopeLocked(RLSharedGpuGroup *shareGroup, uint64_t key)
{
    if ((shareGroup == nullptr) || (key == 0)) return;

    auto scopeIt = shareGroup->programUseScopes.find(key);
    if (scopeIt == shareGroup->programUseScopes.end()) return;

    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope = scopeIt->second;
    shareGroup->programUseScopes.erase(scopeIt);
    if ((scope != nullptr) && (scope->lastFence != nullptr))
    {
        QueueProgramFenceForDeleteLocked(shareGroup, scope->lastFence);
        scope->lastFence = nullptr;
    }
}

static void RetainObjectInGroup(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, unsigned int id)
{
    if (!shareGroup || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end())
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
        shareGroup->refs.emplace(key, 2);
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: retain on unregistered object accepted in compatibility mode (type=%u id=%u refs=2)",
            (unsigned)type, id);
    }
    else
    {
        refIt->second += 1;
    }
}

static void RetainKeyLocked(RLSharedGpuGroup *shareGroup, uint64_t key)
{
    if (!shareGroup || key == 0) return;
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end())
    {
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredRetainRejectCount.fetch_add(1, std::memory_order_relaxed);
            uint32_t objectTypeBits = 0, objectId = 0;
            SplitKey(key, objectTypeBits, objectId);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: retain on unregistered object rejected (type=%u id=%u)",
                (unsigned)objectTypeBits, objectId);
            return;
        }

        // Compatibility mode: assume an implicit owner reference already exists.
        shareGroup->refs.emplace(key, 2);
        uint32_t objectTypeBits = 0, objectId = 0;
        SplitKey(key, objectTypeBits, objectId);
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: retain on unregistered object accepted in compatibility mode (type=%u id=%u refs=2)",
            (unsigned)objectTypeBits, objectId);
    }
    else
    {
        refIt->second += 1;
    }
}

static void ReleaseKeyLocked(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, uint64_t key)
{
    if (!shareGroup || key == 0) return;
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end()) {
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
        NoteUntrackedReleaseLocked(shareGroup, type, key);
        return;
    }

    if (refIt->second <= 1) {
        shareGroup->refs.erase(refIt);
        shareGroup->owners.erase(key);
        shareGroup->orphanedOwners.erase(key);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto programLocIt = shareGroup->programLocs.find(key);
            if (programLocIt != shareGroup->programLocs.end()) {
                if (programLocIt->second) RL_FREE(programLocIt->second);
                shareGroup->programLocs.erase(programLocIt);
            }
            RemoveProgramScopeLocked(shareGroup, key);
        }
        PushPendingDelete(shareGroup, key);
        return;
    }

    refIt->second -= 1;
}

static void ReleaseObjectInGroup(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, unsigned int id)
{
    if (!shareGroup || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end()) {
        if (IsStrictTrackingEnabled())
        {
            gUnregisteredReleaseRejectCount.fetch_add(1, std::memory_order_relaxed);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: release on unregistered object rejected (type=%u id=%u)",
                (unsigned)type, id);
            return;
        }

        // Compatibility mode: ignore release in this group to avoid accidental cross-group deletes.
        NoteUntrackedReleaseLocked(shareGroup, type, key);
        return;
    }

    if (refIt->second <= 1) {
        shareGroup->refs.erase(refIt);
        shareGroup->owners.erase(key);
        shareGroup->orphanedOwners.erase(key);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            auto programLocIt = shareGroup->programLocs.find(key);
            if (programLocIt != shareGroup->programLocs.end()) {
                if (programLocIt->second) RL_FREE(programLocIt->second);
                shareGroup->programLocs.erase(programLocIt);
            }
            RemoveProgramScopeLocked(shareGroup, key);
        }
        PushPendingDelete(shareGroup, key);
        return;
    }

    refIt->second -= 1;
}

static void RegisterFramebufferAttachmentInGroup(RLSharedGpuGroup *shareGroup, unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    if (!shareGroup || framebufferId == 0 || objId == 0) return;
    if (type != RL_SHARED_GPU_OBJECT_TEXTURE && type != RL_SHARED_GPU_OBJECT_RENDERBUFFER) return;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)objId);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    shareGroup->framebufferAttachments[(uint32_t)framebufferId][attachment] = key;

    // Keep legacy depth mapping in sync for existing query path.
    if (attachment == 100) shareGroup->framebufferDepth[(uint32_t)framebufferId] = key;
}

static void UnregisterFramebufferAttachmentInGroup(RLSharedGpuGroup *shareGroup, unsigned int framebufferId, int attachment)
{
    if (!shareGroup || framebufferId == 0) return;
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);

    auto framebufferIt = shareGroup->framebufferAttachments.find((uint32_t)framebufferId);
    if (framebufferIt != shareGroup->framebufferAttachments.end())
    {
        framebufferIt->second.erase(attachment);
        if (framebufferIt->second.empty()) shareGroup->framebufferAttachments.erase(framebufferIt);
    }

    // Keep legacy depth mapping in sync.
    if (attachment == 100) shareGroup->framebufferDepth.erase((uint32_t)framebufferId);
}

static void UnregisterFramebufferAttachmentsInGroup(RLSharedGpuGroup *shareGroup, unsigned int framebufferId)
{
    if (!shareGroup || framebufferId == 0) return;
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    shareGroup->framebufferAttachments.erase((uint32_t)framebufferId);
    shareGroup->framebufferDepth.erase((uint32_t)framebufferId);
}

} // namespace

extern "C" {

bool RLSharedGpuHasCurrentGroup(void)
{
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    return (bool)pinned;
}

bool RLSharedGpuHasContextGroup(RLContext *ctx)
{
    PinnedGroup pinned = PinExistingGroupForContext(ctx);
    return (bool)pinned;
}

bool RLSharedGpuContextUsesSharedTrackedScope(RLContext *ctx)
{
    PinnedGroup pinned = PinExistingGroupForContext(ctx);
    return GroupUsesSharedTrackedScope(pinned.get());
}

bool RLSharedGpuContextResolveTrackedScopeHandle(RLContext *ctx, void **groupHandleOut)
{
    if (groupHandleOut != nullptr) *groupHandleOut = nullptr;
    if ((ctx == nullptr) || (groupHandleOut == nullptr)) return false;

    RLSharedGpuBindingLockScope bindingLock;
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
    if ((shareGroup == nullptr) || !GroupUsesSharedTrackedScope(shareGroup)) return false;

    *groupHandleOut = (void *)shareGroup;
    return true;
}

bool RLSharedGpuGroupUsesSharedTrackedScope(const void *groupHandle)
{
    return GroupUsesSharedTrackedScope((const RLSharedGpuGroup *)groupHandle);
}

void RLSharedGpuGroupSetSharedTrackedScope(void *groupHandle)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if (shareGroup == nullptr) return;
    shareGroup->trackedScopePolicy.store((int)RL_SHARED_GPU_TRACKED_SCOPE_SHARE_GROUP, std::memory_order_release);
}

bool RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx)
{
    if (!ctx) return false;

    PinnedGroup desiredPinned;
    if (shareWithCtx) desiredPinned = PinExistingGroupForContext(shareWithCtx);
    RLSharedGpuGroup *desired = desiredPinned.get();
    if ((shareWithCtx != nullptr) && (desired == nullptr))
    {
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: share-group bind failed: target context has no active group");
        return false;
    }

    if (desired != nullptr)
    {
        RLTrackedPromotionResult targetPromotionResult = RLTrackedObjectPromoteContextEntriesToShareGroup(shareWithCtx, (void *)desired);
        if (targetPromotionResult == RL_TRACKED_PROMOTION_FAILED)
        {
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: share-group bind failed: tracked promotion aborted for target context");
            return false;
        }
    }

    RLSharedGpuGroup *current = nullptr;
    {
        RLSharedGpuBindingLockScope bindingLock;
        current = (RLSharedGpuGroup *)ctx->gpuShareGroup;

        if (desired == nullptr)
        {
            if (current == nullptr) ctx->gpuShareGroup = (void *)NewGroup();
            return true;
        }

        if (current == desired) return true;

        if ((current != nullptr) && (current != desired))
        {
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: share-group rebind failed: rebinding a live context to a different share-group is unsupported");
            return false;
        }

        AddGroupRef(desired);                   // New binding ref for ctx.
        ctx->gpuShareGroup = (void *)desired;
    }

    if (GroupUsesSharedTrackedScope(desired))
    {
        RLTrackedPromotionResult contextPromotionResult = RLTrackedObjectPromoteContextEntriesToShareGroup(ctx, (void *)desired);
        if (contextPromotionResult == RL_TRACKED_PROMOTION_FAILED)
        {
            bool removedBinding = false;
            {
                RLSharedGpuBindingLockScope bindingLock;
                if ((RLSharedGpuGroup *)ctx->gpuShareGroup == desired)
                {
                    ctx->gpuShareGroup = nullptr;
                    removedBinding = true;
                }
            }

            if (removedBinding) ReleaseGroupRef(desired);

            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: share-group bind failed: tracked promotion aborted for source context");
            return false;
        }
    }

    return true;
}

void RLSharedGpuContextUnbindShareGroup(RLContext *ctx)
{
    if (!ctx) return;
    RLSharedGpuGroup *shareGroup = nullptr;
    PinnedGroup pinned;

    {
        RLSharedGpuBindingLockScope bindingLock;
        shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
        if (!shareGroup) return;

        AddGroupRef(shareGroup);                // Pin group during orphaning and binding release.
        pinned = PinnedGroup(shareGroup);
        ctx->gpuShareGroup = nullptr;
    }

    if (!shareGroup) return;

    unsigned int orphanedCount = 0u;
    {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        orphanedCount = OrphanOwnersForContextLocked(shareGroup, ctx);
    }
    if (orphanedCount > 0u)
    {
        RLTraceLog(RL_E_LOG_WARNING,
            "SHARED_GPU: context unbind orphaned %u shared object owners; adopt explicitly if still needed",
            orphanedCount);
    }

    ReleaseGroupRef(shareGroup);               // Drop ctx binding ref. Temporary pin drops on scope exit.
}

void RLSharedGpuRegisterObject(RLSharedGpuObjectType type, unsigned int id)
{
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RegisterObjectInGroup(pinned.get(), type, id);
}

void RLSharedGpuRegisterProgramLocs(unsigned int programId, int *locs)
{
    if (programId == 0 || locs == nullptr) return;
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (shareGroup == nullptr) return;
    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto programLocIt = shareGroup->programLocs.find(key);
    if (programLocIt == shareGroup->programLocs.end()) {
        shareGroup->programLocs.emplace(key, (void*)locs);
    } else {
        // Should not happen; keep existing pointer, free the new one to avoid leaks.
        RL_FREE(locs);
    }
}

void RLSharedGpuRegisterFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType type, unsigned int objId)
{
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RegisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, 100, type, objId);
}

void RLSharedGpuUnregisterFramebufferDepth(unsigned int framebufferId)
{
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    UnregisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, 100);
}

bool RLSharedGpuQueryFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType *typeOut, unsigned int *objIdOut)
{
    if (framebufferId == 0 || !typeOut || !objIdOut) return false;
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto depthIt = shareGroup->framebufferDepth.find((uint32_t)framebufferId);
    if (depthIt == shareGroup->framebufferDepth.end()) return false;

    uint32_t objectTypeBits = 0, objectId = 0;
    SplitKey(depthIt->second, objectTypeBits, objectId);
    *typeOut = (RLSharedGpuObjectType)objectTypeBits;
    *objIdOut = (unsigned int)objectId;
    return true;
}

void RLSharedGpuRetainFramebufferTree(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return;
    const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    RetainKeyLocked(shareGroup, fboKey);
    auto framebufferIt = shareGroup->framebufferAttachments.find((uint32_t)framebufferId);
    if (framebufferIt != shareGroup->framebufferAttachments.end())
    {
        shareGroup->fboAttachmentMapHitCount += 1;
        std::unordered_set<uint64_t> dedup;
        for (const auto &attachmentEntry : framebufferIt->second)
        {
            if (dedup.insert(attachmentEntry.second).second) RetainKeyLocked(shareGroup, attachmentEntry.second);
        }
    }
    else
    {
        shareGroup->fboAttachmentMapMissCount += 1;
        // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
        auto depthIt = shareGroup->framebufferDepth.find((uint32_t)framebufferId);
        if (depthIt != shareGroup->framebufferDepth.end()) RetainKeyLocked(shareGroup, depthIt->second);
    }
}

void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId)
{
    if (framebufferId == 0) return;
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return;
    const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto framebufferIt = shareGroup->framebufferAttachments.find((uint32_t)framebufferId);
    if (framebufferIt != shareGroup->framebufferAttachments.end())
    {
        shareGroup->fboAttachmentMapHitCount += 1;
        std::unordered_set<uint64_t> dedup;
        for (const auto &attachmentEntry : framebufferIt->second)
        {
            if (!dedup.insert(attachmentEntry.second).second) continue;
            if (shareGroup->refs.find(attachmentEntry.second) == shareGroup->refs.end()) shareGroup->fboAttachmentReleaseSkippedCount += 1;
            uint32_t objectTypeBits = 0, objectId = 0;
            SplitKey(attachmentEntry.second, objectTypeBits, objectId);
            ReleaseKeyLocked(shareGroup, (RLSharedGpuObjectType)objectTypeBits, attachmentEntry.second);
        }
    }
    else
    {
        shareGroup->fboAttachmentMapMissCount += 1;
        // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
        auto depthIt = shareGroup->framebufferDepth.find((uint32_t)framebufferId);
        if (depthIt != shareGroup->framebufferDepth.end())
        {
            if (shareGroup->refs.find(depthIt->second) == shareGroup->refs.end()) shareGroup->fboAttachmentReleaseSkippedCount += 1;
            uint32_t objectTypeBits = 0, objectId = 0;
            SplitKey(depthIt->second, objectTypeBits, objectId);
            ReleaseKeyLocked(shareGroup, (RLSharedGpuObjectType)objectTypeBits, depthIt->second);
        }
    }
    ReleaseKeyLocked(shareGroup, RL_SHARED_GPU_OBJECT_FRAMEBUFFER, fboKey);
    // If framebuffer is no longer tracked, drop its attachment mapping to avoid staleness.
    if (shareGroup->refs.find(fboKey) == shareGroup->refs.end()) {
        shareGroup->framebufferAttachments.erase((uint32_t)framebufferId);
        shareGroup->framebufferDepth.erase((uint32_t)framebufferId);
    }
}

RLSharedGpuFramebufferMapStats RLSharedGpuGetFramebufferMapStats(void)
{
    RLSharedGpuFramebufferMapStats out = { 0 };
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return out;

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    out.mapHitCount = (unsigned long long)shareGroup->fboAttachmentMapHitCount;
    out.mapMissCount = (unsigned long long)shareGroup->fboAttachmentMapMissCount;
    out.releaseSkippedCount = (unsigned long long)shareGroup->fboAttachmentReleaseSkippedCount;
    return out;
}

bool RLSharedGpuGetObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext **ownerOut)
{
    if ((id == 0) || (ownerOut == nullptr)) return false;
    *ownerOut = nullptr;

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    if (shareGroup->refs.find(key) == shareGroup->refs.end()) return false;

    auto ownerIt = shareGroup->owners.find(key);
    if (ownerIt == shareGroup->owners.end()) return false;
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

    PinnedGroup currentPinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = currentPinned.get();
    if (!shareGroup) return false;
    PinnedGroup targetPinned = PinExistingGroupForContext(targetCtx);
    if (targetPinned.get() != shareGroup) return false;

    RLContext *currentCtx = GetCurrentContextSafe();
    if (currentCtx == nullptr) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    if (shareGroup->refs.find(key) == shareGroup->refs.end()) return false;

    auto ownerIt = shareGroup->owners.find(key);
    if ((ownerIt != shareGroup->owners.end()) && (ownerIt->second == nullptr) &&
        (shareGroup->orphanedOwners.find(key) != shareGroup->orphanedOwners.end())) return false;
    if ((ownerIt != shareGroup->owners.end()) && (ownerIt->second != nullptr) && (ownerIt->second != currentCtx)) return false;

    shareGroup->owners[key] = targetCtx;
    shareGroup->orphanedOwners.erase(key);
    return true;
}

bool RLSharedGpuTryAdoptOrphanedObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    if ((id == 0) || (targetCtx == nullptr)) return false;

    PinnedGroup currentPinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = currentPinned.get();
    if (!shareGroup) return false;
    PinnedGroup targetPinned = PinExistingGroupForContext(targetCtx);
    if (targetPinned.get() != shareGroup) return false;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    if (shareGroup->refs.find(key) == shareGroup->refs.end()) return false;

    auto ownerIt = shareGroup->owners.find(key);
    if (ownerIt == shareGroup->owners.end()) return false;
    if (ownerIt->second != nullptr) return false;
    if (shareGroup->orphanedOwners.find(key) == shareGroup->orphanedOwners.end()) return false;

    ownerIt->second = targetCtx;
    shareGroup->orphanedOwners.erase(key);
    return true;
}

bool RLSharedGpuLockOwnerGroup(RLContext *currentCtx, RLContext *targetCtx, void **groupHandleOut)
{
    if (groupHandleOut != nullptr) *groupHandleOut = nullptr;
    if ((currentCtx == nullptr) || (targetCtx == nullptr) || (groupHandleOut == nullptr)) return false;

    PinnedGroup currentPinned = PinExistingGroupForContext(currentCtx);
    PinnedGroup targetPinned = PinExistingGroupForContext(targetCtx);
    RLSharedGpuGroup *shareGroup = currentPinned.get();
    if ((shareGroup == nullptr) || (targetPinned.get() != shareGroup)) return false;

    AddGroupRef(shareGroup);
    shareGroup->groupStateMutex.lock();
    RLTraceCallbackIsolationEnter();
    *groupHandleOut = (void *)shareGroup;
    return true;
}

void RLSharedGpuUnlockOwnerGroup(void *groupHandle)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if (shareGroup == nullptr) return;
    shareGroup->groupStateMutex.unlock();
    RLTraceCallbackIsolationLeave();
    ReleaseGroupRef(shareGroup);
}

int RLSharedGpuCanTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *currentCtx, RLContext *targetCtx)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if ((shareGroup == nullptr) || (id == 0) || (currentCtx == nullptr) || (targetCtx == nullptr)) return -1;

    const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
    if (shareGroup->refs.find(objectKey) == shareGroup->refs.end()) return 0;

    auto ownerIt = shareGroup->owners.find(objectKey);
    if ((ownerIt != shareGroup->owners.end()) && (ownerIt->second == nullptr) &&
        (shareGroup->orphanedOwners.find(objectKey) != shareGroup->orphanedOwners.end())) return -1;
    if ((ownerIt != shareGroup->owners.end()) && (ownerIt->second == targetCtx)) return 2;
    if ((ownerIt != shareGroup->owners.end()) && (ownerIt->second != nullptr) && (ownerIt->second != currentCtx)) return -1;

    return 1;
}

void RLSharedGpuTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return;

    const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
    shareGroup->owners[objectKey] = targetCtx;
    shareGroup->orphanedOwners.erase(objectKey);
}

int RLSharedGpuCanAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return -1;

    const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
    if (shareGroup->refs.find(objectKey) == shareGroup->refs.end()) return 0;

    auto ownerIt = shareGroup->owners.find(objectKey);
    if (ownerIt == shareGroup->owners.end()) return 0;
    if ((ownerIt->second == targetCtx) && (shareGroup->orphanedOwners.find(objectKey) == shareGroup->orphanedOwners.end())) return 2;
    if ((ownerIt->second != nullptr) || (shareGroup->orphanedOwners.find(objectKey) == shareGroup->orphanedOwners.end())) return -1;

    return 1;
}

void RLSharedGpuAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
    if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return;

    const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
    shareGroup->owners[objectKey] = targetCtx;
    shareGroup->orphanedOwners.erase(objectKey);
}

void RLSharedGpuRegisterFramebufferAttachment(unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RegisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, attachment, type, objId);
}

void RLSharedGpuUnregisterFramebufferAttachment(unsigned int framebufferId, int attachment)
{
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    UnregisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, attachment);
}

void RLSharedGpuUnregisterFramebufferAttachments(unsigned int framebufferId)
{
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    UnregisterFramebufferAttachmentsInGroup(pinned.get(), framebufferId);
}

bool RLSharedGpuBeginProgramUseScope(unsigned int programId, int policy)
{
    if (programId == 0) return false;

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
    {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        if (shareGroup->refs.find(key) == shareGroup->refs.end()) return false;
        scope = GetOrCreateProgramUseScopeLocked(shareGroup, key);
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

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
    {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        auto scopeIt = shareGroup->programUseScopes.find(key);
        if (scopeIt == shareGroup->programUseScopes.end()) return;
        scope = scopeIt->second;
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

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto scopeIt = shareGroup->programUseScopes.find(key);
    if (scopeIt == shareGroup->programUseScopes.end()) return false;
    scope = scopeIt->second;
    if (!scope) return false;

    *fenceOut = scope->lastFence;
    scope->lastFence = nullptr;
    return true;
}

bool RLSharedGpuStoreProgramFence(unsigned int programId, void *fence)
{
    if (programId == 0) return false;

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    auto scopeIt = shareGroup->programUseScopes.find(key);
    if (scopeIt == shareGroup->programUseScopes.end()) return false;
    scope = scopeIt->second;
    if (!scope) return false;
    if (scope->lastFence != nullptr) return false;

    scope->lastFence = fence;
    return true;
}

bool RLSharedGpuPopPendingProgramFence(void **fenceOut)
{
    if (fenceOut == nullptr) return false;
    *fenceOut = nullptr;

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    if (shareGroup->pendingProgramFences.empty()) return false;

    *fenceOut = shareGroup->pendingProgramFences.front();
    shareGroup->pendingProgramFences.pop_front();
    return (*fenceOut != nullptr);
}


void RLSharedGpuRetainObject(RLSharedGpuObjectType type, unsigned int id)
{
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RetainObjectInGroup(pinned.get(), type, id);
}

void RLSharedGpuReleaseObject(RLSharedGpuObjectType type, unsigned int id)
{
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    ReleaseObjectInGroup(pinned.get(), type, id);
}

void RLSharedGpuRetainObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    PinnedGroup pinned = EnsurePinnedGroupForContext(ctx);
    RetainObjectInGroup(pinned.get(), type, id);
}

void RLSharedGpuReleaseObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    PinnedGroup pinned = EnsurePinnedGroupForContext(ctx);
    ReleaseObjectInGroup(pinned.get(), type, id);
}

bool RLSharedGpuPopPendingDelete(RLSharedGpuObjectType *typeOut, unsigned int *idOut)
{
    if (!typeOut || !idOut) return false;

    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return false;

    uint64_t key = 0;
    {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        if (shareGroup->pending.empty()) return false;
        key = shareGroup->pending.front();
        shareGroup->pending.pop_front();
        shareGroup->pendingSet.erase(key);
    }

    uint32_t objectTypeBits = 0, objectId = 0;
    SplitKey(key, objectTypeBits, objectId);

    if (objectTypeBits == RL_SHARED_GPU_OBJECT_TEXTURE) {
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        auto traceIt = shareGroup->textureTrace.find(objectId);
        if (traceIt != shareGroup->textureTrace.end()) {
            if (traceIt->second.label.find("font") != std::string::npos) {
                RLTraceLog(RL_E_LOG_DEBUG,
                    "SHARED_GPU: font texture pop delete: id=%u label=%s mark=%s release=%s reason=%s",
                    objectId,
                    traceIt->second.label.c_str(),
                    traceIt->second.markSource.c_str(),
                    traceIt->second.releaseSource.c_str(),
                    traceIt->second.releaseReason.c_str());
            }
        }
        shareGroup->textureTrace.erase(objectId);
    }

    *typeOut = (RLSharedGpuObjectType)objectTypeBits;
    *idOut = (unsigned int)objectId;
    return true;
}

void RLSharedGpuDebugDumpState(const char *label)
{
    PinnedGroup pinned = PinExistingGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) {
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
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        live = shareGroup->refs.size();
        pend = shareGroup->pending.size();
        untrackedRelease = shareGroup->releaseUntrackedCount;
        fboMapHit = shareGroup->fboAttachmentMapHitCount;
        fboMapMiss = shareGroup->fboAttachmentMapMissCount;
        fboReleaseSkipped = shareGroup->fboAttachmentReleaseSkippedCount;
        for (auto &refEntry : shareGroup->refs) {
            uint32_t objectTypeBits = 0, objectId = 0;
            SplitKey(refEntry.first, objectTypeBits, objectId);
            if (objectTypeBits < 8) liveByType[objectTypeBits] += 1;
        }
        for (auto &pendingKey : shareGroup->pending) {
            uint32_t objectTypeBits = 0, objectId = 0;
            SplitKey(pendingKey, objectTypeBits, objectId);
            if (objectTypeBits < 8) pendByType[objectTypeBits] += 1;
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
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return;

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    RLSharedGpuGroup::TextureTrace &trace = shareGroup->textureTrace[id];
    trace.label = label;
    trace.markSource = BuildSourceText(sourceFile, sourceLine);
}

void RLSharedGpuTraceTextureRelease(unsigned int id, const char *sourceFile, int sourceLine, const char *reason)
{
    if (id == 0) return;
    PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
    RLSharedGpuGroup *shareGroup = pinned.get();
    if (!shareGroup) return;

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    RLSharedGpuGroup::TextureTrace &trace = shareGroup->textureTrace[id];
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
