#ifndef GLDRAW_UI_EDITOR_VIEWMODEL_H
#define GLDRAW_UI_EDITOR_VIEWMODEL_H

#include <app/command_types.h>
#include <app/workspace.h>
#include <document/object.h>
#include <tools/tool.h>
#include <ui/editor_action.h>

struct Workspace;

#define EDITOR_VIEWMODEL_MAX_COMMANDS (EDITOR_COMMAND_MODAL_CANCEL + 1)
#define EDITOR_VIEWMODEL_SHORTCUT_CAPACITY 64
#define EDITOR_VIEWMODEL_TOOLTIP_CAPACITY 128
#define EDITOR_VIEWMODEL_NAME_CAPACITY 64
#define EDITOR_VIEWMODEL_PATH_CAPACITY GLDRAW_PATH_MAX
#define EDITOR_VIEWMODEL_STATUS_CAPACITY 256

typedef struct EditorCommandView {
    int available;
    char shortcut[EDITOR_VIEWMODEL_SHORTCUT_CAPACITY];
    char unavailable_reason[EDITOR_VIEWMODEL_TOOLTIP_CAPACITY];
} EditorCommandView;

typedef struct EditorToolView {
    char id[EDITOR_ACTION_ID_CAPACITY];
    char name[EDITOR_VIEWMODEL_NAME_CAPACITY];
    char tooltip[EDITOR_VIEWMODEL_TOOLTIP_CAPACITY];
    char unavailable_reason[EDITOR_VIEWMODEL_TOOLTIP_CAPACITY];
    char icon[EDITOR_VIEWMODEL_NAME_CAPACITY];
    char shortcut[EDITOR_VIEWMODEL_SHORTCUT_CAPACITY];
    int active;
    int available;
    EditorCommand command;
} EditorToolView;

typedef struct EditorPropertyView {
    ObjectId object_id;
    char name[EDITOR_ACTION_PROPERTY_CAPACITY];
    GraphicPropertyType type;
    int editable;
    float min_value;
    float max_value;
    float step;
    float inc_per_pixel;
    float value;
} EditorPropertyView;

typedef struct EditorLayerView {
    LayerId id;
    char name[EDITOR_VIEWMODEL_NAME_CAPACITY];
    int visible;
    int locked;
    int active;
    int object_count;
} EditorLayerView;

typedef struct EditorDocumentSummaryView {
    int object_count;
    int selection_count;
    int undo_count;
    int redo_count;
    float zoom_percent;
    int document_dirty;
    char current_document_path[EDITOR_VIEWMODEL_PATH_CAPACITY];
    char status_message[EDITOR_VIEWMODEL_STATUS_CAPACITY];
    char active_tool_label[EDITOR_VIEWMODEL_NAME_CAPACITY];
    char selection_type_name[EDITOR_VIEWMODEL_NAME_CAPACITY];
} EditorDocumentSummaryView;

typedef struct EditorViewModel {
    EditorDocumentSummaryView summary;
    EditorCommandView commands[EDITOR_VIEWMODEL_MAX_COMMANDS];
    EditorToolView* tools;
    int tool_count;
    int tool_capacity;
    EditorPropertyView properties[GRAPHIC_OBJECT_MAX_PROPERTIES];
    int property_count;
    EditorLayerView* layers;
    int layer_count;
    int layer_capacity;
    UiDialogState active_dialog;
    int has_selection;
} EditorViewModel;

void editor_viewmodel_init(EditorViewModel* view_model);
void editor_viewmodel_shutdown(EditorViewModel* view_model);
int editor_viewmodel_build(EditorViewModel* view_model, const struct Workspace* workspace);
int editor_viewmodel_command_available(const EditorViewModel* view_model,
                                       EditorCommand command);
const char* editor_viewmodel_command_shortcut(const EditorViewModel* view_model,
                                              EditorCommand command);
const char* editor_viewmodel_command_unavailable_reason(const EditorViewModel* view_model,
                                                        EditorCommand command);

#endif /* GLDRAW_UI_EDITOR_VIEWMODEL_H */
