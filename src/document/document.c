#include "document_internal.h"

#include <stdlib.h>
#include <string.h>

void document_init(Document *document) {
  if (!document) {
    return;
  }

  memset(document, 0, sizeof(*document));
  document->next_id = 1u;
  document->revision = 1u;
  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document_reset_layers(document);
}

void document_shutdown(Document *document) {
  if (!document) {
    return;
  }

  document_clear_objects(document);
  document_spatial_free(document);
  free(document->layers);
  document->layers = NULL;
  document->layer_count = 0;
  document->layer_capacity = 0;
  document->active_layer_id = 0u;
  document->next_layer_id = 0u;
  document->revision++;
}

void document_reset(Document *document) {
  if (!document) {
    return;
  }

  document_clear_objects(document);
  document_spatial_free(document);
  document->next_id = 1u;
  document->revision = 1u;
  document->spatial_cell_size = DOCUMENT_SPATIAL_CELL_SIZE;
  document_reset_layers(document);
}
