#ifndef GLDRAW_RENDER_RENDER_SCENE_SNAPSHOT_H
#define GLDRAW_RENDER_RENDER_SCENE_SNAPSHOT_H

#include <render/render_scene.h>

typedef struct RenderSceneCacheKey {
    unsigned int document_revision;
    unsigned int overlay_revision;
    int selection_count;
    uint64_t selection_revision;
    int selection_preview_active;
    RectF viewport;
    Vec2 center;
    Vec2 selection_preview_delta;
    float zoom;
} RenderSceneCacheKey;

typedef struct RenderSceneSnapshot {
    RenderSceneDesc desc;
    RenderSceneCacheKey cache_key;
} RenderSceneSnapshot;

int render_scene_snapshot_capture(RenderSceneSnapshot* snapshot,
                                  const RenderSceneDesc* scene);
int render_scene_snapshot_equal(const RenderSceneSnapshot* a,
                                const RenderSceneSnapshot* b);

#endif /* GLDRAW_RENDER_RENDER_SCENE_SNAPSHOT_H */
