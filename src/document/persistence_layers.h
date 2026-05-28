#ifndef GLDRAW_DOCUMENT_PERSISTENCE_LAYERS_H
#define GLDRAW_DOCUMENT_PERSISTENCE_LAYERS_H

#include "persistence_json.h"

#include <document/document.h>

int parse_layers_array(JsonParser* parser, Document* document);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_LAYERS_H */
