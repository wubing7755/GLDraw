#include "document_internal.h"

#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

static int document_spatial_add_entry(Document* document,
                                      int cell_index,
                                      int object_index)
{
    DocumentSpatialEntry* entries = NULL;
    int capacity = 0;

    if (!document || cell_index < 0 || cell_index >= document->spatial_cell_count) {
        return 0;
    }

    if (document->spatial_entry_count >= document->spatial_entry_capacity) {
        capacity =
            document->spatial_entry_capacity > 0 ? document->spatial_entry_capacity * 2 : 128;
        entries = (DocumentSpatialEntry*)realloc(
            document->spatial_entries, (size_t)capacity * sizeof(document->spatial_entries[0]));
        if (!entries) {
            return 0;
        }
        document->spatial_entries = entries;
        document->spatial_entry_capacity = capacity;
    }

    document->spatial_entries[document->spatial_entry_count].object_index = object_index;
    document->spatial_entries[document->spatial_entry_count].next =
        document->spatial_heads[cell_index];
    document->spatial_heads[cell_index] = document->spatial_entry_count;
    document->spatial_entry_count++;
    return 1;
}

static int document_spatial_rebuild(Document* document)
{
    RectF bounds = {0.0f, 0.0f, 0.0f, 0.0f};
    int first = 1;
    int i = 0;

    if (!document) {
        return 0;
    }
    if (document->spatial_revision == document->revision) {
        return 1;
    }

    document->spatial_entry_count = 0;
    if (document->count <= 0) {
        if (document->spatial_heads) {
            memset(document->spatial_heads, 0xff,
                   (size_t)document->spatial_cell_count * sizeof(document->spatial_heads[0]));
        }
        if (document->spatial_marks) {
            memset(document->spatial_marks, 0,
                   (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
        }
        document->spatial_bounds = bounds;
        document->spatial_revision = document->revision;
        return 1;
    }

    for (i = 0; i < document->count; ++i) {
        RectF object_bounds;

        if (!document->objects[i]) {
            continue;
        }

        object_bounds = object_get_bounds(document->objects[i]);
        if (first) {
            bounds = object_bounds;
            first = 0;
        } else {
            float left = bounds.x < object_bounds.x ? bounds.x : object_bounds.x;
            float bottom = bounds.y < object_bounds.y ? bounds.y : object_bounds.y;
            float right = rectf_right(&bounds) > rectf_right(&object_bounds)
                              ? rectf_right(&bounds)
                              : rectf_right(&object_bounds);
            float top = rectf_top(&bounds) > rectf_top(&object_bounds)
                            ? rectf_top(&bounds)
                            : rectf_top(&object_bounds);
            bounds.x = left;
            bounds.y = bottom;
            bounds.w = right - left;
            bounds.h = top - bottom;
        }
    }

    bounds.x -= 1.0f;
    bounds.y -= 1.0f;
    bounds.w += 2.0f;
    bounds.h += 2.0f;
    if (bounds.w < DOCUMENT_SPATIAL_CELL_SIZE) {
        bounds.w = DOCUMENT_SPATIAL_CELL_SIZE;
    }
    if (bounds.h < DOCUMENT_SPATIAL_CELL_SIZE) {
        bounds.h = DOCUMENT_SPATIAL_CELL_SIZE;
    }

    document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
    document->spatial_cols = (int)ceilf(bounds.w / document->spatial_cell_size);
    document->spatial_rows = (int)ceilf(bounds.h / document->spatial_cell_size);
    if (document->spatial_cols < 1) {
        document->spatial_cols = 1;
    }
    if (document->spatial_rows < 1) {
        document->spatial_rows = 1;
    }
    if (document->spatial_cols > DOCUMENT_SPATIAL_MAX_AXIS_CELLS) {
        document->spatial_cols = DOCUMENT_SPATIAL_MAX_AXIS_CELLS;
    }
    if (document->spatial_rows > DOCUMENT_SPATIAL_MAX_AXIS_CELLS) {
        document->spatial_rows = DOCUMENT_SPATIAL_MAX_AXIS_CELLS;
    }
    document->spatial_cell_count = document->spatial_cols * document->spatial_rows;
    document->spatial_bounds = bounds;

    {
        int* heads = (int*)realloc(document->spatial_heads,
                                   (size_t)document->spatial_cell_count *
                                       sizeof(document->spatial_heads[0]));
        unsigned int* marks =
            (unsigned int*)realloc(document->spatial_marks,
                                   (size_t)document->count *
                                       sizeof(document->spatial_marks[0]));
        if (!heads || !marks) {
            if (heads) {
                document->spatial_heads = heads;
            }
            if (marks) {
                document->spatial_marks = marks;
            }
            document_spatial_free(document);
            return 0;
        }
        document->spatial_heads = heads;
        document->spatial_marks = marks;
        document->spatial_mark_capacity = document->count;
    }

    memset(document->spatial_heads, 0xff,
           (size_t)document->spatial_cell_count * sizeof(document->spatial_heads[0]));
    memset(document->spatial_marks, 0,
           (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
    document->spatial_query_token = 0u;

    for (i = 0; i < document->count; ++i) {
        RectF object_bounds;
        int min_col = 0;
        int max_col = 0;
        int min_row = 0;
        int max_row = 0;
        int row = 0;
        int col = 0;

        if (!document->objects[i]) {
            continue;
        }

        object_bounds = object_get_bounds(document->objects[i]);
        min_col = (int)floorf((object_bounds.x - bounds.x) / document->spatial_cell_size);
        max_col = (int)floorf((rectf_right(&object_bounds) - bounds.x) /
                              document->spatial_cell_size);
        min_row = (int)floorf((object_bounds.y - bounds.y) / document->spatial_cell_size);
        max_row = (int)floorf((rectf_top(&object_bounds) - bounds.y) /
                              document->spatial_cell_size);
        if (min_col < 0) {
            min_col = 0;
        }
        if (min_row < 0) {
            min_row = 0;
        }
        if (max_col >= document->spatial_cols) {
            max_col = document->spatial_cols - 1;
        }
        if (max_row >= document->spatial_rows) {
            max_row = document->spatial_rows - 1;
        }

        for (row = min_row; row <= max_row; ++row) {
            for (col = min_col; col <= max_col; ++col) {
                if (!document_spatial_add_entry(document, row * document->spatial_cols + col, i)) {
                    document_spatial_free(document);
                    return 0;
                }
            }
        }
    }

    document->spatial_revision = document->revision;
    return 1;
}

static int rect_intersects(const RectF* a, const RectF* b)
{
    if (!a || !b) {
        return 0;
    }

    return !(rectf_right(a) < b->x || rectf_right(b) < a->x || rectf_top(a) < b->y ||
             rectf_top(b) < a->y);
}

void document_spatial_free(Document* document)
{
    if (!document) {
        return;
    }

    free(document->spatial_heads);
    free(document->spatial_entries);
    free(document->spatial_marks);
    document->spatial_heads = NULL;
    document->spatial_entries = NULL;
    document->spatial_marks = NULL;
    document->spatial_cols = 0;
    document->spatial_rows = 0;
    document->spatial_cell_count = 0;
    document->spatial_entry_count = 0;
    document->spatial_entry_capacity = 0;
    document->spatial_revision = 0u;
    document->spatial_query_token = 0u;
    document->spatial_bounds = (RectF){0.0f, 0.0f, 0.0f, 0.0f};
    document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
    document->spatial_mark_capacity = 0;
}

void document_spatial_invalidate(Document* document)
{
    if (!document) {
        return;
    }

    document->spatial_revision = 0u;
}

int document_query_visible_indices(const Document* document,
                                   RectF visible_rect,
                                   int* out_indices,
                                   int max_indices)
{
    Document* mutable_document = (Document*)document;
    int count = 0;
    int cell_min_col = 0;
    int cell_max_col = 0;
    int cell_min_row = 0;
    int cell_max_row = 0;
    int row = 0;
    int col = 0;

    if (!document || !out_indices || max_indices <= 0) {
        return 0;
    }

    if (!document_spatial_rebuild(mutable_document) || document->count <= 0 ||
        document->spatial_cell_count <= 0) {
        return 0;
    }

    if (document->spatial_query_token == 0xffffffffu) {
        memset(document->spatial_marks, 0,
               (size_t)document->spatial_mark_capacity * sizeof(document->spatial_marks[0]));
        mutable_document->spatial_query_token = 1u;
    } else {
        mutable_document->spatial_query_token++;
    }

    cell_min_col = (int)floorf((visible_rect.x - document->spatial_bounds.x) /
                               document->spatial_cell_size);
    cell_max_col = (int)floorf((rectf_right(&visible_rect) - document->spatial_bounds.x) /
                               document->spatial_cell_size);
    cell_min_row = (int)floorf((visible_rect.y - document->spatial_bounds.y) /
                               document->spatial_cell_size);
    cell_max_row = (int)floorf((rectf_top(&visible_rect) - document->spatial_bounds.y) /
                               document->spatial_cell_size);
    if (cell_min_col < 0) {
        cell_min_col = 0;
    }
    if (cell_min_row < 0) {
        cell_min_row = 0;
    }
    if (cell_max_col >= document->spatial_cols) {
        cell_max_col = document->spatial_cols - 1;
    }
    if (cell_max_row >= document->spatial_rows) {
        cell_max_row = document->spatial_rows - 1;
    }

    for (row = cell_min_row; row <= cell_max_row; ++row) {
        for (col = cell_min_col; col <= cell_max_col; ++col) {
            int entry_index = document->spatial_heads[row * document->spatial_cols + col];
            while (entry_index >= 0) {
                int object_index = document->spatial_entries[entry_index].object_index;
                const GraphicObject* object = document->objects[object_index];
                const DocumentLayer* layer =
                    object ? document_layer_find_const(document, object->layer_id) : NULL;

                if (object &&
                    document->spatial_marks[object_index] != document->spatial_query_token &&
                    layer && layer->visible) {
                    RectF object_bounds = object_get_bounds(object);
                    if (!rect_intersects(&visible_rect, &object_bounds)) {
                        entry_index = document->spatial_entries[entry_index].next;
                        continue;
                    }
                    document->spatial_marks[object_index] = document->spatial_query_token;
                    if (count < max_indices) {
                        out_indices[count++] = object_index;
                    }
                }
                entry_index = document->spatial_entries[entry_index].next;
            }
        }
    }

    return count;
}

int document_query_point_indices(const Document* document,
                                 Vec2 point,
                                 float tolerance,
                                 int* out_indices,
                                 int max_indices)
{
    RectF query_rect;

    query_rect.x = point.x - tolerance;
    query_rect.y = point.y - tolerance;
    query_rect.w = tolerance * 2.0f;
    query_rect.h = tolerance * 2.0f;
    return document_query_visible_indices(document, query_rect, out_indices, max_indices);
}
