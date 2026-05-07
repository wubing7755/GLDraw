#ifndef GLDRAW_DOCUMENT_DOCUMENT_INTERNAL_H
#define GLDRAW_DOCUMENT_DOCUMENT_INTERNAL_H

#include <document/document.h>

#define DOCUMENT_DEFAULT_LAYER_ID 1u
#define DOCUMENT_DEFAULT_LAYER_NAME "Layer 1"
#define DOCUMENT_DEFAULT_OBJECT_CAPACITY 16
#define DOCUMENT_DEFAULT_LAYER_CAPACITY 4
#define DOCUMENT_SPATIAL_CELL_SIZE 256.0f
#define DOCUMENT_SPATIAL_MAX_AXIS_CELLS 64

void document_clear_objects(Document* document);
int document_reserve_objects(Document* document, int needed);
void document_reset_layers(Document* document);
int document_reserve_layers(Document* document, int needed);
void document_spatial_free(Document* document);
void document_spatial_invalidate(Document* document);
LayerId document_resolve_layer_id(const Document* document, LayerId layer_id);

#endif /* GLDRAW_DOCUMENT_DOCUMENT_INTERNAL_H */
