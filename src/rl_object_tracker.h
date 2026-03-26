#ifndef RL_OBJECT_TRACKER_H
#define RL_OBJECT_TRACKER_H

#include "raylib.h"

#include <stddef.h>
#include <stdint.h>

typedef enum RLTrackedObjectKind
{
    RL_TRACKED_OBJECT_FONT = 1,
    RL_TRACKED_OBJECT_TEXTURE = 2,
    RL_TRACKED_OBJECT_RENDER_TEXTURE = 3,
    RL_TRACKED_OBJECT_MODEL = 4,
    RL_TRACKED_OBJECT_MESH = 5,
    RL_TRACKED_OBJECT_MATERIAL = 6
} RLTrackedObjectKind;

bool RLTrackedObjectRetain(RLTrackedObjectKind kind, uint64_t key, RLContext *ownerContext, const void *snapshot, size_t snapshotSize);
bool RLTrackedObjectRelease(RLTrackedObjectKind kind, uint64_t key, void *outSnapshot, size_t outSnapshotSize, unsigned int *outRemainingRefCount);
bool RLTrackedObjectGetOwnerContext(RLTrackedObjectKind kind, uint64_t key, RLContext **outOwnerContext);
bool RLTrackedObjectIsOwnedByCurrentContext(RLTrackedObjectKind kind, uint64_t key);
bool RLTrackedObjectTryTransferOwner(RLTrackedObjectKind kind, uint64_t key, RLContext *targetContext);

// Resolve best-effort target window handle for render-thread handoff.
// If tracked owner context differs from current context, returns owner window handle when available.
void *RLResolveRenderThreadWindowHandleForTrackedObject(RLTrackedObjectKind kind, uint64_t key, const char *apiName);

#endif
