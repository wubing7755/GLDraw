#ifndef GLDRAW_DOCUMENT_PERSISTENCE_H
#define GLDRAW_DOCUMENT_PERSISTENCE_H

#include <document/document.h>

int document_save_json(const Document* document, const char* path);
int document_load_json(Document* document, const char* path);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_H */
