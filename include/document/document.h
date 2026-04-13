#ifndef GLDRAW_DOCUMENT_DOCUMENT_H
#define GLDRAW_DOCUMENT_DOCUMENT_H

#include <document/object.h>

#define DOCUMENT_MAX_OBJECTS 1024
#define DOCUMENT_MAX_SELECTION 128

typedef struct {
    ObjectId ids[DOCUMENT_MAX_SELECTION];
    int count;
} SelectionSet;

typedef struct Document {
    GraphicObject* objects[DOCUMENT_MAX_OBJECTS];
    int count;
    unsigned int revision;
    ObjectId next_id;
    SelectionSet selection;
} Document;

void document_init(Document* document);
void document_shutdown(Document* document);
void document_reset(Document* document);

int document_add_object(Document* document, GraphicObject* object);
int document_append_object_with_id(Document* document, GraphicObject* object, ObjectId id);
GraphicObject* document_find_object(const Document* document, ObjectId id);
GraphicObject* document_get_object_at(const Document* document, int index);
int document_remove_object(Document* document, ObjectId id);
void document_delete_selection(Document* document);
void document_touch(Document* document);
ObjectId document_max_id(const Document* document);

void document_clear_selection(Document* document);
int document_selection_contains(const Document* document, ObjectId id);
int document_selection_add(Document* document, ObjectId id);
void document_selection_remove(Document* document, ObjectId id);
void document_selection_toggle(Document* document, ObjectId id);
GraphicObject* document_primary_selection(const Document* document);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_H */
