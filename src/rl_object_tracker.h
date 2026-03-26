#ifndef RL_OBJECT_TRACKER_H
#define RL_OBJECT_TRACKER_H

#include "raylib.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RLTrackedObjectKind
{
    RL_TRACKED_OBJECT_FONT = 1,
    RL_TRACKED_OBJECT_TEXTURE = 2,
    RL_TRACKED_OBJECT_RENDER_TEXTURE = 3,
    RL_TRACKED_OBJECT_MODEL = 4,
    RL_TRACKED_OBJECT_MESH = 5,
    RL_TRACKED_OBJECT_MATERIAL = 6
} RLTrackedObjectKind;

typedef enum RLTrackedPromotionResult
{
    RL_TRACKED_PROMOTION_FAILED = 0,
    RL_TRACKED_PROMOTION_NO_CHANGES = 1,
    RL_TRACKED_PROMOTION_SUCCEEDED = 2
} RLTrackedPromotionResult;

bool RLTrackedObjectRetain(RLTrackedObjectKind kind, uint64_t key, RLContext *ownerContext, const void *snapshot, size_t snapshotSize);
bool RLTrackedObjectRelease(RLTrackedObjectKind kind, uint64_t key, void *outSnapshot, size_t outSnapshotSize, unsigned int *outRemainingRefCount);
bool RLTrackedObjectGetOwnerContext(RLTrackedObjectKind kind, uint64_t key, RLContext **outOwnerContext);
bool RLTrackedObjectIsOwnedByCurrentContext(RLTrackedObjectKind kind, uint64_t key);
bool RLTrackedObjectTryTransferOwner(RLTrackedObjectKind kind, uint64_t key, RLContext *targetContext);
RLTrackedPromotionResult RLTrackedObjectPromoteContextEntriesToShareGroup(RLContext *ctx, void *shareGroup);

// Resolve best-effort target window handle for render-thread handoff.
// If tracked owner context differs from current context, returns owner window handle when available.
void *RLResolveRenderThreadWindowHandleForTrackedObject(RLTrackedObjectKind kind, uint64_t key, const char *apiName);

#ifdef __cplusplus
}
#endif

#endif
