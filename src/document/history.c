#include <document/history.h>

#include <base/log.h>

#include <stdlib.h>
#include <string.h>

static void snapshot_move(DocumentSnapshot* dst, DocumentSnapshot* src)
{
    int i = 0;

    document_snapshot_free(dst);
    *dst = *src;

    for (i = 0; i < DOCUMENT_MAX_OBJECTS; ++i) {
        src->objects[i] = NULL;
    }
    src->count = 0;
    src->revision = 0;
    src->next_id = 0;
    src->selection.count = 0;
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

static void history_shift_left(DocumentHistoryEntry* entries, int* count)
{
    int i = 0;

    if (!entries || !count || *count <= 0) {
        return;
    }

    history_entry_free(&entries[0]);
    for (i = 1; i < *count; ++i) {
        entries[i - 1] = entries[i];
        history_entry_reset(&entries[i]);
    }
    (*count)--;
}

static int document_restore_snapshot(Document* document, const DocumentSnapshot* snapshot)
{
    GraphicObject* clones[DOCUMENT_MAX_OBJECTS];
    int i = 0;

    if (!document || !snapshot) {
        return 0;
    }

    memset(clones, 0, sizeof(clones));
    for (i = 0; i < snapshot->count; ++i) {
        clones[i] = object_clone(snapshot->objects[i]);
        if (!clones[i]) {
            int j = 0;
            for (j = 0; j < i; ++j) {
                object_destroy(clones[j]);
            }
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
        snapshot->objects[i] = NULL;
    }
    snapshot->count = 0;
    snapshot->selection.count = 0;
    snapshot->revision = 0;
    snapshot->next_id = 0;
}

int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document)
{
    int i = 0;

    if (!snapshot || !document) {
        return 0;
    }

    document_snapshot_free(snapshot);
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
    history->undo_count = 0;
    history->redo_count = 0;
    free(history->undo_stack);
    free(history->redo_stack);
    history->undo_stack = NULL;
    history->redo_stack = NULL;
    history->capacity = 0;
}

int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document)
{
    DocumentHistoryEntry entry;

    if (!history || !history->undo_stack || !history->redo_stack || !before || !after_document) {
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
        history->redo_count--;
    }

    if (history->undo_count >= history->capacity) {
        history_shift_left(history->undo_stack, &history->undo_count);
    }

    history->undo_stack[history->undo_count++] = entry;
    return 1;
}

int document_history_undo(DocumentHistory* history, Document* document)
{
    DocumentHistoryEntry entry;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || history->undo_count <= 0) {
        return 0;
    }

    entry = history->undo_stack[history->undo_count - 1];
    history_entry_reset(&history->undo_stack[history->undo_count - 1]);
    history->undo_count--;

    if (history->redo_count >= history->capacity) {
        history_shift_left(history->redo_stack, &history->redo_count);
    }

    if (!document_restore_snapshot(document, &entry.before)) {
        history_entry_free(&entry);
        return 0;
    }

    history->redo_stack[history->redo_count++] = entry;
    return 1;
}

int document_history_redo(DocumentHistory* history, Document* document)
{
    DocumentHistoryEntry entry;

    if (!history || !history->undo_stack || !history->redo_stack ||
        !document || history->redo_count <= 0) {
        return 0;
    }

    entry = history->redo_stack[history->redo_count - 1];
    history_entry_reset(&history->redo_stack[history->redo_count - 1]);
    history->redo_count--;

    if (history->undo_count >= history->capacity) {
        history_shift_left(history->undo_stack, &history->undo_count);
    }

    if (!document_restore_snapshot(document, &entry.after)) {
        history_entry_free(&entry);
        return 0;
    }

    history->undo_stack[history->undo_count++] = entry;
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
