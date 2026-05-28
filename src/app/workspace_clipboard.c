/**
 * @file workspace_clipboard.c
 * @brief Clipboard copy, paste, and cut behavior for workspace selections.
 */
#include <app/workspace_clipboard.h>

#include <app/workspace_internal.h>
#include <base/math2d.h>
#include <canvas/canvas_view.h>
#include <commands/command.h>
#include <document/document.h>

#include <stdlib.h>

#define CLIPBOARD_PASTE_OFFSET_PIXELS 10.0f

static int workspace_clipboard_reserve(Workspace* workspace, int needed)
{
    GraphicObject** objects = NULL;
    int capacity = 0;

    if (!workspace || needed <= 0) {
        return workspace != NULL;
    }
    if (needed <= workspace->session.clipboard_capacity) {
        return 1;
    }

    capacity = workspace->session.clipboard_capacity > 0
                   ? workspace->session.clipboard_capacity
                   : 8;
    while (capacity < needed) {
        capacity *= 2;
    }

    objects = (GraphicObject**)realloc(workspace->session.clipboard_objects,
                                       (size_t)capacity * sizeof(workspace->session.clipboard_objects[0]));
    if (!objects) {
        return 0;
    }

    workspace->session.clipboard_objects = objects;
    workspace->session.clipboard_capacity = capacity;
    return 1;
}

static void workspace_clipboard_prune_noneditable_selection(Workspace* workspace)
{
    int i = 0;

    if (!workspace) {
        return;
    }

    while (i < workspace->session.selection.count) {
        ObjectId id = workspace->session.selection.ids[i];
        if (!document_find_object(&workspace->core.document, id) ||
            document_object_is_locked(&workspace->core.document, id)) {
            selection_set_remove(&workspace->session.selection, id);
            continue;
        }
        ++i;
    }
}

static int workspace_clipboard_delete_selection(Workspace* workspace)
{
    Command* command = NULL;

    if (!workspace || workspace->session.selection.count <= 0) {
        return 0;
    }
    workspace_clipboard_prune_noneditable_selection(workspace);
    if (workspace->session.selection.count <= 0) {
        return 0;
    }

    command = command_create_delete_selection(&workspace->core.document,
                                              &workspace->session.selection);
    if (!command ||
        !command_executor_execute(&workspace->core.commands,
                                  command,
                                  &workspace->core.document)) {
        return 0;
    }

    selection_set_clear(&workspace->session.selection);
    workspace_sync_document_dirty(workspace);
    return 1;
}

int workspace_clipboard_copy_selection(Workspace* workspace)
{
    GraphicObject** clipboard_objects = NULL;
    int selection_count = 0;
    int i = 0;

    if (!workspace) {
        return 0;
    }

    selection_count = workspace->session.selection.count;
    if (selection_count <= 0) {
        return 0;
    }

    clipboard_objects = (GraphicObject**)calloc((size_t)selection_count, sizeof(clipboard_objects[0]));
    if (!clipboard_objects) {
        return 0;
    }

    for (i = 0; i < selection_count; ++i) {
        GraphicObject* source = document_find_object(&workspace->core.document,
                                                     workspace->session.selection.ids[i]);

        if (!source) {
            break;
        }

        clipboard_objects[i] = object_clone(source);
        if (!clipboard_objects[i]) {
            break;
        }
    }

    if (i != selection_count) {
        int j = 0;
        for (j = 0; j < selection_count; ++j) {
            object_destroy(clipboard_objects[j]);
        }
        free(clipboard_objects);
        return 0;
    }

    workspace_clear_clipboard(workspace);
    if (!workspace_clipboard_reserve(workspace, selection_count)) {
        for (i = 0; i < selection_count; ++i) {
            object_destroy(clipboard_objects[i]);
        }
        free(clipboard_objects);
        return 0;
    }
    for (i = 0; i < selection_count; ++i) {
        workspace->session.clipboard_objects[i] = clipboard_objects[i];
    }
    workspace->session.clipboard_count = selection_count;
    workspace->session.clipboard_paste_serial = 0;
    free(clipboard_objects);
    return 1;
}

int workspace_clipboard_paste(Workspace* workspace)
{
    SelectionSet pasted_selection = {0};
    Command* command = NULL;
    Vec2 paste_delta = vec2_make(0.0f, 0.0f);
    float world_offset = 0.0f;
    int paste_count = 0;

    if (!workspace) {
        return 0;
    }

    paste_count = workspace->session.clipboard_count;
    if (paste_count <= 0) {
        return 0;
    }
    if (document_layer_is_locked(&workspace->core.document,
                                 document_active_layer_id(&workspace->core.document))) {
        return 0;
    }

    world_offset = canvas_view_world_tolerance_for_pixels(&workspace->core.canvas,
                                                          CLIPBOARD_PASTE_OFFSET_PIXELS);
    paste_delta = vec2_make(world_offset * (float)(workspace->session.clipboard_paste_serial + 1u),
                            -world_offset * (float)(workspace->session.clipboard_paste_serial + 1u));
    command = command_create_paste_objects(&workspace->core.document,
                                           workspace->session.clipboard_objects,
                                           paste_count,
                                           paste_delta,
                                           document_active_layer_id(&workspace->core.document),
                                           &pasted_selection);
    if (!command ||
        !command_executor_execute(&workspace->core.commands,
                                  command,
                                  &workspace->core.document)) {
        selection_set_shutdown(&pasted_selection);
        return 0;
    }

    selection_set_clear(&workspace->session.selection);
    if (!selection_set_copy(&workspace->session.selection, &pasted_selection)) {
        selection_set_shutdown(&pasted_selection);
        return 0;
    }
    selection_set_shutdown(&pasted_selection);
    workspace->session.clipboard_paste_serial++;
    workspace_sync_document_dirty(workspace);
    return 1;
}

int workspace_clipboard_cut_selection(Workspace* workspace)
{
    if (!workspace || workspace->session.selection.count <= 0) {
        return 0;
    }
    workspace_clipboard_prune_noneditable_selection(workspace);
    if (workspace->session.selection.count <= 0) {
        return 0;
    }

    if (!workspace_clipboard_copy_selection(workspace)) {
        return 0;
    }

    if (!command_executor_begin_transaction(&workspace->core.commands)) {
        workspace_clear_clipboard(workspace);
        return 0;
    }

    if (!workspace_clipboard_delete_selection(workspace)) {
        command_executor_rollback_transaction(&workspace->core.commands,
                                              &workspace->core.document);
        workspace_clear_clipboard(workspace);
        return 0;
    }

    if (!command_executor_commit_transaction(&workspace->core.commands)) {
        workspace_clear_clipboard(workspace);
        return 0;
    }

    return 1;
}
