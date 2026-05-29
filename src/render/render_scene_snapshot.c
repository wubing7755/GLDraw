#include "render_scene_snapshot.h"

static RenderSceneCacheKey render_scene_cache_key(const RenderSceneDesc* scene)
{
    RenderSceneCacheKey key;

    key.document_revision = scene && scene->document ? document_revision(scene->document) : 0u;
    key.overlay_revision = scene && scene->overlay_object
                               ? scene->overlay_object->revision
                               : 0u;
    key.selection_count = scene && scene->selection ? scene->selection->count : 0;
    key.selection_revision = scene && scene->selection ? scene->selection->revision : 0u;
    key.selection_preview_active = scene ? scene->selection_preview_active : 0;
    key.selection_preview_delta = scene ? scene->selection_preview_delta
                                        : (Vec2){0.0f, 0.0f};
    key.viewport = scene && scene->canvas ? canvas_view_viewport(scene->canvas)
                                          : (RectF){0.0f, 0.0f, 0.0f, 0.0f};
    key.center = scene && scene->canvas ? canvas_view_center(scene->canvas)
                                        : (Vec2){0.0f, 0.0f};
    key.zoom = scene && scene->canvas ? canvas_view_zoom(scene->canvas) : 0.0f;
    return key;
}

int render_scene_snapshot_capture(RenderSceneSnapshot* snapshot,
                                  const RenderSceneDesc* scene)
{
    if (!snapshot || !scene || !scene->document || !scene->canvas) {
        return 0;
    }

    snapshot->desc = *scene;
    snapshot->cache_key = render_scene_cache_key(scene);
    return 1;
}

int render_scene_snapshot_equal(const RenderSceneSnapshot* a,
                                const RenderSceneSnapshot* b)
{
    const RenderSceneCacheKey* a_key = a ? &a->cache_key : NULL;
    const RenderSceneCacheKey* b_key = b ? &b->cache_key : NULL;

    if (!a_key || !b_key) {
        return 0;
    }

    return a_key->document_revision == b_key->document_revision &&
           a_key->overlay_revision == b_key->overlay_revision &&
           a_key->selection_count == b_key->selection_count &&
           a_key->selection_revision == b_key->selection_revision &&
           a_key->selection_preview_active == b_key->selection_preview_active &&
           a_key->selection_preview_delta.x == b_key->selection_preview_delta.x &&
           a_key->selection_preview_delta.y == b_key->selection_preview_delta.y &&
           a_key->zoom == b_key->zoom &&
           a_key->center.x == b_key->center.x &&
           a_key->center.y == b_key->center.y &&
           a_key->viewport.x == b_key->viewport.x &&
           a_key->viewport.y == b_key->viewport.y &&
           a_key->viewport.w == b_key->viewport.w &&
           a_key->viewport.h == b_key->viewport.h;
}
