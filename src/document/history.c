/**
 * @file history.c
 * @brief Snapshot-based undo/redo implementation.
 */
#include <document/history.h>

#include <base/log.h>
#include <base/math2d.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void history_entry_free(DocumentHistoryEntry* entry);
static void history_entry_reset(DocumentHistoryEntry* entry);

static void history_reset_scalar_edit(DocumentHistoryScalarEdit* edit)
{
    if (!edit) {
        return;
    }

    memset(edit, 0, sizeof(*edit));
}

static void history_reset_transform_edit(DocumentHistoryTransformEdit* edit)
{
    if (!edit) {
        return;
    }

    memset(edit, 0, sizeof(*edit));
}

static void history_reset_slot(DocumentHistory* history, int index, int undo_stack)
{
    if (!history || index < 0 || index >= history->capacity) {
        return;
    }

    if (undo_stack) {
        history->undo_kinds[index] = DOCUMENT_HISTORY_ENTRY_SNAPSHOT;
        history_reset_scalar_edit(&history->undo_scalar_edits[index]);
        history_reset_transform_edit(&history->undo_transform_edits[index]);
    } else {
        history->redo_kinds[index] = DOCUMENT_HISTORY_ENTRY_SNAPSHOT;
        history_reset_scalar_edit(&history->redo_scalar_edits[index]);
        history_reset_transform_edit(&history->redo_transform_edits[index]);
    }
}

static void snapshot_move(DocumentSnapshot* dst, DocumentSnapshot* src)
{
    document_snapshot_free(dst);
    *dst = *src;
    src->objects = NULL;
    src->capacity = 0;
    src->count = 0;
    src->revision = 0;
    src->next_id = 0;
    src->layer_count = 0;
    src->active_layer_id = 0u;
    src->next_layer_id = 0u;
}

static GraphicObject* history_find_scalar_object(Document* document, ObjectId id)
{
    if (!document || id == 0) {
        return NULL;
    }

    return document_find_object(document, id);
}

static int history_apply_scalar_edit_undo(Document* document,
                                          SelectionSet* selection,
                                          const DocumentHistoryScalarEdit* edit)
{
    GraphicObject* object = history_find_scalar_object(document, edit ? edit->object_id : 0);

    if (!document || !selection || !edit || !object) {
        return 0;
    }
    if (!object_set_scalar(object, edit->key, edit->before_value)) {
        return 0;
    }
    document->revision = edit->revision_before;
    *selection = edit->selection;
    return 1;
}

static int history_apply_scalar_edit_redo(Document* document,
                                          SelectionSet* selection,
                                          const DocumentHistoryScalarEdit* edit)
{
    GraphicObject* object = history_find_scalar_object(document, edit ? edit->object_id : 0);

    if (!document || !selection || !edit || !object) {
        return 0;
    }
    if (!object_set_scalar(object, edit->key, edit->after_value)) {
        return 0;
    }
    document->revision = edit->revision_after;
    *selection = edit->selection;
    return 1;
}

static int history_apply_transform(Document* document,
                                   SelectionSet* selection,
                                   const DocumentHistoryTransformEdit* edit,
                                   Vec2 delta,
                                   unsigned int target_revision)
{
    int i = 0;

    if (!document || !selection || !edit) {
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
    *selection = edit->selection;
    return 1;
}

static int history_apply_transform_edit_undo(Document* document,
                                             SelectionSet* selection,
                                             const DocumentHistoryTransformEdit* edit)
{
    Vec2 undo_delta = {0.0f, 0.0f};

    if (!edit) {
        return 0;
    }

    undo_delta.x = -edit->delta.x;
    undo_delta.y = -edit->delta.y;
    return history_apply_transform(document, selection, edit, undo_delta, edit->revision_before);
}

static int history_apply_transform_edit_redo(Document* document,
                                             SelectionSet* selection,
                                             const DocumentHistoryTransformEdit* edit)
{
    if (!edit) {
        return 0;
    }

    return history_apply_transform(document, selection, edit, edit->delta, edit->revision_after);
}

static void history_clear_redo(DocumentHistory* history)
{
    while (history->redo_count > 0) {
        history_entry_free(&history->redo_stack[history->redo_count - 1]);
        history_entry_reset(&history->redo_stack[history->redo_count - 1]);
        history->redo_count--;
        history_reset_slot(history, history->redo_count, 0);
    }
}

static void history_evict_oldest(DocumentHistory* history, int undo_stack)
{
    int i = 0;
    DocumentHistoryEntry* entries = undo_stack ? history->undo_stack : history->redo_stack;
    DocumentHistoryEntryKind* kinds = undo_stack ? history->undo_kinds : history->redo_kinds;
    DocumentHistoryScalarEdit* scalar_edits =
        undo_stack ? history->undo_scalar_edits : history->redo_scalar_edits;
    DocumentHistoryTransformEdit* transform_edits =
        undo_stack ? history->undo_transform_edits : history->redo_transform_edits;
    int* count = undo_stack ? &history->undo_count : &history->redo_count;

    if (!history || !entries || !kinds || !scalar_edits || !transform_edits || *count <= 0) {
        return;
    }

    history_entry_free(&entries[0]);
    for (i = 1; i < *count; ++i) {
        entries[i - 1] = entries[i];
        history_entry_reset(&entries[i]);
        kinds[i - 1] = kinds[i];
        scalar_edits[i - 1] = scalar_edits[i];
        transform_edits[i - 1] = transform_edits[i];
    }
    (*count)--;
    history_reset_slot(history, *count, undo_stack);
}

static int history_push_scalar_edit_impl(DocumentHistory* history,
                                         const Document* document,
                                         const SelectionSet* selection,
                                         ObjectId object_id,
                                         const char* key,
                                         float before_value,
                                         float after_value,
                                         unsigned int revision_before,
                                         unsigned int revision_after)
{
    DocumentHistoryEntry entry;
    DocumentHistoryScalarEdit scalar = {0};

    if (!history || !document || !selection || !key || key[0] == '\0' || object_id == 0) {
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);
    entry.before_selection = *selection;
    entry.after_selection = *selection;

    scalar.object_id = object_id;
    scalar.before_value = before_value;
    scalar.after_value = after_value;
    scalar.revision_before = revision_before;
    scalar.revision_after = revision_after;
    scalar.selection = *selection;
    snprintf(scalar.key, sizeof(scalar.key), "%s", key);

    history_clear_redo(history);
    if (history->undo_count >= history->capacity) {
        history_evict_oldest(history, 1);
    }

    history->undo_stack[history->undo_count] = entry;
    history->undo_kinds[history->undo_count] = DOCUMENT_HISTORY_ENTRY_SCALAR_EDIT;
    history->undo_scalar_edits[history->undo_count] = scalar;
    history_reset_transform_edit(&history->undo_transform_edits[history->undo_count]);
    history->undo_count++;
    return 1;
}

static int history_push_transform_edit_impl(DocumentHistory* history,
                                            const Document* document,
                                            const SelectionSet* selection,
                                            const ObjectId* object_ids,
                                            int object_count,
                                            Vec2 delta,
                                            unsigned int revision_before,
                                            unsigned int revision_after)
{
    DocumentHistoryEntry entry;
    DocumentHistoryTransformEdit transform = {0};
    int i = 0;

    if (!history || !document || !selection || !object_ids ||
        object_count <= 0 || object_count > DOCUMENT_MAX_SELECTION) {
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);
    entry.before_selection = *selection;
    entry.after_selection = *selection;

    transform.object_count = object_count;
    transform.delta = delta;
    transform.revision_before = revision_before;
    transform.revision_after = revision_after;
    transform.selection = *selection;
    for (i = 0; i < object_count; ++i) {
        transform.object_ids[i] = object_ids[i];
    }

    history_clear_redo(history);
    if (history->undo_count >= history->capacity) {
        history_evict_oldest(history, 1);
    }

    history->undo_stack[history->undo_count] = entry;
    history->undo_kinds[history->undo_count] = DOCUMENT_HISTORY_ENTRY_TRANSFORM_EDIT;
    history_reset_scalar_edit(&history->undo_scalar_edits[history->undo_count]);
    history->undo_transform_edits[history->undo_count] = transform;
    history->undo_count++;
    return 1;
}

static void history_entry_free(DocumentHistoryEntry* entry)
{
    document_snapshot_free(&entry->before);
    document_snapshot_free(&entry->after);
}

static void history_entry_reset(DocumentHistoryEntry* entry)
{
    if (!entry) {
        return;
    }

    document_snapshot_init(&entry->before);
    document_snapshot_init(&entry->after);
    selection_set_clear(&entry->before_selection);
    selection_set_clear(&entry->after_selection);
}

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

    document_reset(document);
    for (i = 0; i < snapshot->count; ++i) {
        document->objects[i] = clones[i];
    }
    document->count = snapshot->count;
    document->revision = snapshot->revision;
    document->next_id = snapshot->next_id;
    memcpy(document->layers, snapshot->layers, sizeof(snapshot->layers));
    document->layer_count = snapshot->layer_count;
    document->active_layer_id = snapshot->active_layer_id;
    document->next_layer_id = snapshot->next_layer_id;
    free(clones);
    return 1;
}

void document_snapshot_init(DocumentSnapshot* snapshot)
{
    if (snapshot) {
        memset(snapshot, 0, sizeof(*snapshot));
    }
}

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
    snapshot->revision = 0;
    snapshot->next_id = 0;
    snapshot->layer_count = 0;
    snapshot->active_layer_id = 0u;
    snapshot->next_layer_id = 0u;
}

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
    memcpy(snapshot->layers, document->layers, sizeof(snapshot->layers));
    snapshot->layer_count = document->layer_count;
    snapshot->active_layer_id = document->active_layer_id;
    snapshot->next_layer_id = document->next_layer_id;

    for (i = 0; i < document->count; ++i) {
        snapshot->objects[i] = object_clone(document->objects[i]);
        if (!snapshot->objects[i]) {
            document_snapshot_free(snapshot);
            return 0;
        }
    }

    return 1;
}

int document_history_init(DocumentHistory* history)
{
    size_t cap = 0u;

    if (!history) {
        return 0;
    }

    memset(history, 0, sizeof(*history));
    history->capacity = DOCUMENT_HISTORY_MAX_ENTRIES;
    history->undo_stack = history_alloc_entries(history->capacity);
    history->redo_stack = history_alloc_entries(history->capacity);

    cap = (size_t)history->capacity;
    history->undo_kinds = (DocumentHistoryEntryKind*)calloc(cap, sizeof(*history->undo_kinds));
    history->redo_kinds = (DocumentHistoryEntryKind*)calloc(cap, sizeof(*history->redo_kinds));
    history->undo_scalar_edits =
        (DocumentHistoryScalarEdit*)calloc(cap, sizeof(*history->undo_scalar_edits));
    history->redo_scalar_edits =
        (DocumentHistoryScalarEdit*)calloc(cap, sizeof(*history->redo_scalar_edits));
    history->undo_transform_edits =
        (DocumentHistoryTransformEdit*)calloc(cap, sizeof(*history->undo_transform_edits));
    history->redo_transform_edits =
        (DocumentHistoryTransformEdit*)calloc(cap, sizeof(*history->redo_transform_edits));

    if (!history->undo_stack || !history->redo_stack ||
        !history->undo_kinds || !history->redo_kinds ||
        !history->undo_scalar_edits || !history->redo_scalar_edits ||
        !history->undo_transform_edits || !history->redo_transform_edits) {
        LOG_ERROR("%s", "Failed to allocate document history stacks");
        document_history_shutdown(history);
        return 0;
    }

    return 1;
}

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

    free(history->undo_stack);
    free(history->redo_stack);
    free(history->undo_kinds);
    free(history->redo_kinds);
    free(history->undo_scalar_edits);
    free(history->redo_scalar_edits);
    free(history->undo_transform_edits);
    free(history->redo_transform_edits);
    memset(history, 0, sizeof(*history));
}

int document_history_push(DocumentHistory* history,
                          DocumentSnapshot* before,
                          const SelectionSet* before_selection,
                          const Document* after_document,
                          const SelectionSet* after_selection)
{
    DocumentHistoryEntry entry;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !before || !before_selection || !after_document || !after_selection) {
        if (before) {
            document_snapshot_free(before);
            document_snapshot_init(before);
        }
        return 0;
    }

    document_snapshot_init(&entry.before);
    document_snapshot_init(&entry.after);
    entry.before_selection = *before_selection;
    entry.after_selection = *after_selection;

    if (!document_snapshot_capture(&entry.after, after_document)) {
        document_snapshot_free(before);
        document_snapshot_init(before);
        return 0;
    }

    snapshot_move(&entry.before, before);
    history_clear_redo(history);
    if (history->undo_count >= history->capacity) {
        history_evict_oldest(history, 1);
    }

    history->undo_stack[history->undo_count] = entry;
    history->undo_kinds[history->undo_count] = DOCUMENT_HISTORY_ENTRY_SNAPSHOT;
    history_reset_scalar_edit(&history->undo_scalar_edits[history->undo_count]);
    history_reset_transform_edit(&history->undo_transform_edits[history->undo_count]);
    history->undo_count++;
    return 1;
}

int document_history_push_scalar_edit(DocumentHistory* history,
                                      const Document* document,
                                      const SelectionSet* selection,
                                      ObjectId object_id,
                                      const char* key,
                                      float before_value,
                                      float after_value,
                                      unsigned int revision_before,
                                      unsigned int revision_after)
{
    if (!history || !document || !selection || !key || key[0] == '\0' || object_id == 0) {
        return 0;
    }
    if (fabsf(after_value - before_value) <= 1e-6f) {
        return 1;
    }

    return history_push_scalar_edit_impl(history,
                                         document,
                                         selection,
                                         object_id,
                                         key,
                                         before_value,
                                         after_value,
                                         revision_before,
                                         revision_after);
}

int document_history_push_translate_edit(DocumentHistory* history,
                                         const Document* document,
                                         const SelectionSet* selection,
                                         const ObjectId* object_ids,
                                         int object_count,
                                         Vec2 delta,
                                         unsigned int revision_before,
                                         unsigned int revision_after)
{
    if (!history || !document || !selection || !object_ids ||
        object_count <= 0 || object_count > DOCUMENT_MAX_SELECTION) {
        return 0;
    }
    if (vec2_length_sq(delta) <= 1e-6f) {
        return 1;
    }

    return history_push_transform_edit_impl(history,
                                            document,
                                            selection,
                                            object_ids,
                                            object_count,
                                            delta,
                                            revision_before,
                                            revision_after);
}

int document_history_undo(DocumentHistory* history,
                          Document* document,
                          SelectionSet* selection)
{
    DocumentHistoryEntry entry;
    DocumentHistoryEntryKind kind = DOCUMENT_HISTORY_ENTRY_SNAPSHOT;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || !selection || history->undo_count <= 0) {
        return 0;
    }

    kind = history->undo_kinds[history->undo_count - 1];
    entry = history->undo_stack[history->undo_count - 1];
    history_entry_reset(&history->undo_stack[history->undo_count - 1]);
    history->undo_count--;

    if (history->redo_count >= history->capacity) {
        history_evict_oldest(history, 0);
    }

    if (kind == DOCUMENT_HISTORY_ENTRY_SCALAR_EDIT) {
        if (!history_apply_scalar_edit_undo(document, selection,
                                            &history->undo_scalar_edits[history->undo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (kind == DOCUMENT_HISTORY_ENTRY_TRANSFORM_EDIT) {
        if (!history_apply_transform_edit_undo(document, selection,
                                               &history->undo_transform_edits[history->undo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else {
        if (!document_restore_snapshot(document, &entry.before)) {
            history_entry_free(&entry);
            return 0;
        }
        *selection = entry.before_selection;
    }

    history->redo_stack[history->redo_count] = entry;
    history->redo_kinds[history->redo_count] = kind;
    history->redo_scalar_edits[history->redo_count] = history->undo_scalar_edits[history->undo_count];
    history->redo_transform_edits[history->redo_count] =
        history->undo_transform_edits[history->undo_count];
    history->redo_count++;
    history_reset_slot(history, history->undo_count, 1);
    return 1;
}

int document_history_redo(DocumentHistory* history,
                          Document* document,
                          SelectionSet* selection)
{
    DocumentHistoryEntry entry;
    DocumentHistoryEntryKind kind = DOCUMENT_HISTORY_ENTRY_SNAPSHOT;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || !selection || history->redo_count <= 0) {
        return 0;
    }

    kind = history->redo_kinds[history->redo_count - 1];
    entry = history->redo_stack[history->redo_count - 1];
    history_entry_reset(&history->redo_stack[history->redo_count - 1]);
    history->redo_count--;

    if (history->undo_count >= history->capacity) {
        history_evict_oldest(history, 1);
    }

    if (kind == DOCUMENT_HISTORY_ENTRY_SCALAR_EDIT) {
        if (!history_apply_scalar_edit_redo(document, selection,
                                            &history->redo_scalar_edits[history->redo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else if (kind == DOCUMENT_HISTORY_ENTRY_TRANSFORM_EDIT) {
        if (!history_apply_transform_edit_redo(document, selection,
                                               &history->redo_transform_edits[history->redo_count])) {
            history_entry_free(&entry);
            return 0;
        }
    } else {
        if (!document_restore_snapshot(document, &entry.after)) {
            history_entry_free(&entry);
            return 0;
        }
        *selection = entry.after_selection;
    }

    history->undo_stack[history->undo_count] = entry;
    history->undo_kinds[history->undo_count] = kind;
    history->undo_scalar_edits[history->undo_count] = history->redo_scalar_edits[history->redo_count];
    history->undo_transform_edits[history->undo_count] =
        history->redo_transform_edits[history->redo_count];
    history->undo_count++;
    history_reset_slot(history, history->redo_count, 0);
    return 1;
}

int document_history_can_undo(const DocumentHistory* history)
{
    return history && history->undo_count > 0;
}

int document_history_can_redo(const DocumentHistory* history)
{
    return history && history->redo_count > 0;
}
