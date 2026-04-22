/**
 * @file persistence.h
 * @brief Document JSON persistence interface.
 */
#ifndef GLDRAW_DOCUMENT_PERSISTENCE_H
#define GLDRAW_DOCUMENT_PERSISTENCE_H

#include <document/document.h>

/**
 * @brief Save the document as a JSON file.
 * @param document Document to save.
 * @param path Output file path.
 * @return Non-zero on success, zero on failure.
 */
int document_save_json(const Document* document, const char* path);

/**
 * @brief Load the document from a JSON file.
 * @param document Target document (will be reset/overwritten before loading).
 * @param path Input file path.
 * @return Non-zero on success, zero on failure.
 */
int document_load_json(Document* document, const char* path);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_H */
