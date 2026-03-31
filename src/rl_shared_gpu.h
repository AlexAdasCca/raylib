#ifndef RL_SHARED_GPU_H
#define RL_SHARED_GPU_H

// Internal helper for share-group wide GPU object lifetime management.
//
// This module provides:
// - Share-group binding for RLContext instances
// - A lightweight reference counter per GL object id (per share-group)
// - Deferred deletion queue: the last release enqueues a delete, actual glDelete* happens
//   on a thread with a current OpenGL context (drained by rlgl).
//
// NOTE:
// - gpuShareGroup is an internal lifetime/deferred-delete grouping primitive.
// - tracked-object shared-namespace policy is separate and may remain context-scoped
//   even when a gpuShareGroup exists.

#include "rl_context.h"   // Internal RLContext definition

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RLSharedGpuObjectType {
    RL_SHARED_GPU_OBJECT_TEXTURE = 1,
    RL_SHARED_GPU_OBJECT_BUFFER = 2,
    RL_SHARED_GPU_OBJECT_VERTEX_ARRAY = 3,
    RL_SHARED_GPU_OBJECT_FRAMEBUFFER = 4,
    RL_SHARED_GPU_OBJECT_RENDERBUFFER = 5,
    RL_SHARED_GPU_OBJECT_PROGRAM = 6,
} RLSharedGpuObjectType;

typedef enum RLSharedGpuTrackingModeInternal {
    RL_SHARED_GPU_TRACKING_MODE_COMPATIBLE = 0, // Unregistered retain/release follows compatibility fallback.
    RL_SHARED_GPU_TRACKING_MODE_STRICT = 1      // Unregistered retain/release is rejected and counted.
} RLSharedGpuTrackingModeInternal;

typedef enum RLSharedGpuTrackedScopePolicyInternal {
    RL_SHARED_GPU_TRACKED_SCOPE_CONTEXT = 0,
    RL_SHARED_GPU_TRACKED_SCOPE_SHARE_GROUP = 1
} RLSharedGpuTrackedScopePolicyInternal;

// Bind ctx to a share-group. If shareWithCtx is non-NULL, ctx joins shareWithCtx's group.
// Otherwise a new share-group is created for ctx.
// Returns false if an explicit share target was requested but it has no active group,
// tracked-object promotion failed, or rebinding a live context to a different share-group was attempted.
// Safe to call multiple times.
bool RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx);

// Unbind ctx from its share-group. If it was the last context in the group, the group is freed.
void RLSharedGpuContextUnbindShareGroup(RLContext *ctx);

// Register a newly created GL object (initial refcount = 1) in the CURRENT context share-group.
void RLSharedGpuRegisterObject(RLSharedGpuObjectType type, unsigned int id);

// Optional auxiliary data tied to a program object (CPU-side locations array).
// The memory is freed automatically when the program refcount reaches 0.
void RLSharedGpuRegisterProgramLocs(unsigned int programId, int *locs);

// Optional auxiliary data tied to a framebuffer object: its depth attachment (texture/renderbuffer).
// This enables context-free retain/release of render textures across a share-group.
void RLSharedGpuRegisterFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType type, unsigned int objId);

// Remove any cached depth attachment mapping for a framebuffer.
// Useful when an attachment is detached or replaced.
void RLSharedGpuUnregisterFramebufferDepth(unsigned int framebufferId);

// Query cached depth attachment mapping for a framebuffer.
// Returns true if a mapping exists in the CURRENT context share-group.
bool RLSharedGpuQueryFramebufferDepth(unsigned int framebufferId, RLSharedGpuObjectType *typeOut, unsigned int *objIdOut);

// Register or unregister any framebuffer attachment (color/depth/stencil slot).
// attachment uses rlgl attachment ids: color channels [0..7], depth=100, stencil=200.
void RLSharedGpuRegisterFramebufferAttachment(unsigned int framebufferId, int attachment, RLSharedGpuObjectType type, unsigned int objId);
void RLSharedGpuUnregisterFramebufferAttachment(unsigned int framebufferId, int attachment);
void RLSharedGpuUnregisterFramebufferAttachments(unsigned int framebufferId);

// Convenience helpers to retain/release framebuffer and its registered depth attachment together.
void RLSharedGpuRetainFramebufferTree(unsigned int framebufferId);
void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId);

typedef struct RLSharedGpuGroupDiagStatsInternal {
    int hasShareGroup;
    int usesSharedTrackedScope;
    unsigned int contextRefCount;
    unsigned long long liveObjectCount;
    unsigned long long pendingDeleteCount;
    unsigned long long ownerEntryCount;
    unsigned long long orphanedOwnerCount;
    unsigned long long framebufferAttachmentMapCount;
    unsigned long long framebufferDepthMapCount;
    unsigned long long programLocEntryCount;
    unsigned long long programUseScopeCount;
    unsigned long long pendingProgramFenceCount;
    unsigned long long textureTraceCount;
    unsigned long long liveTextureCount;
    unsigned long long liveBufferCount;
    unsigned long long liveVertexArrayCount;
    unsigned long long liveFramebufferCount;
    unsigned long long liveRenderbufferCount;
    unsigned long long liveProgramCount;
    unsigned long long pendingTextureCount;
    unsigned long long pendingBufferCount;
    unsigned long long pendingVertexArrayCount;
    unsigned long long pendingFramebufferCount;
    unsigned long long pendingRenderbufferCount;
    unsigned long long pendingProgramCount;
    unsigned long long releaseUntrackedCount;
    unsigned long long framebufferMapHitCount;
    unsigned long long framebufferMapMissCount;
    unsigned long long framebufferReleaseSkippedCount;
} RLSharedGpuGroupDiagStatsInternal;

RLSharedGpuGroupDiagStatsInternal RLSharedGpuGetGroupDiagStatsForContextInternal(RLContext *ctx);

// Owner-query/transfer helpers (write-ownership control, per share-group object key).
bool RLSharedGpuGetObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext **ownerOut);
bool RLSharedGpuIsObjectOwnedByCurrentContext(RLSharedGpuObjectType type, unsigned int id);
bool RLSharedGpuTryTransferObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx);
bool RLSharedGpuTryAdoptOrphanedObjectOwner(RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx);
bool RLSharedGpuLockOwnerGroup(RLContext *currentCtx, RLContext *targetCtx, void **groupHandleOut);
void RLSharedGpuUnlockOwnerGroup(void *groupHandle);
int RLSharedGpuCanTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *currentCtx, RLContext *targetCtx);
void RLSharedGpuTransferObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx);
int RLSharedGpuCanAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx);
void RLSharedGpuAdoptOrphanedObjectOwnerLocked(void *groupHandle, RLSharedGpuObjectType type, unsigned int id, RLContext *targetCtx);

// Serialized scope for shared shader program concurrent use.
// policy values are RLSharedShaderUsePolicy from raylib.h.
bool RLSharedGpuBeginProgramUseScope(unsigned int programId, int policy);
void RLSharedGpuEndProgramUseScope(unsigned int programId, int policy);
bool RLSharedGpuTakeProgramFence(unsigned int programId, void **fenceOut);
bool RLSharedGpuStoreProgramFence(unsigned int programId, void *fence);
bool RLSharedGpuPopPendingProgramFence(void **fenceOut);

// Increment the refcount for a GL object in the CURRENT context share-group.
// This does NOT assign or change owner. Owner is set by RegisterObject/explicit transfer.
void RLSharedGpuRetainObject(RLSharedGpuObjectType type, unsigned int id);

// Decrement the refcount for a GL object in the CURRENT context share-group.
// When refcount reaches 0, a deferred delete record is enqueued.
void RLSharedGpuReleaseObject(RLSharedGpuObjectType type, unsigned int id);

// Context-addressed variants (do NOT require ctx to be current).
void RLSharedGpuRetainObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id);
void RLSharedGpuReleaseObjectOnContext(RLContext *ctx, RLSharedGpuObjectType type, unsigned int id);

// Pop one pending delete from the CURRENT context share-group.
// Returns true if an item was popped.
bool RLSharedGpuPopPendingDelete(RLSharedGpuObjectType *typeOut, unsigned int *idOut);

// Debug helper: dump current share-group state (live refs and pending deletes) to stderr.
// Safe to call only when a context belonging to the target share-group is current.
void RLSharedGpuDebugDumpState(const char *label);

// Debug tracing helpers for texture lifetime analysis (especially font atlas textures).
// sourceFile/sourceLine should usually pass __FILE__/__LINE__ from the callsite.
void RLSharedGpuSetTextureDebugLabel(unsigned int id, const char *label, const char *sourceFile, int sourceLine);
void RLSharedGpuTraceTextureRelease(unsigned int id, const char *sourceFile, int sourceLine, const char *reason);

// Lightweight validation helpers used by public API wrappers.
bool RLSharedGpuHasCurrentGroup(void);
bool RLSharedGpuHasContextGroup(RLContext *ctx);
bool RLSharedGpuContextUsesSharedTrackedScope(RLContext *ctx);
bool RLSharedGpuContextResolveTrackedScopeHandle(RLContext *ctx, void **groupHandleOut);
bool RLSharedGpuGroupUsesSharedTrackedScope(const void *groupHandle);
void RLSharedGpuGroupSetSharedTrackedScope(void *groupHandle);

// Runtime tracking policy for unregistered object retain/release handling.
void RLSharedGpuSetTrackingMode(RLSharedGpuTrackingModeInternal mode);
RLSharedGpuTrackingModeInternal RLSharedGpuGetTrackingMode(void);
void RLSharedGpuEnableCumulativeDiagStats(void);
void RLSharedGpuDisableCumulativeDiagStats(void);
bool RLSharedGpuIsCumulativeDiagStatsEnabled(void);
bool RLSharedGpuResetGroupDiagStatsForContextInternal(RLContext *ctx);

typedef struct RLSharedGpuTrackingRejectDiagStatsInternal {
    unsigned long long unregisteredRetainRejectCount;
    unsigned long long unregisteredReleaseRejectCount;
} RLSharedGpuTrackingRejectDiagStatsInternal;

RLSharedGpuTrackingRejectDiagStatsInternal RLSharedGpuGetTrackingRejectDiagStatsInternal(void);
void RLSharedGpuResetTrackingRejectDiagStatsInternal(void);

#ifdef __cplusplus
}
#endif

#endif  // RL_SHARED_GPU_H
