#ifndef GLDRAW_DOCUMENT_PERSISTENCE_OBJECTS_H
#define GLDRAW_DOCUMENT_PERSISTENCE_OBJECTS_H

#include "persistence_json.h"

#include <document/document.h>

int parse_objects_array(JsonParser* parser, Document* document);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_OBJECTS_H */
