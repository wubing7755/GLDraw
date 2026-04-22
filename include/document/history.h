/**
 * @file history.h
 * @brief Document undo/redo (snapshot + lightweight edit) interface.
 */
#ifndef GLDRAW_DOCUMENT_HISTORY_H
#define GLDRAW_DOCUMENT_HISTORY_H

#include <document/document.h>

#define DOCUMENT_HISTORY_MAX_ENTRIES 128

/**
 * @struct DocumentSnapshot
 * @brief Deep-copy snapshot of the document.
 *
 * @member objects Deep-copied object array.
 * @member capacity Allocation capacity of `objects`.
 * @member count Object count.
 * @member revision Document revision at snapshot time.
 * @member next_id Next allocatable ID at snapshot time.
 * @member selection Selection set at snapshot time.
 */
typedef struct {
    GraphicObject** objects;
    int capacity;
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} DocumentSnapshot;

/**
 * @struct DocumentHistoryEntry
 * @brief A history entry based on complete snapshots (before -> after).
 *
 * @member before Snapshot before the edit.
 * @member after Snapshot after the edit.
 */
typedef struct {
    DocumentSnapshot before;
    DocumentSnapshot after;
} DocumentHistoryEntry;

/**
 * @struct DocumentHistory
 * @brief Undo/redo history stack.
 *
 * @member undo_stack Undo stack.
 * @member redo_stack Redo stack.
 * @member capacity Maximum entry count per stack.
 * @member undo_count Current undo stack entry count.
 * @member redo_count Current redo stack entry count.
 */
typedef struct DocumentHistory {
    DocumentHistoryEntry* undo_stack;
    DocumentHistoryEntry* redo_stack;
    int capacity;
    int undo_count;
    int redo_count;
} DocumentHistory;

/**
 * @brief Initialize the document snapshot to an empty state.
 * @param snapshot Snapshot to initialize.
 * @return No return value.
 */
void document_snapshot_init(DocumentSnapshot* snapshot);

/**
 * @brief Release object copies and internal buffers owned by the snapshot.
 * @param snapshot Target snapshot.
 * @return No return value.
 */
void document_snapshot_free(DocumentSnapshot* snapshot);

/**
 * @brief Capture a deep copy of the document into a snapshot.
 * @param snapshot Target snapshot (output).
 * @param document Source document.
 * @return Non-zero on success; zero on out-of-memory or clone failure.
 */
int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document);

/**
 * @brief Initialize the undo/redo history stack.
 * @param history Target history object.
 * @return Non-zero on success; zero on memory allocation failure.
 */
int document_history_init(DocumentHistory* history);

/**
 * @brief Release all entries in the history stack.
 * @param history Target history object.
 * @return No return value.
 */
void document_history_shutdown(DocumentHistory* history);

/**
 * @brief Push a complete snapshot edit record.
 *
 * Algorithm:
 * 1. Copy `before` snapshot as the undo starting point;
 * 2. Capture a snapshot from `after_document` as the undo ending point;
 * 3. Push onto the undo stack (evicting the oldest entry if necessary);
 * 4. Clear the redo stack to ensure linear history branching.
 *
 * @param history History object.
 * @param before Snapshot before the edit (contents are taken over by the history after the call).
 * @param after_document Document after the edit.
 * @return Non-zero on success, zero on failure.
 */
int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document);

/**
 * @brief Push a scalar property edit history record.
 *
 * This interface records single-field changes on objects to avoid the cost of full snapshots.
 *
 * @param history History object.
 * @param document Current document.
 * @param object_id Target object ID.
 * @param key Scalar field name.
 * @param before_value Value before the modification.
 * @param after_value Value after the modification.
 * @param revision_before Document revision before modification.
 * @param revision_after Document revision after modification.
 * @return Non-zero on success, zero on failure.
 */
int document_history_push_scalar_edit(DocumentHistory* history, const Document* document, ObjectId object_id, const char* key, float before_value, float after_value, unsigned int revision_before, unsigned int revision_after);

/**
 * @brief Push a batch translate edit history record.
 *
 * This interface records a set of object IDs and a translation vector;
 * undo/redo applies the translation in the opposite direction.
 *
 * @param history History object.
 * @param document Current document.
 * @param object_ids Object ID array.
 * @param object_count Object count.
 * @param delta Translation delta (in world coordinates).
 * @param revision_before Revision before modification.
 * @param revision_after Revision after modification.
 * @return Non-zero on success, zero on failure.
 */
int document_history_push_translate_edit(DocumentHistory* history, const Document* document, const ObjectId* object_ids, int object_count, Vec2 delta, unsigned int revision_before, unsigned int revision_after);

/**
 * @brief Perform an undo.
 *
 * Algorithm:
 * 1. Pop the top record from the undo stack;
 * 2. Apply the record to the document (full snapshot or lightweight edit);
 * 3. Move the record to the top of the redo stack;
 * 4. Return the result of the application.
 *
 * @param history History object.
 * @param document Target document.
 * @return Non-zero if there is something to undo and the application succeeded, zero otherwise.
 */
int document_history_undo(DocumentHistory* history, Document* document);

/**
 * @brief Perform a redo.
 *
 * The algorithm is symmetric to undo: pop from the redo stack, apply, push back onto the undo stack.
 *
 * @param history History object.
 * @param document Target document.
 * @return Non-zero if there is something to redo and the application succeeded, zero otherwise.
 */
int document_history_redo(DocumentHistory* history, Document* document);

/**
 * @brief Check whether undo is available.
 * @param history History object.
 * @return Non-zero if undo is available, zero otherwise.
 */
int document_history_can_undo(const DocumentHistory* history);

/**
 * @brief Check whether redo is available.
 * @param history History object.
 * @return Non-zero if redo is available, zero otherwise.
 */
int document_history_can_redo(const DocumentHistory* history);

#endif /* GLDRAW_DOCUMENT_HISTORY_H */
