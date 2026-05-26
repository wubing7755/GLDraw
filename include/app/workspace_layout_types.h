/**
 * @file workspace_layout_types.h
 * @brief Shared workspace layout snapshot value types.
 */
#ifndef GLDRAW_APP_WORKSPACE_LAYOUT_TYPES_H
#define GLDRAW_APP_WORKSPACE_LAYOUT_TYPES_H

#include <base/types.h>

typedef struct WorkspaceLayout {
    RectF window_bounds;
    RectF canvas_content_bounds;
    RectF appbar_bounds;
    RectF rail_bounds;
    RectF panel_bounds;
    RectF status_bounds;
    unsigned int layout_revision;
} WorkspaceLayout;

#endif /* GLDRAW_APP_WORKSPACE_LAYOUT_TYPES_H */
