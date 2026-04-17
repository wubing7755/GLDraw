/**
 * @file persistence.h
 * @brief Document JSON save/load interface.
 *
 * Role in project:
 * - Serializes in-memory `Document` into stable JSON format.
 * - Deserializes JSON back to runtime objects with IDs and style data.
 *
 * Module relationships:
 * - Called by application and menu actions.
 * - Depends on document/object APIs for object construction and lookup.
 */
#ifndef GLDRAW_DOCUMENT_PERSISTENCE_H
#define GLDRAW_DOCUMENT_PERSISTENCE_H

#include <document/document.h>

/** Save document to JSON file path. Returns 1 on success, 0 on I/O/serialization failure. */
int document_save_json(const Document* document, const char* path);
/** Load document from JSON file path. Returns 1 on success, 0 on I/O/parse/validation failure. */
int document_load_json(Document* document, const char* path);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_H */
