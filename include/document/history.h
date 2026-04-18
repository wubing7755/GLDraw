/**
 * @file history.h
 * @brief Snapshot-based undo/redo interfaces.
 *
 * Role in project:
 * - Captures deep copies of `Document` state before/after edits.
 * - Maintains bounded undo/redo stacks for reversible operations.
 *
 * Module relationships:
 * - Built on top of `document.h` and `object` cloning.
 * - Called by tools/UI whenever a user-visible edit is committed.
 *
 * Concurrency note:
 * - History stacks are mutable and unsynchronized; use from one thread only.
 */
#ifndef GLDRAW_DOCUMENT_HISTORY_H
#define GLDRAW_DOCUMENT_HISTORY_H

#include <document/document.h>

#define DOCUMENT_HISTORY_MAX_ENTRIES 128

/** Deep-copied document state stored in history stacks. */
typedef struct {
    GraphicObject** objects;
    int capacity;
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} DocumentSnapshot;

/** One undo/redo transaction pair (`before` -> `after`). */
typedef struct {
    DocumentSnapshot before; /**< Document state before the edit */
    DocumentSnapshot after;  /**< Document state after the edit */
} DocumentHistoryEntry;

/**
 * @brief Undo/redo history with bounded stacks.
 *
 * Concurrency note:
 * - Stacks are mutable and unsynchronized; use from one thread only.
 */
typedef struct DocumentHistory {
    DocumentHistoryEntry* undo_stack; /**< Uncommitted edits stack */
    DocumentHistoryEntry* redo_stack; /**< Reverted edits stack */
    int capacity;                      /**< Max entries per stack */
    int undo_count;                    /**< Number of entries in undo stack */
    int redo_count;                    /**< Number of entries in redo stack */
} DocumentHistory;

/** Initialize snapshot to empty state. Complexity: `O(1)`. */
void document_snapshot_init(DocumentSnapshot* snapshot);
/** Free all objects owned by snapshot. Complexity: `O(n)`. */
void document_snapshot_free(DocumentSnapshot* snapshot);
/** Deep-copy document into snapshot. Returns 0 on allocation/clone failure. Complexity: `O(n)`. */
int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document);

/** Allocate undo/redo stacks. Returns 0 on allocation failure. Complexity: `O(capacity)`. */
int document_history_init(DocumentHistory* history);
/** Release all stack entries and internal arrays. Complexity: `O(total_snapshots)`. */
void document_history_shutdown(DocumentHistory* history);
/** Push one transaction and clear redo stack. Complexity: `O(n + capacity)` worst-case. */
int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document);
/** Push lightweight scalar edit transaction without full document snapshots. Returns 1 on success, 0 on failure. */
int document_history_push_scalar_edit(DocumentHistory* history, const Document* document, ObjectId object_id, const char* key, float before_value, float after_value, unsigned int revision_before, unsigned int revision_after);
/** Push lightweight translate transaction for a fixed object-id set. Returns 1 on success, 0 on failure. */
int document_history_push_translate_edit(DocumentHistory* history, const Document* document, const ObjectId* object_ids, int object_count, Vec2 delta, unsigned int revision_before, unsigned int revision_after);
/** Apply latest undo entry. Returns 0 when stack empty/failure. Complexity: `O(n + capacity)` worst-case. */
int document_history_undo(DocumentHistory* history, Document* document);
/** Apply latest redo entry. Returns 0 when stack empty/failure. Complexity: `O(n + capacity)` worst-case. */
int document_history_redo(DocumentHistory* history, Document* document);
/** Check undo availability. Complexity: `O(1)`. */
int document_history_can_undo(const DocumentHistory* history);
/** Check redo availability. Complexity: `O(1)`. */
int document_history_can_redo(const DocumentHistory* history);

#endif /* GLDRAW_DOCUMENT_HISTORY_H */
