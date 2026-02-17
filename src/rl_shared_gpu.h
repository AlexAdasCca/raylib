#ifndef RL_SHARED_GPU_H
#define RL_SHARED_GPU_H

// Internal helper for share-group wide GPU object lifetime management.
//
// This module provides:
// - Share-group binding for RLContext instances (same share-group => shared GL object namespace)
// - A lightweight reference counter per GL object id (per share-group)
// - Deferred deletion queue: the last release enqueues a delete, actual glDelete* happens
//   on a thread with a current OpenGL context (drained by rlgl).

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

// Bind ctx to a share-group. If shareWithCtx is non-NULL, ctx joins shareWithCtx's group.
// Otherwise a new share-group is created for ctx.
// Safe to call multiple times.
void RLSharedGpuContextBindShareGroup(RLContext *ctx, RLContext *shareWithCtx);

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

// Convenience helpers to retain/release framebuffer and its registered depth attachment together.
void RLSharedGpuRetainFramebufferTree(unsigned int framebufferId);
void RLSharedGpuReleaseFramebufferTree(unsigned int framebufferId);

// Increment the refcount for a GL object in the CURRENT context share-group.
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

#ifdef __cplusplus
}
#endif

#endif  // RL_SHARED_GPU_H
