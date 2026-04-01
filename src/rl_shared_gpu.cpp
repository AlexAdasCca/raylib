#include "rl_shared_gpu.h"
#include "config.h"
#include "raylib.h"
#include "rl_object_tracker.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// NOTE: We keep this module independent of OpenGL headers.
//       Actual glDelete* calls are performed by rlgl (draining pending deletes).

namespace {

struct RLTraceCallbackIsolationScope {
    RLTraceCallbackIsolationScope() { RLTraceCallbackIsolationEnter(); }
    ~RLTraceCallbackIsolationScope() { RLTraceCallbackIsolationLeave(); }
};

static void RLSharedGpuLogFailure(const char *operationName, const char *failureKind, const char *detail = nullptr) noexcept
{
    RLTraceCallbackIsolationScope callbackIsolation;
    const char *safeOperationName = (operationName != nullptr) ? operationName : "operation";
    const char *safeFailureKind = (failureKind != nullptr) ? failureKind : "failure";
    if ((detail != nullptr) && (detail[0] != '\0'))
    {
        RLTraceLog(RL_E_LOG_WARNING, "SHARED_GPU: %s failed: %s (%s)", safeOperationName, safeFailureKind, detail);
    }
    else
    {
        RLTraceLog(RL_E_LOG_WARNING, "SHARED_GPU: %s failed: %s", safeOperationName, safeFailureKind);
    }
}

template <typename T, typename Fn>
static T RLSharedGpuRunNoexceptValue(const char *operationName, T fallbackValue, Fn&& fn) noexcept
{
    try
    {
        return fn();
    }
    catch (const std::bad_alloc&)
    {
        RLSharedGpuLogFailure(operationName, "out of memory");
    }
    catch (const std::exception& ex)
    {
        RLSharedGpuLogFailure(operationName, "exception", ex.what());
    }
    catch (...)
    {
        RLSharedGpuLogFailure(operationName, "unknown exception");
    }

    return fallbackValue;
}

template <typename Fn>
static void RLSharedGpuRunNoexceptVoid(const char *operationName, Fn&& fn) noexcept
{
    try
    {
        fn();
    }
    catch (const std::bad_alloc&)
    {
        RLSharedGpuLogFailure(operationName, "out of memory");
    }
    catch (const std::exception& ex)
    {
        RLSharedGpuLogFailure(operationName, "exception", ex.what());
    }
    catch (...)
    {
        RLSharedGpuLogFailure(operationName, "unknown exception");
    }
}

#define RL_SHARED_GPU_TRY_BOOL(operationName, fallbackValue) \
    return RLSharedGpuRunNoexceptValue<bool>(operationName, fallbackValue, [&]() -> bool {

#define RL_SHARED_GPU_TRY_VALUE(typeName, operationName, fallbackValue) \
    return RLSharedGpuRunNoexceptValue<typeName>(operationName, fallbackValue, [&]() -> typeName {

#define RL_SHARED_GPU_TRY_VOID(operationName) \
    RLSharedGpuRunNoexceptVoid(operationName, [&]() {

#define RL_SHARED_GPU_TRY_END() \
    });

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
    uint64_t releaseUntrackedCount = 0;
    uint64_t fboAttachmentMapHitCount = 0;
    uint64_t fboAttachmentMapMissCount = 0;
    uint64_t fboAttachmentReleaseSkippedCount = 0;
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
static std::atomic<int> gDiagStatsEnabled(1);
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

static inline bool IsSharedGpuCumulativeDiagStatsEnabled(void)
{
#if RL_SHARED_GPU_DIAG_STATS
    return (gDiagStatsEnabled.load(std::memory_order_relaxed) != 0);
#else
    return false;
#endif
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
    RLSharedGpuGroup *group = new (std::nothrow) RLSharedGpuGroup();
    if (group == nullptr)
    {
        RLTraceLog(RL_E_LOG_WARNING, "SHARED_GPU: Failed to allocate share-group");
    }
    return group;
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

class RLSharedGpuOwnerGroupAcquireGuard
{
public:
    RLSharedGpuOwnerGroupAcquireGuard() = default;
    ~RLSharedGpuOwnerGroupAcquireGuard()
    {
        if (isolationEntered) RLTraceCallbackIsolationLeave();
        if (mutexHeld && (shareGroup != nullptr)) shareGroup->groupStateMutex.unlock();
        if (refHeld && (shareGroup != nullptr)) ReleaseGroupRef(shareGroup);
    }

    void SetGroup(RLSharedGpuGroup *group) { shareGroup = group; }

    void AcquireRef()
    {
        if ((shareGroup == nullptr) || refHeld) return;
        AddGroupRef(shareGroup);
        refHeld = true;
    }

    void LockMutex()
    {
        if ((shareGroup == nullptr) || mutexHeld) return;
        shareGroup->groupStateMutex.lock();
        mutexHeld = true;
    }

    void EnterIsolation()
    {
        if (isolationEntered) return;
        RLTraceCallbackIsolationEnter();
        isolationEntered = true;
    }

    void Disarm()
    {
        isolationEntered = false;
        mutexHeld = false;
        refHeld = false;
        shareGroup = nullptr;
    }

private:
    RLSharedGpuGroup *shareGroup = nullptr;
    bool refHeld = false;
    bool mutexHeld = false;
    bool isolationEntered = false;
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
        if (shareGroup == nullptr) return PinnedGroup();
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

static bool PushPendingDeleteLocked(RLSharedGpuGroup *shareGroup, uint64_t key)
{
    if ((shareGroup == nullptr) || (key == 0)) return false;
    if (shareGroup->pendingSet.find(key) != shareGroup->pendingSet.end()) return true;

    shareGroup->pending.push_back(key);
    try
    {
        shareGroup->pendingSet.insert(key);
    }
    catch (...)
    {
        shareGroup->pending.pop_back();
        throw;
    }

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

    return true;
}

static void NoteUntrackedReleaseLocked(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, uint64_t key)
{
    if (!shareGroup || key == 0) return;
    if (IsSharedGpuCumulativeDiagStatsEnabled()) shareGroup->releaseUntrackedCount += 1;
    uint32_t keyType = 0, keyId = 0;
    SplitKey(key, keyType, keyId);
    RLTraceLog(RL_E_LOG_WARNING,
        "SHARED_GPU: release on untracked object ignored (requestedType=%u keyType=%u id=%u untrackedCount=%llu)",
        (unsigned)type, (unsigned)keyType, (unsigned)keyId, (unsigned long long)shareGroup->releaseUntrackedCount);
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
        if (currentCtx != nullptr)
        {
            shareGroup->owners.emplace(key, currentCtx);
            try
            {
                shareGroup->refs.emplace(key, 1);
            }
            catch (...)
            {
                shareGroup->owners.erase(key);
                throw;
            }
            shareGroup->orphanedOwners.erase(key);
        }
        else shareGroup->refs.emplace(key, 1);
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

    std::vector<uint64_t> ownerKeysToOrphan;
    for (const auto &ownerEntry : shareGroup->owners)
    {
        if (ownerEntry.second == ctx) ownerKeysToOrphan.push_back(ownerEntry.first);
    }

    if (ownerKeysToOrphan.empty()) return 0u;

    std::vector<uint64_t> insertedOrphanKeys;
    insertedOrphanKeys.reserve(ownerKeysToOrphan.size());

    try
    {
        for (uint64_t ownerKey : ownerKeysToOrphan)
        {
            std::pair<std::unordered_set<uint64_t>::iterator, bool> insertResult =
                shareGroup->orphanedOwners.insert(ownerKey);
            if (insertResult.second) insertedOrphanKeys.push_back(ownerKey);
        }
    }
    catch (...)
    {
        for (uint64_t insertedKey : insertedOrphanKeys) shareGroup->orphanedOwners.erase(insertedKey);
        throw;
    }

    for (uint64_t ownerKey : ownerKeysToOrphan)
    {
        std::unordered_map<uint64_t, RLContext*>::iterator ownerIt = shareGroup->owners.find(ownerKey);
        if (ownerIt != shareGroup->owners.end()) ownerIt->second = nullptr;
    }

    return (unsigned int)ownerKeysToOrphan.size();
}

static bool GetOrCreateProgramUseScopeLocked(RLSharedGpuGroup *shareGroup, uint64_t key, std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> *scopeOut)
{
    if (scopeOut != nullptr) *scopeOut = std::shared_ptr<RLSharedGpuGroup::ProgramUseScope>();
    if ((shareGroup == nullptr) || (key == 0) || (scopeOut == nullptr)) return false;

    auto scopeIt = shareGroup->programUseScopes.find(key);
    if (scopeIt != shareGroup->programUseScopes.end())
    {
        *scopeOut = scopeIt->second;
        return (*scopeOut != nullptr);
    }

    std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope = std::make_shared<RLSharedGpuGroup::ProgramUseScope>();
    shareGroup->programUseScopes.emplace(key, scope);
    *scopeOut = scope;
    return true;
}

static bool QueueProgramFenceForDeleteLocked(RLSharedGpuGroup *shareGroup, void *fence)
{
    if ((shareGroup == nullptr) || (fence == nullptr)) return false;
    shareGroup->pendingProgramFences.push_back(fence);
    return true;
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
            if (IsSharedGpuCumulativeDiagStatsEnabled()) gUnregisteredRetainRejectCount.fetch_add(1, std::memory_order_relaxed);
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
            if (IsSharedGpuCumulativeDiagStatsEnabled()) gUnregisteredRetainRejectCount.fetch_add(1, std::memory_order_relaxed);
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

static bool ReleaseKeyLocked(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, uint64_t key)
{
    if (!shareGroup || key == 0) return false;
    auto refIt = shareGroup->refs.find(key);
    if (refIt == shareGroup->refs.end()) {
        if (IsStrictTrackingEnabled())
        {
            if (IsSharedGpuCumulativeDiagStatsEnabled()) gUnregisteredReleaseRejectCount.fetch_add(1, std::memory_order_relaxed);
            uint32_t keyType = 0, keyId = 0;
            SplitKey(key, keyType, keyId);
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: release on unregistered object rejected (requestedType=%u keyType=%u id=%u)",
                (unsigned)type, (unsigned)keyType, (unsigned)keyId);
            return false;
        }

        // Compatibility mode: ignore release in this group to avoid accidental cross-group deletes.
        NoteUntrackedReleaseLocked(shareGroup, type, key);
        return false;
    }

    if (refIt->second <= 1) {
        auto programLocIt = shareGroup->programLocs.end();
        void *programLocs = nullptr;
        auto scopeIt = shareGroup->programUseScopes.end();
        std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
        bool queuedFence = false;

        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            programLocIt = shareGroup->programLocs.find(key);
            if (programLocIt != shareGroup->programLocs.end()) programLocs = programLocIt->second;

            scopeIt = shareGroup->programUseScopes.find(key);
            if (scopeIt != shareGroup->programUseScopes.end()) scope = scopeIt->second;

            if ((scope != nullptr) && (scope->lastFence != nullptr))
            {
                QueueProgramFenceForDeleteLocked(shareGroup, scope->lastFence);
                queuedFence = true;
            }
        }

        try
        {
            PushPendingDeleteLocked(shareGroup, key);
        }
        catch (...)
        {
            if (queuedFence) shareGroup->pendingProgramFences.pop_back();
            throw;
        }

        shareGroup->refs.erase(refIt);
        shareGroup->owners.erase(key);
        shareGroup->orphanedOwners.erase(key);
        if (type == RL_SHARED_GPU_OBJECT_PROGRAM) {
            if ((scope != nullptr) && queuedFence) scope->lastFence = nullptr;
            if (programLocIt != shareGroup->programLocs.end()) {
                if (programLocs != nullptr) RL_FREE(programLocs);
                shareGroup->programLocs.erase(programLocIt);
            }
            if (scopeIt != shareGroup->programUseScopes.end()) shareGroup->programUseScopes.erase(scopeIt);
        }
        return true;
    }

    refIt->second -= 1;
    return true;
}

static void ReleaseObjectInGroup(RLSharedGpuGroup *shareGroup, RLSharedGpuObjectType type, unsigned int id)
{
    if (!shareGroup || id == 0) return;
    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)id);

    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    (void)ReleaseKeyLocked(shareGroup, type, key);
}

static void RegisterFramebufferAttachmentInGroup(RLSharedGpuGroup *shareGroup, unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    if (!shareGroup || framebufferId == 0 || objId == 0) return;
    if (type != RL_SHARED_GPU_OBJECT_TEXTURE && type != RL_SHARED_GPU_OBJECT_RENDERBUFFER) return;

    const uint64_t key = MakeKey((uint32_t)type, (uint32_t)objId);
    RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
    const uint32_t framebufferKey = (uint32_t)framebufferId;
    auto framebufferIt = shareGroup->framebufferAttachments.find(framebufferKey);
    bool createdFramebufferEntry = false;
    if (framebufferIt == shareGroup->framebufferAttachments.end())
    {
        auto insertResult = shareGroup->framebufferAttachments.emplace(framebufferKey, std::unordered_map<int, uint64_t>());
        framebufferIt = insertResult.first;
        createdFramebufferEntry = insertResult.second;
    }

    auto attachmentIt = framebufferIt->second.find(attachment);
    const bool hadOldAttachment = (attachmentIt != framebufferIt->second.end());
    const uint64_t oldAttachmentKey = hadOldAttachment ? attachmentIt->second : 0u;

    const bool syncDepth = (attachment == 100);
    auto depthIt = shareGroup->framebufferDepth.find(framebufferKey);
    const bool hadOldDepth = (depthIt != shareGroup->framebufferDepth.end());
    const uint64_t oldDepthKey = hadOldDepth ? depthIt->second : 0u;

    try
    {
        framebufferIt->second[attachment] = key;
    }
    catch (...)
    {
        if (createdFramebufferEntry && framebufferIt->second.empty()) shareGroup->framebufferAttachments.erase(framebufferIt);
        throw;
    }

    // Keep legacy depth mapping in sync for existing query path.
    if (syncDepth)
    {
        try
        {
            shareGroup->framebufferDepth[framebufferKey] = key;
        }
        catch (...)
        {
            if (hadOldAttachment) framebufferIt->second[attachment] = oldAttachmentKey;
            else framebufferIt->second.erase(attachment);
            if (createdFramebufferEntry && framebufferIt->second.empty()) shareGroup->framebufferAttachments.erase(framebufferIt);
            if (hadOldDepth) shareGroup->framebufferDepth[framebufferKey] = oldDepthKey;
            else shareGroup->framebufferDepth.erase(framebufferKey);
            throw;
        }
    }
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
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuHasCurrentGroup", false)
        PinnedGroup pinned = PinExistingGroupForCurrentContext();
        return (bool)pinned;
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuHasContextGroup(RLContext *ctx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuHasContextGroup", false)
        PinnedGroup pinned = PinExistingGroupForContext(ctx);
        return (bool)pinned;
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuContextUsesSharedTrackedScope(RLContext *ctx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuContextUsesSharedTrackedScope", false)
        PinnedGroup pinned = PinExistingGroupForContext(ctx);
        return GroupUsesSharedTrackedScope(pinned.get());
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuContextResolveTrackedScopeHandle(RLContext *ctx, void **groupHandleOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuContextResolveTrackedScopeHandle", false)
        if (groupHandleOut != nullptr) *groupHandleOut = nullptr;
        if ((ctx == nullptr) || (groupHandleOut == nullptr)) return false;

        RLSharedGpuBindingLockScope bindingLock;
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
        if ((shareGroup == nullptr) || !GroupUsesSharedTrackedScope(shareGroup)) return false;

        *groupHandleOut = (void *)shareGroup;
        return true;
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuGroupUsesSharedTrackedScope(const void *groupHandle)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuGroupUsesSharedTrackedScope", false)
        return GroupUsesSharedTrackedScope((const RLSharedGpuGroup *)groupHandle);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuGroupSetSharedTrackedScope(void *groupHandle)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuGroupSetSharedTrackedScope")
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
        if (shareGroup == nullptr) return;
        shareGroup->trackedScopePolicy.store((int)RL_SHARED_GPU_TRACKED_SCOPE_SHARE_GROUP, std::memory_order_release);
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuContextBindShareGroup", false)
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
                if (current == nullptr)
                {
                    current = NewGroup();
                    if (current == nullptr) return false;
                    ctx->gpuShareGroup = (void *)current;
                }
                return true;
            }

            if (current == desired) return true;

            if ((current != nullptr) && (current != desired))
            {
                RLTraceLog(RL_E_LOG_WARNING,
                    "SHARED_GPU: share-group rebind failed: rebinding a live context to a different share-group is unsupported");
                return false;
            }
        }

        if (GroupUsesSharedTrackedScope(desired))
        {
            RLTrackedPromotionResult contextPromotionResult = RLTrackedObjectPromoteContextEntriesToShareGroup(ctx, (void *)desired);
            if (contextPromotionResult == RL_TRACKED_PROMOTION_FAILED)
            {
                RLTraceLog(RL_E_LOG_WARNING,
                    "SHARED_GPU: share-group bind failed: tracked promotion aborted for source context");
                return false;
            }
        }

        {
            RLSharedGpuBindingLockScope bindingLock;
            current = (RLSharedGpuGroup *)ctx->gpuShareGroup;

            if (current == desired) return true;
            if (current != nullptr)
            {
                RLTraceLog(RL_E_LOG_WARNING,
                    "SHARED_GPU: share-group bind failed: context binding changed during bind");
                return false;
            }

            AddGroupRef(desired);                   // New binding ref for ctx.
            ctx->gpuShareGroup = (void *)desired;
        }

        return true;
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuContextUnbindShareGroup(RLContext *ctx)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuContextUnbindShareGroup")
        if (!ctx) return;
        RLSharedGpuGroup *shareGroup = nullptr;
        unsigned int orphanedCount = 0u;
        {
            RLSharedGpuBindingLockScope bindingLock;
            shareGroup = (RLSharedGpuGroup *)ctx->gpuShareGroup;
            if (!shareGroup) return;

            RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
            orphanedCount = OrphanOwnersForContextLocked(shareGroup, ctx);
            ctx->gpuShareGroup = nullptr;
        }

        if (orphanedCount > 0u)
        {
            RLTraceLog(RL_E_LOG_WARNING,
                "SHARED_GPU: context unbind orphaned %u shared object owners; adopt explicitly if still needed",
                orphanedCount);
        }

        ReleaseGroupRef(shareGroup);               // Drop ctx binding ref after orphan + unbind commit succeeds.
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRegisterObject(RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRegisterObject")
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RegisterObjectInGroup(pinned.get(), type, id);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRegisterProgramLocs(unsigned int programId, int *locs)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRegisterProgramLocs")
        if (programId == 0 || locs == nullptr) return;
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (shareGroup == nullptr) return;
        const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        auto programLocIt = shareGroup->programLocs.find(key);
        if (programLocIt == shareGroup->programLocs.end()) {
            try
            {
                shareGroup->programLocs.emplace(key, (void*)locs);
            }
            catch (...)
            {
                RL_FREE(locs);
                throw;
            }
        } else {
            // Should not happen; keep existing pointer, free the new one to avoid leaks.
            RL_FREE(locs);
        }
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRegisterFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType type, unsigned int objId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRegisterFramebufferDepth")
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RegisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, 100, type, objId);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuUnregisterFramebufferDepth(unsigned int framebufferId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuUnregisterFramebufferDepth")
        PinnedGroup pinned = PinExistingGroupForCurrentContext();
        UnregisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, 100);
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuQueryFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType *typeOut, unsigned int *objIdOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuQueryFramebufferDepth", false)
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
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRetainFramebufferTree(unsigned int framebufferId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRetainFramebufferTree")
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
            if (IsSharedGpuCumulativeDiagStatsEnabled()) shareGroup->fboAttachmentMapHitCount += 1;
            std::unordered_set<uint64_t> dedup;
            for (const auto &attachmentEntry : framebufferIt->second)
            {
                if (dedup.insert(attachmentEntry.second).second) RetainKeyLocked(shareGroup, attachmentEntry.second);
            }
        }
        else
        {
            if (IsSharedGpuCumulativeDiagStatsEnabled()) shareGroup->fboAttachmentMapMissCount += 1;
            // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
            auto depthIt = shareGroup->framebufferDepth.find((uint32_t)framebufferId);
            if (depthIt != shareGroup->framebufferDepth.end()) RetainKeyLocked(shareGroup, depthIt->second);
        }
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuReleaseFramebufferTree")
        if (framebufferId == 0) return;
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (!shareGroup) return;
        const uint64_t fboKey = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_FRAMEBUFFER, (uint32_t)framebufferId);
        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        auto framebufferIt = shareGroup->framebufferAttachments.find((uint32_t)framebufferId);
        if (framebufferIt != shareGroup->framebufferAttachments.end())
        {
            if (IsSharedGpuCumulativeDiagStatsEnabled()) shareGroup->fboAttachmentMapHitCount += 1;
            std::unordered_set<uint64_t> dedup;
            for (const auto &attachmentEntry : framebufferIt->second)
            {
                if (!dedup.insert(attachmentEntry.second).second) continue;
                if (IsSharedGpuCumulativeDiagStatsEnabled() && (shareGroup->refs.find(attachmentEntry.second) == shareGroup->refs.end())) shareGroup->fboAttachmentReleaseSkippedCount += 1;
                uint32_t objectTypeBits = 0, objectId = 0;
                SplitKey(attachmentEntry.second, objectTypeBits, objectId);
                (void)ReleaseKeyLocked(shareGroup, (RLSharedGpuObjectType)objectTypeBits, attachmentEntry.second);
            }
        }
        else
        {
            if (IsSharedGpuCumulativeDiagStatsEnabled()) shareGroup->fboAttachmentMapMissCount += 1;
            // Backward-compatible fallback: use legacy depth mapping when generic map is unavailable.
            auto depthIt = shareGroup->framebufferDepth.find((uint32_t)framebufferId);
            if (depthIt != shareGroup->framebufferDepth.end())
            {
                if (IsSharedGpuCumulativeDiagStatsEnabled() && (shareGroup->refs.find(depthIt->second) == shareGroup->refs.end())) shareGroup->fboAttachmentReleaseSkippedCount += 1;
                uint32_t objectTypeBits = 0, objectId = 0;
                SplitKey(depthIt->second, objectTypeBits, objectId);
                (void)ReleaseKeyLocked(shareGroup, (RLSharedGpuObjectType)objectTypeBits, depthIt->second);
            }
        }
        (void)ReleaseKeyLocked(shareGroup, RL_SHARED_GPU_OBJECT_FRAMEBUFFER, fboKey);
        // If framebuffer is no longer tracked, drop its attachment mapping to avoid staleness.
        if (shareGroup->refs.find(fboKey) == shareGroup->refs.end()) {
            shareGroup->framebufferAttachments.erase((uint32_t)framebufferId);
            shareGroup->framebufferDepth.erase((uint32_t)framebufferId);
        }
    RL_SHARED_GPU_TRY_END()
}

RLSharedGpuGroupDiagStatsInternal RLSharedGpuGetGroupDiagStatsForContextInternal(RLContext *ctx)
{
    RL_SHARED_GPU_TRY_VALUE(RLSharedGpuGroupDiagStatsInternal, "RLSharedGpuGetGroupDiagStatsForContextInternal", RLSharedGpuGroupDiagStatsInternal{ 0 })
        RLSharedGpuGroupDiagStatsInternal out = { 0 };
        PinnedGroup pinned = PinExistingGroupForContext(ctx);
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (!shareGroup) return out;

        out.hasShareGroup = 1;
        out.contextRefCount = shareGroup->ctxRefs.load(std::memory_order_relaxed);
        out.usesSharedTrackedScope = (shareGroup->trackedScopePolicy.load(std::memory_order_relaxed) == RL_SHARED_GPU_TRACKED_SCOPE_SHARE_GROUP);

        {
        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
            out.liveObjectCount = (unsigned long long)shareGroup->refs.size();
            out.pendingDeleteCount = (unsigned long long)shareGroup->pending.size();
            out.ownerEntryCount = (unsigned long long)shareGroup->owners.size();
            out.orphanedOwnerCount = (unsigned long long)shareGroup->orphanedOwners.size();
            out.framebufferAttachmentMapCount = (unsigned long long)shareGroup->framebufferAttachments.size();
            out.framebufferDepthMapCount = (unsigned long long)shareGroup->framebufferDepth.size();
            out.programLocEntryCount = (unsigned long long)shareGroup->programLocs.size();
            out.programUseScopeCount = (unsigned long long)shareGroup->programUseScopes.size();
            out.pendingProgramFenceCount = (unsigned long long)shareGroup->pendingProgramFences.size();
            out.textureTraceCount = (unsigned long long)shareGroup->textureTrace.size();
            out.releaseUntrackedCount = (unsigned long long)shareGroup->releaseUntrackedCount;
            out.framebufferMapHitCount = (unsigned long long)shareGroup->fboAttachmentMapHitCount;
            out.framebufferMapMissCount = (unsigned long long)shareGroup->fboAttachmentMapMissCount;
            out.framebufferReleaseSkippedCount = (unsigned long long)shareGroup->fboAttachmentReleaseSkippedCount;

            for (auto &refEntry : shareGroup->refs) {
                uint32_t objectTypeBits = 0;
                uint32_t objectId = 0;
                SplitKey(refEntry.first, objectTypeBits, objectId);
                switch (objectTypeBits)
                {
                    case RL_SHARED_GPU_OBJECT_TEXTURE: out.liveTextureCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_BUFFER: out.liveBufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_VERTEX_ARRAY: out.liveVertexArrayCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_FRAMEBUFFER: out.liveFramebufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_RENDERBUFFER: out.liveRenderbufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_PROGRAM: out.liveProgramCount += 1; break;
                    default: break;
                }
            }

            for (auto &pendingKey : shareGroup->pending) {
                uint32_t objectTypeBits = 0;
                uint32_t objectId = 0;
                SplitKey(pendingKey, objectTypeBits, objectId);
                switch (objectTypeBits)
                {
                    case RL_SHARED_GPU_OBJECT_TEXTURE: out.pendingTextureCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_BUFFER: out.pendingBufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_VERTEX_ARRAY: out.pendingVertexArrayCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_FRAMEBUFFER: out.pendingFramebufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_RENDERBUFFER: out.pendingRenderbufferCount += 1; break;
                    case RL_SHARED_GPU_OBJECT_PROGRAM: out.pendingProgramCount += 1; break;
                    default: break;
                }
            }
        }

        return out;
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuGetObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext **ownerOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuGetObjectOwner", false)
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuIsObjectOwnedByCurrentContext(RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuIsObjectOwnedByCurrentContext", false)
        RLContext *owner = nullptr;
        if (!RLSharedGpuGetObjectOwner(type, id, &owner)) return false;
        return (owner == GetCurrentContextSafe());
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuTryTransferObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuTryTransferObjectOwner", false)
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuTryAdoptOrphanedObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuTryAdoptOrphanedObjectOwner", false)
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuLockOwnerGroup(RLContext *currentCtx, RLContext *targetCtx, void **groupHandleOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuLockOwnerGroup", false)
        if (groupHandleOut != nullptr) *groupHandleOut = nullptr;
        if ((currentCtx == nullptr) || (targetCtx == nullptr) || (groupHandleOut == nullptr)) return false;

        PinnedGroup currentPinned = PinExistingGroupForContext(currentCtx);
        PinnedGroup targetPinned = PinExistingGroupForContext(targetCtx);
        RLSharedGpuGroup *shareGroup = currentPinned.get();
        if ((shareGroup == nullptr) || (targetPinned.get() != shareGroup)) return false;

        RLSharedGpuOwnerGroupAcquireGuard ownerGroupGuard;
        ownerGroupGuard.SetGroup(shareGroup);
        ownerGroupGuard.AcquireRef();
        ownerGroupGuard.LockMutex();
        ownerGroupGuard.EnterIsolation();
        *groupHandleOut = (void *)shareGroup;
        ownerGroupGuard.Disarm();
        return true;
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuUnlockOwnerGroup(void *groupHandle)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuUnlockOwnerGroup")
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
        if (shareGroup == nullptr) return;
        shareGroup->groupStateMutex.unlock();
        RLTraceCallbackIsolationLeave();
        ReleaseGroupRef(shareGroup);
    RL_SHARED_GPU_TRY_END()
}

int RLSharedGpuCanTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *currentCtx, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_VALUE(int, "RLSharedGpuCanTransferObjectOwnerLocked", -1)
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
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuTransferObjectOwnerLocked")
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
        if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return;

        const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
        shareGroup->owners[objectKey] = targetCtx;
        shareGroup->orphanedOwners.erase(objectKey);
    RL_SHARED_GPU_TRY_END()
}

int RLSharedGpuCanAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_VALUE(int, "RLSharedGpuCanAdoptOrphanedObjectOwnerLocked", -1)
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
        if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return -1;

        const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
        if (shareGroup->refs.find(objectKey) == shareGroup->refs.end()) return 0;

        auto ownerIt = shareGroup->owners.find(objectKey);
        if (ownerIt == shareGroup->owners.end()) return 0;
        if ((ownerIt->second == targetCtx) && (shareGroup->orphanedOwners.find(objectKey) == shareGroup->orphanedOwners.end())) return 2;
        if ((ownerIt->second != nullptr) || (shareGroup->orphanedOwners.find(objectKey) == shareGroup->orphanedOwners.end())) return -1;

        return 1;
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuAdoptOrphanedObjectOwnerLocked")
        RLSharedGpuGroup *shareGroup = (RLSharedGpuGroup *)groupHandle;
        if ((shareGroup == nullptr) || (id == 0) || (targetCtx == nullptr)) return;

        const uint64_t objectKey = MakeKey((uint32_t)type, (uint32_t)id);
        shareGroup->owners[objectKey] = targetCtx;
        shareGroup->orphanedOwners.erase(objectKey);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRegisterFramebufferAttachment(unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRegisterFramebufferAttachment")
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RegisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, attachment, type, objId);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuUnregisterFramebufferAttachment(unsigned int framebufferId, int attachment)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuUnregisterFramebufferAttachment")
        PinnedGroup pinned = PinExistingGroupForCurrentContext();
        UnregisterFramebufferAttachmentInGroup(pinned.get(), framebufferId, attachment);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuUnregisterFramebufferAttachments(unsigned int framebufferId)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuUnregisterFramebufferAttachments")
        PinnedGroup pinned = PinExistingGroupForCurrentContext();
        UnregisterFramebufferAttachmentsInGroup(pinned.get(), framebufferId);
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuBeginProgramUseScope(unsigned int programId, int policy)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuBeginProgramUseScope", false)
        if (programId == 0) return false;

        PinnedGroup pinned = PinExistingGroupForCurrentContext();
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (!shareGroup) return false;

        const uint64_t key = MakeKey((uint32_t)RL_SHARED_GPU_OBJECT_PROGRAM, (uint32_t)programId);
        std::shared_ptr<RLSharedGpuGroup::ProgramUseScope> scope;
        {
        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
            if (shareGroup->refs.find(key) == shareGroup->refs.end()) return false;
            if (!GetOrCreateProgramUseScopeLocked(shareGroup, key, &scope)) return false;
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
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuEndProgramUseScope(unsigned int programId, int policy)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuEndProgramUseScope")
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuTakeProgramFence(unsigned int programId, void **fenceOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuTakeProgramFence", false)
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuStoreProgramFence(unsigned int programId, void *fence)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuStoreProgramFence", false)
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
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuPopPendingProgramFence(void **fenceOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuPopPendingProgramFence", false)
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
    RL_SHARED_GPU_TRY_END()
}


void RLSharedGpuRetainObject(RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRetainObject")
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RetainObjectInGroup(pinned.get(), type, id);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuReleaseObject(RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuReleaseObject")
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        ReleaseObjectInGroup(pinned.get(), type, id);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuRetainObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuRetainObjectOnContext")
        PinnedGroup pinned = EnsurePinnedGroupForContext(ctx);
        RetainObjectInGroup(pinned.get(), type, id);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuReleaseObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuReleaseObjectOnContext")
        PinnedGroup pinned = EnsurePinnedGroupForContext(ctx);
        ReleaseObjectInGroup(pinned.get(), type, id);
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuPopPendingDelete(RLSharedGpuObjectType *typeOut, unsigned int *idOut)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuPopPendingDelete", false)
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
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuDebugDumpState(const char *label)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuDebugDumpState")
        RLSharedGpuGroupDiagStatsInternal stats = RLSharedGpuGetGroupDiagStatsForContextInternal(GetCurrentContextSafe());
        RLSharedGpuTrackingRejectDiagStatsInternal rejectStats = RLSharedGpuGetTrackingRejectDiagStatsInternal();
        if (!stats.hasShareGroup) {
            RLTraceLog(RL_E_LOG_INFO, "SHARED_GPU: %s: no share-group bound on current context", label ? label : "state");
            return;
        }

        RLTraceLog(RL_E_LOG_INFO,
            "SHARED_GPU: %s: ctxRefs=%u live=%llu pending=%llu owners=%llu orphaned=%llu trackedScope=%s untrackedRelease=%llu | "
            "live(tex=%llu buf=%llu vao=%llu fbo=%llu rbo=%llu prog=%llu) pend(tex=%llu buf=%llu vao=%llu fbo=%llu rbo=%llu prog=%llu)",
            label ? label : "state",
            stats.contextRefCount,
            stats.liveObjectCount,
            stats.pendingDeleteCount,
            stats.ownerEntryCount,
            stats.orphanedOwnerCount,
            stats.usesSharedTrackedScope ? "share-group" : "context",
            stats.releaseUntrackedCount,
            stats.liveTextureCount,
            stats.liveBufferCount,
            stats.liveVertexArrayCount,
            stats.liveFramebufferCount,
            stats.liveRenderbufferCount,
            stats.liveProgramCount,
            stats.pendingTextureCount,
            stats.pendingBufferCount,
            stats.pendingVertexArrayCount,
            stats.pendingFramebufferCount,
            stats.pendingRenderbufferCount,
            stats.pendingProgramCount);
        RLTraceLog(RL_E_LOG_INFO,
            "SHARED_GPU: %s: maps(attach=%llu depth=%llu programLoc=%llu programScope=%llu pendingFence=%llu textureTrace=%llu) "
            "fboMap(hit=%llu miss=%llu releaseSkip=%llu) reject(retain=%llu release=%llu)",
            label ? label : "state",
            stats.framebufferAttachmentMapCount,
            stats.framebufferDepthMapCount,
            stats.programLocEntryCount,
            stats.programUseScopeCount,
            stats.pendingProgramFenceCount,
            stats.textureTraceCount,
            stats.framebufferMapHitCount,
            stats.framebufferMapMissCount,
            stats.framebufferReleaseSkippedCount,
            rejectStats.unregisteredRetainRejectCount,
            rejectStats.unregisteredReleaseRejectCount);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuSetTextureDebugLabel(unsigned int id, const char *label, const char *sourceFile, int sourceLine)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuSetTextureDebugLabel")
        if (id == 0 || label == nullptr) return;
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (!shareGroup) return;

        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        RLSharedGpuGroup::TextureTrace &trace = shareGroup->textureTrace[id];
        trace.label = label;
        trace.markSource = BuildSourceText(sourceFile, sourceLine);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuTraceTextureRelease(unsigned int id, const char *sourceFile, int sourceLine, const char *reason)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuTraceTextureRelease")
        if (id == 0) return;
        PinnedGroup pinned = EnsurePinnedGroupForCurrentContext();
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (!shareGroup) return;

        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        RLSharedGpuGroup::TextureTrace &trace = shareGroup->textureTrace[id];
        if (trace.label.empty()) trace.label = "texture";
        trace.releaseSource = BuildSourceText(sourceFile, sourceLine);
        trace.releaseReason = (reason != nullptr) ? reason : "(none)";
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuSetTrackingMode(RLSharedGpuTrackingModeInternal mode)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuSetTrackingMode")
        if ((mode != RL_SHARED_GPU_TRACKING_MODE_COMPATIBLE) && (mode != RL_SHARED_GPU_TRACKING_MODE_STRICT))
        {
            RLTraceLog(RL_E_LOG_WARNING, "SHARED_GPU: invalid tracking mode=%d", (int)mode);
            return;
        }

        gTrackingMode.store((int)mode, std::memory_order_relaxed);
        RLTraceLog(RL_E_LOG_INFO, "SHARED_GPU: tracking mode=%s",
                   (mode == RL_SHARED_GPU_TRACKING_MODE_STRICT) ? "strict" : "compatible");
    RL_SHARED_GPU_TRY_END()
}

RLSharedGpuTrackingModeInternal RLSharedGpuGetTrackingMode(void)
{
    RL_SHARED_GPU_TRY_VALUE(RLSharedGpuTrackingModeInternal, "RLSharedGpuGetTrackingMode", RL_SHARED_GPU_TRACKING_MODE_STRICT)
        return (RLSharedGpuTrackingModeInternal)gTrackingMode.load(std::memory_order_relaxed);
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuEnableCumulativeDiagStats(void)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuEnableCumulativeDiagStats")
    #if RL_SHARED_GPU_DIAG_STATS
        gDiagStatsEnabled.store(1, std::memory_order_relaxed);
    #endif
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuDisableCumulativeDiagStats(void)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuDisableCumulativeDiagStats")
    #if RL_SHARED_GPU_DIAG_STATS
        gDiagStatsEnabled.store(0, std::memory_order_relaxed);
    #endif
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuIsCumulativeDiagStatsEnabled(void)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuIsCumulativeDiagStatsEnabled", false)
    #if RL_SHARED_GPU_DIAG_STATS
        return IsSharedGpuCumulativeDiagStatsEnabled();
    #else
        return false;
    #endif
    RL_SHARED_GPU_TRY_END()
}

bool RLSharedGpuResetGroupDiagStatsForContextInternal(RLContext *ctx)
{
    RL_SHARED_GPU_TRY_BOOL("RLSharedGpuResetGroupDiagStatsForContextInternal", false)
    #if RL_SHARED_GPU_DIAG_STATS
        PinnedGroup pinned = PinExistingGroupForContext(ctx);
        RLSharedGpuGroup *shareGroup = pinned.get();
        if (shareGroup == nullptr) return false;

        RLSharedGpuGroupLockScope shareGroupLock(shareGroup);
        shareGroup->releaseUntrackedCount = 0;
        shareGroup->fboAttachmentMapHitCount = 0;
        shareGroup->fboAttachmentMapMissCount = 0;
        shareGroup->fboAttachmentReleaseSkippedCount = 0;
        return true;
    #else
        (void)ctx;
        return false;
    #endif
    RL_SHARED_GPU_TRY_END()
}

RLSharedGpuTrackingRejectDiagStatsInternal RLSharedGpuGetTrackingRejectDiagStatsInternal(void)
{
    RL_SHARED_GPU_TRY_VALUE(RLSharedGpuTrackingRejectDiagStatsInternal, "RLSharedGpuGetTrackingRejectDiagStatsInternal", RLSharedGpuTrackingRejectDiagStatsInternal{ 0 })
        RLSharedGpuTrackingRejectDiagStatsInternal out = { 0 };
        out.unregisteredRetainRejectCount = gUnregisteredRetainRejectCount.load(std::memory_order_relaxed);
        out.unregisteredReleaseRejectCount = gUnregisteredReleaseRejectCount.load(std::memory_order_relaxed);
        return out;
    RL_SHARED_GPU_TRY_END()
}

void RLSharedGpuResetTrackingRejectDiagStatsInternal(void)
{
    RL_SHARED_GPU_TRY_VOID("RLSharedGpuResetTrackingRejectDiagStatsInternal")
        gUnregisteredRetainRejectCount.store(0, std::memory_order_relaxed);
        gUnregisteredReleaseRejectCount.store(0, std::memory_order_relaxed);
    RL_SHARED_GPU_TRY_END()
}

} // extern "C"
