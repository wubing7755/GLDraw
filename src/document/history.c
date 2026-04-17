/**
 * @file history.c
 * @brief Snapshot-based undo/redo implementation.
 *
 * Role in project:
 * - Captures deep document snapshots before/after edits.
 * - Applies snapshots for undo and redo with bounded stack capacity.
 *
 * Module relationships:
 * - Depends on document/object cloning and destruction.
 * - Called by tools/UI commit paths for user-visible edits.
 */
#include <document/history.h>

#include <base/log.h>
#include <base/math2d.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    HISTORY_ENTRY_SNAPSHOT = 0,
    HISTORY_ENTRY_SCALAR_EDIT,
    HISTORY_ENTRY_TRANSFORM_EDIT
} HistoryEntryKind;

typedef struct {
    ObjectId object_id;
    char key[32];
    float before_value;
    float after_value;
    unsigned int revision_before;
    unsigned int revision_after;
} HistoryScalarEdit;

typedef struct {
    ObjectId object_ids[DOCUMENT_MAX_SELECTION];
    int object_count;
    Vec2 delta;
    unsigned int revision_before;
    unsigned int revision_after;
} HistoryTransformEdit;

typedef struct HistoryInternals {
    DocumentHistory* owner;
    HistoryEntryKind* undo_kinds;
    HistoryEntryKind* redo_kinds;
    HistoryScalarEdit* undo_scalar_edits;
    HistoryScalarEdit* redo_scalar_edits;
    HistoryTransformEdit* undo_transform_edits;
    HistoryTransformEdit* redo_transform_edits;
    struct HistoryInternals* next;
} HistoryInternals;

static HistoryInternals* g_history_internals_head = NULL;

static void history_entry_free(DocumentHistoryEntry* entry);
static void history_entry_reset(DocumentHistoryEntry* entry);

/** Find internal sidecar metadata for one history instance. */
static HistoryInternals* history_find_internals(const DocumentHistory* history)
{
    HistoryInternals* cursor = g_history_internals_head;
    while (cursor) {
        if (cursor->owner == history) {
            return cursor;
        }
        cursor = cursor->next;
    }
    return NULL;
}

/** Free sidecar metadata arrays for one history instance. */
static void history_free_internals(HistoryInternals* internals)
{
    if (!internals) {
        return;
    }

    free(internals->undo_kinds);
    free(internals->redo_kinds);
    free(internals->undo_scalar_edits);
    free(internals->redo_scalar_edits);
    free(internals->undo_transform_edits);
    free(internals->redo_transform_edits);
    internals->undo_kinds = NULL;
    internals->redo_kinds = NULL;
    internals->undo_scalar_edits = NULL;
    internals->redo_scalar_edits = NULL;
    internals->undo_transform_edits = NULL;
    internals->redo_transform_edits = NULL;
}

/** Remove and destroy sidecar metadata for history. */
static void history_unregister_internals(DocumentHistory* history)
{
    HistoryInternals* prev = NULL;
    HistoryInternals* cursor = g_history_internals_head;

    while (cursor) {
        if (cursor->owner == history) {
            if (prev) {
                prev->next = cursor->next;
            } else {
                g_history_internals_head = cursor->next;
            }
            history_free_internals(cursor);
            free(cursor);
            return;
        }
        prev = cursor;
        cursor = cursor->next;
    }
}

/** Allocate and register sidecar metadata arrays for history. */
static int history_register_internals(DocumentHistory* history)
{
    HistoryInternals* internals = NULL;
    size_t cap = 0u;

    if (!history || history->capacity <= 0) {
        return 0;
    }
    if (history_find_internals(history)) {
        return 1;
    }

    internals = (HistoryInternals*)calloc(1u, sizeof(*internals));
    if (!internals) {
        return 0;
    }

    cap = (size_t)history->capacity;
    internals->undo_kinds = (HistoryEntryKind*)calloc(cap, sizeof(*internals->undo_kinds));
    internals->redo_kinds = (HistoryEntryKind*)calloc(cap, sizeof(*internals->redo_kinds));
    internals->undo_scalar_edits = (HistoryScalarEdit*)calloc(cap, sizeof(*internals->undo_scalar_edits));
    internals->redo_scalar_edits = (HistoryScalarEdit*)calloc(cap, sizeof(*internals->redo_scalar_edits));
    internals->undo_transform_edits = (HistoryTransformEdit*)calloc(cap, sizeof(*internals->undo_transform_edits));
    internals->redo_transform_edits = (HistoryTransformEdit*)calloc(cap, sizeof(*internals->redo_transform_edits));
    if (!internals->undo_kinds || !internals->redo_kinds ||
        !internals->undo_scalar_edits || !internals->redo_scalar_edits ||
        !internals->undo_transform_edits || !internals->redo_transform_edits) {
        history_free_internals(internals);
        free(internals);
        return 0;
    }

    internals->owner = history;
    internals->next = g_history_internals_head;
    g_history_internals_head = internals;
    return 1;
}

/** Get sidecar metadata for history. */
static HistoryInternals* history_get_internals(const DocumentHistory* history)
{
    return history_find_internals(history);
}

/** Move snapshot ownership from `src` to `dst`. Complexity: `O(1)`. */
static void snapshot_move(DocumentSnapshot* dst, DocumentSnapshot* src)
{
    document_snapshot_free(dst);
    *dst = *src;
    src->objects = NULL;
    src->capacity = 0;
    src->count = 0;
    src->revision = 0;
    src->next_id = 0;
    src->selection.count = 0;
}

/** Find target object for scalar edit by stable ID. */
static GraphicObject* history_find_scalar_object(Document* document, ObjectId id)
{
    if (!document || id == 0) {
        return NULL;
    }
    return document_find_object(document, id);
}

/** Apply scalar-edit undo by writing previous value to the same object/key. */
static int history_apply_scalar_edit_undo(Document* document, const HistoryScalarEdit* edit)
{
    GraphicObject* object = history_find_scalar_object(document, edit ? edit->object_id : 0);

    if (!document || !edit || !object) {
        return 0;
    }
    if (!object_set_scalar(object, edit->key, edit->before_value)) {
        return 0;
    }
    document->revision = edit->revision_before;
    return 1;
}

/** Apply scalar-edit redo by writing new value to the same object/key. */
static int history_apply_scalar_edit_redo(Document* document, const HistoryScalarEdit* edit)
{
    GraphicObject* object = history_find_scalar_object(document, edit ? edit->object_id : 0);

    if (!document || !edit || !object) {
        return 0;
    }
    if (!object_set_scalar(object, edit->key, edit->after_value)) {
        return 0;
    }
    document->revision = edit->revision_after;
    return 1;
}

/** Apply one transform edit by translating all recorded objects with `delta`. */
static int history_apply_transform(Document* document,
                                   const HistoryTransformEdit* edit,
                                   Vec2 delta,
                                   unsigned int target_revision)
{
    int i = 0;

    if (!document || !edit) {
        return 0;
    }

    for (i = 0; i < edit->object_count; ++i) {
        GraphicObject* object = document_find_object(document, edit->object_ids[i]);
        if (!object) {
            return 0;
        }
        object_translate(object, delta);
    }

    document->revision = target_revision;
    return 1;
}

/** Apply transform edit undo (reverse delta). */
static int history_apply_transform_edit_undo(Document* document, const HistoryTransformEdit* edit)
{
    Vec2 undo_delta = {0.0f, 0.0f};

    if (!edit) {
        return 0;
    }

    undo_delta.x = -edit->delta.x;
    undo_delta.y = -edit->delta.y;
    return history_apply_transform(document, edit, undo_delta, edit->revision_before);
}

/** Apply transform edit redo (forward delta). */
static int history_apply_transform_edit_redo(Document* document, const HistoryTransformEdit* edit)
{
    if (!edit) {
        return 0;
    }
    return history_apply_transform(document, edit, edit->delta, edit->revision_after);
}

/** Push one scalar edit entry to undo stack and clear redo chain. */
static int history_push_scalar_edit(DocumentHistory* history,
                                    const Document* document,
                                    ObjectId object_id,
                                    const char* key,
                                    float before_value,
                                    float after_value,
                                    unsigned int revision_before,
                                    unsigned int revision_after)
{
    DocumentHistoryEntry entry;
    HistoryInternals* internals = history_get_internals(history);
    HistoryScalarEdit scalar = {0};

    if (!history || !document || !internals || !key || key[0] == '\0' || object_id == 0) {
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);

    scalar.object_id = object_id;
    scalar.before_value = before_value;
    scalar.after_value = after_value;
    scalar.revision_before = revision_before;
    scalar.revision_after = revision_after;
    snprintf(scalar.key, sizeof(scalar.key), "%s", key);

    while (history->redo_count > 0) {
        history_entry_free(&history->redo_stack[history->redo_count - 1]);
        history_entry_reset(&history->redo_stack[history->redo_count - 1]);
        internals->redo_kinds[history->redo_count - 1] = HISTORY_ENTRY_SNAPSHOT;
        memset(&internals->redo_scalar_edits[history->redo_count - 1], 0, sizeof(HistoryScalarEdit));
        memset(&internals->redo_transform_edits[history->redo_count - 1], 0, sizeof(HistoryTransformEdit));
        history->redo_count--;
    }

    if (history->undo_count >= history->capacity) {
        int i = 0;
        history_entry_free(&history->undo_stack[0]);
        for (i = 1; i < history->undo_count; ++i) {
            history->undo_stack[i - 1] = history->undo_stack[i];
            history_entry_reset(&history->undo_stack[i]);
            internals->undo_kinds[i - 1] = internals->undo_kinds[i];
            internals->undo_scalar_edits[i - 1] = internals->undo_scalar_edits[i];
            internals->undo_transform_edits[i - 1] = internals->undo_transform_edits[i];
        }
        history->undo_count--;
    }

    history->undo_stack[history->undo_count] = entry;
    internals->undo_kinds[history->undo_count] = HISTORY_ENTRY_SCALAR_EDIT;
    internals->undo_scalar_edits[history->undo_count] = scalar;
    memset(&internals->undo_transform_edits[history->undo_count], 0, sizeof(HistoryTransformEdit));
    history->undo_count++;
    return 1;
}

/** Push one transform edit entry to undo stack and clear redo chain. */
static int history_push_transform_edit(DocumentHistory* history,
                                       const Document* document,
                                       const ObjectId* object_ids,
                                       int object_count,
                                       Vec2 delta,
                                       unsigned int revision_before,
                                       unsigned int revision_after)
{
    DocumentHistoryEntry entry;
    HistoryInternals* internals = history_get_internals(history);
    HistoryTransformEdit transform = {0};
    int i = 0;

    if (!history || !document || !internals || !object_ids ||
        object_count <= 0 || object_count > DOCUMENT_MAX_SELECTION) {
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);

    transform.object_count = object_count;
    transform.delta = delta;
    transform.revision_before = revision_before;
    transform.revision_after = revision_after;
    for (i = 0; i < object_count; ++i) {
        transform.object_ids[i] = object_ids[i];
    }

    while (history->redo_count > 0) {
        history_entry_free(&history->redo_stack[history->redo_count - 1]);
        history_entry_reset(&history->redo_stack[history->redo_count - 1]);
        internals->redo_kinds[history->redo_count - 1] = HISTORY_ENTRY_SNAPSHOT;
        memset(&internals->redo_scalar_edits[history->redo_count - 1], 0, sizeof(HistoryScalarEdit));
        memset(&internals->redo_transform_edits[history->redo_count - 1], 0, sizeof(HistoryTransformEdit));
        history->redo_count--;
    }

    if (history->undo_count >= history->capacity) {
        history_entry_free(&history->undo_stack[0]);
        for (i = 1; i < history->undo_count; ++i) {
            history->undo_stack[i - 1] = history->undo_stack[i];
            history_entry_reset(&history->undo_stack[i]);
            internals->undo_kinds[i - 1] = internals->undo_kinds[i];
            internals->undo_scalar_edits[i - 1] = internals->undo_scalar_edits[i];
            internals->undo_transform_edits[i - 1] = internals->undo_transform_edits[i];
        }
        history->undo_count--;
    }

    history->undo_stack[history->undo_count] = entry;
    internals->undo_kinds[history->undo_count] = HISTORY_ENTRY_TRANSFORM_EDIT;
    memset(&internals->undo_scalar_edits[history->undo_count], 0, sizeof(HistoryScalarEdit));
    internals->undo_transform_edits[history->undo_count] = transform;
    history->undo_count++;
    return 1;
}

/** Free both snapshots in a history entry. Complexity: `O(n_before + n_after)`. */
static void history_entry_free(DocumentHistoryEntry* entry)
{
    document_snapshot_free(&entry->before);
    document_snapshot_free(&entry->after);
}

/** Reset history entry to empty snapshots. Complexity: `O(1)`. */
static void history_entry_reset(DocumentHistoryEntry* entry)
{
    if (!entry) {
        return;
    }

    document_snapshot_init(&entry->before);
    document_snapshot_init(&entry->after);
}

/** Allocate and initialize history entry array. Complexity: `O(capacity)`. */
static DocumentHistoryEntry* history_alloc_entries(int capacity)
{
    DocumentHistoryEntry* entries = NULL;
    int i = 0;

    if (capacity <= 0) {
        return NULL;
    }

    entries = (DocumentHistoryEntry*)calloc((size_t)capacity, sizeof(*entries));
    if (!entries) {
        return NULL;
    }

    for (i = 0; i < capacity; ++i) {
        history_entry_reset(&entries[i]);
    }

    return entries;
}

/**
 * @brief Replace document contents with deep-cloned snapshot objects.
 * @return `1` on success, `0` on validation/allocation/clone failure.
 *
 * Risk note:
 * - Uses temporary clone array first; document is only replaced after all clones
 *   succeed, avoiding partial state corruption on failure.
 *
 * Complexity: `O(n)`.
 */
static int document_restore_snapshot(Document* document, const DocumentSnapshot* snapshot)
{
    GraphicObject** clones = NULL;
    int i = 0;

    if (!document || !snapshot) {
        return 0;
    }

    if (snapshot->count > DOCUMENT_MAX_OBJECTS) {
        return 0;
    }

    if (snapshot->count > 0) {
        clones = (GraphicObject**)calloc((size_t)snapshot->count, sizeof(*clones));
        if (!clones) {
            return 0;
        }
    }

    for (i = 0; i < snapshot->count; ++i) {
        clones[i] = object_clone(snapshot->objects[i]);
        if (!clones[i]) {
            int j = 0;
            for (j = 0; j < i; ++j) {
                object_destroy(clones[j]);
            }
            free(clones);
            return 0;
        }
    }

    document_shutdown(document);
    for (i = 0; i < snapshot->count; ++i) {
        document->objects[i] = clones[i];
    }
    document->count = snapshot->count;
    document->revision = snapshot->revision;
    document->next_id = snapshot->next_id;
    document->selection = snapshot->selection;
    free(clones);
    return 1;
}

/** Initialize snapshot to zeroed state. Complexity: `O(1)`. */
void document_snapshot_init(DocumentSnapshot* snapshot)
{
    if (snapshot) {
        memset(snapshot, 0, sizeof(*snapshot));
    }
}

/** Free heap memory owned by snapshot. Complexity: `O(n)`. */
void document_snapshot_free(DocumentSnapshot* snapshot)
{
    int i = 0;

    if (!snapshot) {
        return;
    }

    for (i = 0; i < snapshot->count; ++i) {
        object_destroy(snapshot->objects[i]);
    }
    free(snapshot->objects);
    snapshot->objects = NULL;
    snapshot->capacity = 0;
    snapshot->count = 0;
    snapshot->selection.count = 0;
    snapshot->revision = 0;
    snapshot->next_id = 0;
}

/** Capture deep copy of document into snapshot. Complexity: `O(n)`. */
int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document)
{
    int i = 0;
    int capacity = 0;

    if (!snapshot || !document) {
        return 0;
    }

    document_snapshot_free(snapshot);
    capacity = document->count;
    if (capacity > 0) {
        snapshot->objects = (GraphicObject**)calloc((size_t)capacity, sizeof(*snapshot->objects));
        if (!snapshot->objects) {
            return 0;
        }
    }

    snapshot->capacity = capacity;
    snapshot->count = document->count;
    snapshot->revision = document->revision;
    snapshot->next_id = document->next_id;
    snapshot->selection = document->selection;

    for (i = 0; i < document->count; ++i) {
        snapshot->objects[i] = object_clone(document->objects[i]);
        if (!snapshot->objects[i]) {
            document_snapshot_free(snapshot);
            return 0;
        }
    }

    return 1;
}

/** Initialize history stacks and capacity. Complexity: `O(capacity)`. */
int document_history_init(DocumentHistory* history)
{
    if (!history) {
        return 0;
    }

    memset(history, 0, sizeof(*history));
    history->capacity = DOCUMENT_HISTORY_MAX_ENTRIES;
    history->undo_stack = history_alloc_entries(history->capacity);
    history->redo_stack = history_alloc_entries(history->capacity);

    if (!history->undo_stack || !history->redo_stack) {
        LOG_ERROR("%s", "Failed to allocate document history stacks");
        document_history_shutdown(history);
        return 0;
    }
    if (!history_register_internals(history)) {
        LOG_ERROR("%s", "Failed to allocate document history internals");
        document_history_shutdown(history);
        return 0;
    }

    return 1;
}

/** Free all history entries and internal arrays. Complexity: `O(total_snapshots)`. */
void document_history_shutdown(DocumentHistory* history)
{
    int i = 0;

    if (!history) {
        return;
    }

    for (i = 0; i < history->undo_count; ++i) {
        history_entry_free(&history->undo_stack[i]);
    }
    for (i = 0; i < history->redo_count; ++i) {
        history_entry_free(&history->redo_stack[i]);
    }
    history->undo_count = 0;
    history->redo_count = 0;
    free(history->undo_stack);
    free(history->redo_stack);
    history->undo_stack = NULL;
    history->redo_stack = NULL;
    history->capacity = 0;
    history_unregister_internals(history);
}

/**
 * @brief Push edit transaction (`before` + captured `after`) to undo stack.
 * @return `1` on success, `0` on failure.
 *
 * Why clear redo:
 * - Any new forward edit invalidates previous redo chain by definition.
 *
 * Complexity: `O(n + capacity)` worst-case.
 */
int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document)
{
    DocumentHistoryEntry entry;
    HistoryInternals* internals = history_get_internals(history);

    if (!history || !history->undo_stack || !history->redo_stack ||
        !before || !after_document || !internals) {
        if (before) {
            document_snapshot_free(before);
            document_snapshot_init(before);
        }
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);

    if (!document_snapshot_capture(&entry.after, after_document)) {
        document_snapshot_free(&entry.after);
        document_snapshot_free(before);
        document_snapshot_init(before);
        return 0;
    }

    snapshot_move(&entry.before, before);

    while (history->redo_count > 0) {
        history_entry_free(&history->redo_stack[history->redo_count - 1]);
        history_entry_reset(&history->redo_stack[history->redo_count - 1]);
        internals->redo_kinds[history->redo_count - 1] = HISTORY_ENTRY_SNAPSHOT;
        memset(&internals->redo_scalar_edits[history->redo_count - 1], 0, sizeof(HistoryScalarEdit));
        memset(&internals->redo_transform_edits[history->redo_count - 1], 0, sizeof(HistoryTransformEdit));
        history->redo_count--;
    }

    if (history->undo_count >= history->capacity) {
        int i = 0;
        history_entry_free(&history->undo_stack[0]);
        for (i = 1; i < history->undo_count; ++i) {
            history->undo_stack[i - 1] = history->undo_stack[i];
            history_entry_reset(&history->undo_stack[i]);
            internals->undo_kinds[i - 1] = internals->undo_kinds[i];
            internals->undo_scalar_edits[i - 1] = internals->undo_scalar_edits[i];
            internals->undo_transform_edits[i - 1] = internals->undo_transform_edits[i];
        }
        history->undo_count--;
    }

    history->undo_stack[history->undo_count] = entry;
    internals->undo_kinds[history->undo_count] = HISTORY_ENTRY_SNAPSHOT;
    memset(&internals->undo_scalar_edits[history->undo_count], 0, sizeof(HistoryScalarEdit));
    memset(&internals->undo_transform_edits[history->undo_count], 0, sizeof(HistoryTransformEdit));
    history->undo_count++;
    return 1;
}

/**
 * @brief Push lightweight scalar edit transaction without capturing full document snapshots.
 * @return `1` on success, `0` on validation/allocation failure.
 */
int document_history_push_scalar_edit(DocumentHistory* history,
                                      const Document* document,
                                      ObjectId object_id,
                                      const char* key,
                                      float before_value,
                                      float after_value,
                                      unsigned int revision_before,
                                      unsigned int revision_after)
{
    if (!history || !document || !key || key[0] == '\0' || object_id == 0) {
        return 0;
    }
    if (fabsf(after_value - before_value) <= 1e-6f) {
        return 1;
    }
    return history_push_scalar_edit(history,
                                    document,
                                    object_id,
                                    key,
                                    before_value,
                                    after_value,
                                    revision_before,
                                    revision_after);
}

/**
 * @brief Push lightweight translate transaction for a fixed object-id set.
 * @return `1` on success, `0` on validation/allocation failure.
 */
int document_history_push_translate_edit(DocumentHistory* history,
                                         const Document* document,
                                         const ObjectId* object_ids,
                                         int object_count,
                                         Vec2 delta,
                                         unsigned int revision_before,
                                         unsigned int revision_after)
{
    if (!history || !document || !object_ids ||
        object_count <= 0 || object_count > DOCUMENT_MAX_SELECTION) {
        return 0;
    }
    if (vec2_length_sq(delta) <= 1e-6f) {
        return 1;
    }

    return history_push_transform_edit(history,
                                       document,
                                       object_ids,
                                       object_count,
                                       delta,
                                       revision_before,
                                       revision_after);
}

/** Undo latest transaction and move entry to redo stack. Complexity: `O(n + capacity)` worst-case. */
int document_history_undo(DocumentHistory* history, Document* document)
{
    DocumentHistoryEntry entry;
    HistoryInternals* internals = history_get_internals(history);
    HistoryEntryKind kind = HISTORY_ENTRY_SNAPSHOT;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || history->undo_count <= 0 || !internals) {
        return 0;
    }

    kind = internals->undo_kinds[history->undo_count - 1];
    entry = history->undo_stack[history->undo_count - 1];
    history_entry_reset(&history->undo_stack[history->undo_count - 1]);
    history->undo_count--;

    if (history->redo_count >= history->capacity) {
        int i = 0;
        history_entry_free(&history->redo_stack[0]);
        for (i = 1; i < history->redo_count; ++i) {
            history->redo_stack[i - 1] = history->redo_stack[i];
            history_entry_reset(&history->redo_stack[i]);
            internals->redo_kinds[i - 1] = internals->redo_kinds[i];
            internals->redo_scalar_edits[i - 1] = internals->redo_scalar_edits[i];
            internals->redo_transform_edits[i - 1] = internals->redo_transform_edits[i];
        }
        history->redo_count--;
    }

    if (kind == HISTORY_ENTRY_SCALAR_EDIT) {
        if (!history_apply_scalar_edit_undo(document, &internals->undo_scalar_edits[history->undo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (kind == HISTORY_ENTRY_TRANSFORM_EDIT) {
        if (!history_apply_transform_edit_undo(document, &internals->undo_transform_edits[history->undo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (!document_restore_snapshot(document, &entry.before)) {
        history_entry_free(&entry);
        return 0;
    }

    history->redo_stack[history->redo_count] = entry;
    internals->redo_kinds[history->redo_count] = kind;
    internals->redo_scalar_edits[history->redo_count] = internals->undo_scalar_edits[history->undo_count];
    internals->redo_transform_edits[history->redo_count] = internals->undo_transform_edits[history->undo_count];
    history->redo_count++;
    internals->undo_kinds[history->undo_count] = HISTORY_ENTRY_SNAPSHOT;
    memset(&internals->undo_scalar_edits[history->undo_count], 0, sizeof(HistoryScalarEdit));
    memset(&internals->undo_transform_edits[history->undo_count], 0, sizeof(HistoryTransformEdit));
    return 1;
}

/** Redo latest undone transaction and move entry back to undo stack. Complexity: `O(n + capacity)` worst-case. */
int document_history_redo(DocumentHistory* history, Document* document)
{
    DocumentHistoryEntry entry;
    HistoryInternals* internals = history_get_internals(history);
    HistoryEntryKind kind = HISTORY_ENTRY_SNAPSHOT;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || history->redo_count <= 0 || !internals) {
        return 0;
    }

    kind = internals->redo_kinds[history->redo_count - 1];
    entry = history->redo_stack[history->redo_count - 1];
    history_entry_reset(&history->redo_stack[history->redo_count - 1]);
    history->redo_count--;

    if (history->undo_count >= history->capacity) {
        int i = 0;
        history_entry_free(&history->undo_stack[0]);
        for (i = 1; i < history->undo_count; ++i) {
            history->undo_stack[i - 1] = history->undo_stack[i];
            history_entry_reset(&history->undo_stack[i]);
            internals->undo_kinds[i - 1] = internals->undo_kinds[i];
            internals->undo_scalar_edits[i - 1] = internals->undo_scalar_edits[i];
            internals->undo_transform_edits[i - 1] = internals->undo_transform_edits[i];
        }
        history->undo_count--;
    }

    if (kind == HISTORY_ENTRY_SCALAR_EDIT) {
        if (!history_apply_scalar_edit_redo(document, &internals->redo_scalar_edits[history->redo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (kind == HISTORY_ENTRY_TRANSFORM_EDIT) {
        if (!history_apply_transform_edit_redo(document, &internals->redo_transform_edits[history->redo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (!document_restore_snapshot(document, &entry.after)) {
        history_entry_free(&entry);
        return 0;
    }

    history->undo_stack[history->undo_count] = entry;
    internals->undo_kinds[history->undo_count] = kind;
    internals->undo_scalar_edits[history->undo_count] = internals->redo_scalar_edits[history->redo_count];
    internals->undo_transform_edits[history->undo_count] = internals->redo_transform_edits[history->redo_count];
    history->undo_count++;
    internals->redo_kinds[history->redo_count] = HISTORY_ENTRY_SNAPSHOT;
    memset(&internals->redo_scalar_edits[history->redo_count], 0, sizeof(HistoryScalarEdit));
    memset(&internals->redo_transform_edits[history->redo_count], 0, sizeof(HistoryTransformEdit));
    return 1;
}

/** Check if undo stack has entries. Complexity: `O(1)`. */
int document_history_can_undo(const DocumentHistory* history)
{
    return history && history->undo_count > 0;
}

/** Check if redo stack has entries. Complexity: `O(1)`. */
int document_history_can_redo(const DocumentHistory* history)
{
    return history && history->redo_count > 0;
}
