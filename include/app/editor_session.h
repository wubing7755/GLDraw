/**
 * @file editor_session.h
 * @brief Editor-session-only state helpers.
 */
#ifndef GLDRAW_APP_EDITOR_SESSION_H
#define GLDRAW_APP_EDITOR_SESSION_H

#include <document/document.h>
#include <model/selection.h>

/**
 * @brief Resolve the primary selected object from a document.
 * @param selection Selection set.
 * @param document Document to search.
 * @return Selected object or `NULL`.
 */
static inline GraphicObject* selection_set_primary_object(const SelectionSet* selection,
                                                          const Document* document)
{
    if (!selection || !document || selection->count <= 0) {
        return NULL;
    }

    return document_find_object(document, selection->ids[0]);
}

#endif /* GLDRAW_APP_EDITOR_SESSION_H */
