#ifndef GLDRAW_DOCUMENT_HISTORY_H
#define GLDRAW_DOCUMENT_HISTORY_H

#include <document/document.h>

#define DOCUMENT_HISTORY_MAX_ENTRIES 128

typedef struct {
    GraphicObject* objects[DOCUMENT_MAX_OBJECTS];
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} DocumentSnapshot;

typedef struct {
    DocumentSnapshot before;
    DocumentSnapshot after;
} DocumentHistoryEntry;

typedef struct DocumentHistory {
    DocumentHistoryEntry* undo_stack;
    DocumentHistoryEntry* redo_stack;
    int capacity;
    int undo_count;
    int redo_count;
} DocumentHistory;

void document_snapshot_init(DocumentSnapshot* snapshot);
void document_snapshot_free(DocumentSnapshot* snapshot);
int document_snapshot_capture(DocumentSnapshot* snapshot, const Document* document);

int document_history_init(DocumentHistory* history);
void document_history_shutdown(DocumentHistory* history);
int document_history_push(DocumentHistory* history, DocumentSnapshot* before, const Document* after_document);
int document_history_undo(DocumentHistory* history, Document* document);
int document_history_redo(DocumentHistory* history, Document* document);
int document_history_can_undo(const DocumentHistory* history);
int document_history_can_redo(const DocumentHistory* history);

#endif /* GLDRAW_DOCUMENT_HISTORY_H */
