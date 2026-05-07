#ifndef GLDRAW_COMMANDS_COMMAND_INTERNAL_H
#define GLDRAW_COMMANDS_COMMAND_INTERNAL_H

#include <commands/command.h>

#define COMMAND_INITIAL_CAPACITY 16u
#define COMMAND_DEFAULT_MEMORY_BUDGET_BYTES (200u * 1024u * 1024u)

typedef struct CreateObjectCommand {
    Command base;
    GraphicObject* object_snapshot;
    ObjectId object_id;
} CreateObjectCommand;

typedef struct DeleteSelectionCommand {
    Command base;
    GraphicObject** objects;
    int* original_indices;
    int object_count;
} DeleteSelectionCommand;

typedef struct MoveObjectsCommand {
    Command base;
    ObjectId* object_ids;
    int object_count;
    Vec2 delta;
} MoveObjectsCommand;

typedef struct PasteObjectsCommand {
    Command base;
    GraphicObject** object_snapshots;
    ObjectId* object_ids;
    int object_count;
    LayerId layer_id;
} PasteObjectsCommand;

typedef struct SetPropertyCommand {
    Command base;
    ObjectId object_id;
    char key[32];
    float before_value;
    float after_value;
} SetPropertyCommand;

typedef struct SetActiveLayerCommand {
    Command base;
    LayerId before_layer_id;
    LayerId after_layer_id;
} SetActiveLayerCommand;

typedef struct SetLayerVisibilityCommand {
    Command base;
    LayerId layer_id;
    int before_visible;
    int after_visible;
} SetLayerVisibilityCommand;

typedef struct SetLayerLockedCommand {
    Command base;
    LayerId layer_id;
    int before_locked;
    int after_locked;
} SetLayerLockedCommand;

typedef struct RenameLayerCommand {
    Command base;
    LayerId layer_id;
    char before_name[32];
    char after_name[32];
} RenameLayerCommand;

typedef struct MoveLayerCommand {
    Command base;
    LayerId layer_id;
    int before_index;
    int after_index;
} MoveLayerCommand;

typedef struct CreateLayerCommand {
    Command base;
    DocumentLayer layer;
    int layer_index;
    LayerId previous_active_layer_id;
    int created_once;
} CreateLayerCommand;

typedef struct TransactionCommand {
    Command base;
    Command** commands;
    size_t command_count;
} TransactionCommand;

extern CommandAllocator g_allocator;
extern const CommandVTable CREATE_OBJECT_VTABLE;
extern const CommandVTable DELETE_SELECTION_VTABLE;
extern const CommandVTable MOVE_OBJECTS_VTABLE;
extern const CommandVTable PASTE_OBJECTS_VTABLE;
extern const CommandVTable SET_PROPERTY_VTABLE;
extern const CommandVTable SET_ACTIVE_LAYER_VTABLE;
extern const CommandVTable SET_LAYER_VISIBILITY_VTABLE;
extern const CommandVTable SET_LAYER_LOCKED_VTABLE;
extern const CommandVTable RENAME_LAYER_VTABLE;
extern const CommandVTable MOVE_LAYER_VTABLE;
extern const CommandVTable CREATE_LAYER_VTABLE;
extern const CommandVTable TRANSACTION_VTABLE;

void command_destroy(Command* command);
size_t command_measure(const Command* command);
int command_executor_append_internal(CommandExecutor* executor, Command* command);

#endif /* GLDRAW_COMMANDS_COMMAND_INTERNAL_H */
