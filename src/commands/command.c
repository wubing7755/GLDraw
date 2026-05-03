#include <commands/command.h>

#include <base/log.h>
#include <base/math2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_INITIAL_CAPACITY 16u
#define COMMAND_DEFAULT_MEMORY_BUDGET_BYTES (200u * 1024u * 1024u)

static void *command_default_malloc(size_t size) { return malloc(size); }
static void *command_default_calloc(size_t count, size_t size) { return calloc(count, size); }
static void *command_default_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
static void command_default_free(void *ptr) { free(ptr); }

static CommandAllocator g_allocator = {command_default_malloc, command_default_calloc,
                                       command_default_realloc, command_default_free};

typedef struct CreateObjectCommand {
  Command base;
  GraphicObject *object_snapshot;
  ObjectId object_id;
} CreateObjectCommand;

typedef struct DeleteSelectionCommand {
  Command base;
  GraphicObject **objects;
  int *original_indices;
  int object_count;
} DeleteSelectionCommand;

typedef struct MoveObjectsCommand {
  Command base;
  ObjectId *object_ids;
  int object_count;
  Vec2 delta;
} MoveObjectsCommand;

typedef struct PasteObjectsCommand {
  Command base;
  GraphicObject **object_snapshots;
  ObjectId *object_ids;
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
  Command **commands;
  size_t command_count;
} TransactionCommand;

static const CommandVTable CREATE_OBJECT_VTABLE;
static const CommandVTable DELETE_SELECTION_VTABLE;
static const CommandVTable MOVE_OBJECTS_VTABLE;
static const CommandVTable PASTE_OBJECTS_VTABLE;
static const CommandVTable SET_PROPERTY_VTABLE;
static const CommandVTable SET_ACTIVE_LAYER_VTABLE;
static const CommandVTable SET_LAYER_VISIBILITY_VTABLE;
static const CommandVTable SET_LAYER_LOCKED_VTABLE;
static const CommandVTable RENAME_LAYER_VTABLE;
static const CommandVTable MOVE_LAYER_VTABLE;
static const CommandVTable CREATE_LAYER_VTABLE;
static const CommandVTable TRANSACTION_VTABLE;

static void command_destroy(Command *command) {
  if (command && command->vtable && command->vtable->destroy) {
    command->vtable->destroy(command);
  }
}

static size_t command_measure(const Command *command) {
  const CreateObjectCommand *create_command = (const CreateObjectCommand *)command;
  const DeleteSelectionCommand *delete_command = (const DeleteSelectionCommand *)command;
  const MoveObjectsCommand *move_command = (const MoveObjectsCommand *)command;
  const PasteObjectsCommand *paste_command = (const PasteObjectsCommand *)command;
  const SetPropertyCommand *property_command = (const SetPropertyCommand *)command;
  const SetActiveLayerCommand *active_layer_command =
      (const SetActiveLayerCommand *)command;
  const SetLayerVisibilityCommand *visibility_command =
      (const SetLayerVisibilityCommand *)command;
  const SetLayerLockedCommand *locked_command =
      (const SetLayerLockedCommand *)command;
  const RenameLayerCommand *rename_layer_command =
      (const RenameLayerCommand *)command;
  const MoveLayerCommand *move_layer_command = (const MoveLayerCommand *)command;
  const CreateLayerCommand *create_layer_command = (const CreateLayerCommand *)command;
  const TransactionCommand *transaction_command = (const TransactionCommand *)command;
  size_t bytes = 0u;
  size_t i = 0u;

  if (!command || !command->vtable) {
    return 0u;
  }

  if (command->vtable == &CREATE_OBJECT_VTABLE) {
    bytes = sizeof(*create_command);
    if (create_command->object_snapshot) {
      bytes += sizeof(*create_command->object_snapshot);
      bytes += sizeof(GraphicPropertyBag);
    }
    return bytes;
  }
  if (command->vtable == &DELETE_SELECTION_VTABLE) {
    bytes = sizeof(*delete_command);
    bytes += (size_t)delete_command->object_count * sizeof(delete_command->objects[0]);
    bytes += (size_t)delete_command->object_count * sizeof(delete_command->original_indices[0]);
    bytes += (size_t)delete_command->object_count * (sizeof(GraphicObject) + sizeof(GraphicPropertyBag));
    return bytes;
  }
  if (command->vtable == &MOVE_OBJECTS_VTABLE) {
    bytes = sizeof(*move_command);
    bytes += (size_t)move_command->object_count * sizeof(move_command->object_ids[0]);
    return bytes;
  }
  if (command->vtable == &PASTE_OBJECTS_VTABLE) {
    bytes = sizeof(*paste_command);
    bytes += (size_t)paste_command->object_count * sizeof(paste_command->object_snapshots[0]);
    bytes += (size_t)paste_command->object_count * sizeof(paste_command->object_ids[0]);
    bytes += (size_t)paste_command->object_count * (sizeof(GraphicObject) + sizeof(GraphicPropertyBag));
    return bytes;
  }
  if (command->vtable == &SET_PROPERTY_VTABLE) {
    return sizeof(*property_command);
  }
  if (command->vtable == &SET_ACTIVE_LAYER_VTABLE) {
    return sizeof(*active_layer_command);
  }
  if (command->vtable == &SET_LAYER_VISIBILITY_VTABLE) {
    return sizeof(*visibility_command);
  }
  if (command->vtable == &SET_LAYER_LOCKED_VTABLE) {
    return sizeof(*locked_command);
  }
  if (command->vtable == &RENAME_LAYER_VTABLE) {
    return sizeof(*rename_layer_command);
  }
  if (command->vtable == &MOVE_LAYER_VTABLE) {
    return sizeof(*move_layer_command);
  }
  if (command->vtable == &CREATE_LAYER_VTABLE) {
    return sizeof(*create_layer_command);
  }
  if (command->vtable == &TRANSACTION_VTABLE) {
    bytes = sizeof(*transaction_command);
    bytes += transaction_command->command_count * sizeof(transaction_command->commands[0]);
    for (i = 0u; i < transaction_command->command_count; ++i) {
      bytes += command_measure(transaction_command->commands[i]);
    }
    return bytes;
  }

  return sizeof(Command);
}

static void command_executor_refresh_counts(CommandExecutor *executor) {
  if (!executor) {
    return;
  }
  executor->undo_count = executor->cursor;
  executor->redo_count =
      (executor->entry_count >= executor->cursor) ? (executor->entry_count - executor->cursor) : 0u;
}

static int command_reserve_log(CommandExecutor *executor, size_t needed) {
  CommandLogEntry *entries = NULL;
  size_t capacity = 0u;

  if (!executor) {
    return 0;
  }
  if (needed <= executor->entry_capacity) {
    return 1;
  }

  capacity = executor->entry_capacity > 0u ? executor->entry_capacity : COMMAND_INITIAL_CAPACITY;
  while (capacity < needed) {
    capacity *= 2u;
  }

  entries = (CommandLogEntry *)g_allocator.realloc_fn(
      executor->entries, capacity * sizeof(executor->entries[0]));
  if (!entries) {
    return 0;
  }

  executor->entries = entries;
  executor->entry_capacity = capacity;
  return 1;
}

static int command_reserve_transaction(CommandExecutor *executor, size_t needed) {
  Command **commands = NULL;
  size_t capacity = 0u;

  if (!executor) {
    return 0;
  }
  if (needed <= executor->transaction_capacity) {
    return 1;
  }

  capacity = executor->transaction_capacity > 0u ? executor->transaction_capacity
                                                 : COMMAND_INITIAL_CAPACITY;
  while (capacity < needed) {
    capacity *= 2u;
  }

  commands = (Command **)g_allocator.realloc_fn(
      executor->transaction_commands, capacity * sizeof(executor->transaction_commands[0]));
  if (!commands) {
    return 0;
  }

  executor->transaction_commands = commands;
  executor->transaction_capacity = capacity;
  return 1;
}

static void command_executor_discard_entry(CommandLogEntry *entry) {
  if (!entry) {
    return;
  }
  command_destroy(entry->command);
  entry->command = NULL;
  entry->bytes = 0u;
}

static void command_executor_clear_tail(CommandExecutor *executor) {
  size_t i = 0u;

  if (!executor) {
    return;
  }

  for (i = executor->cursor; i < executor->entry_count; ++i) {
    executor->used_bytes -= executor->entries[i].bytes;
    command_executor_discard_entry(&executor->entries[i]);
  }
  executor->entry_count = executor->cursor;
  command_executor_refresh_counts(executor);
}

static void command_executor_prune_budget(CommandExecutor *executor) {
  size_t i = 0u;

  if (!executor || executor->memory_budget_bytes == 0u) {
    return;
  }

  while (executor->used_bytes > executor->memory_budget_bytes && executor->cursor > 1u) {
    executor->used_bytes -= executor->entries[0].bytes;
    command_executor_discard_entry(&executor->entries[0]);
    for (i = 1u; i < executor->entry_count; ++i) {
      executor->entries[i - 1u] = executor->entries[i];
    }
    executor->entry_count--;
    executor->cursor--;
  }
  command_executor_refresh_counts(executor);
}

static int command_executor_append_internal(CommandExecutor *executor, Command *command) {
  Command *previous = NULL;
  size_t before_bytes = 0u;
  size_t after_bytes = 0u;
  size_t bytes = 0u;

  if (!executor || !command || !command->vtable) {
    return 0;
  }

  if (executor->transaction_active) {
    if (!command_reserve_transaction(executor, executor->transaction_count + 1u)) {
      return 0;
    }

    previous = executor->transaction_count > 0u
                   ? executor->transaction_commands[executor->transaction_count - 1u]
                   : NULL;
    if (previous && previous->vtable && previous->vtable->merge) {
      before_bytes = command_measure(previous);
      if (previous->vtable->merge(previous, command)) {
        after_bytes = command_measure(previous);
        executor->transaction_bytes += (after_bytes >= before_bytes) ? (after_bytes - before_bytes)
                                                                     : 0u;
        if (after_bytes < before_bytes) {
          executor->transaction_bytes -= (before_bytes - after_bytes);
        }
        command_destroy(command);
        return 1;
      }
    }

    executor->transaction_commands[executor->transaction_count++] = command;
    executor->transaction_bytes += command_measure(command);
    return 1;
  }

  if (!command_reserve_log(executor, executor->cursor + 1u)) {
    return 0;
  }

  command_executor_clear_tail(executor);
  previous = executor->cursor > 0u ? executor->entries[executor->cursor - 1u].command : NULL;
  if (previous && previous->vtable && previous->vtable->merge) {
    before_bytes = executor->entries[executor->cursor - 1u].bytes;
    if (previous->vtable->merge(previous, command)) {
      after_bytes = command_measure(previous);
      executor->used_bytes += (after_bytes >= before_bytes) ? (after_bytes - before_bytes) : 0u;
      if (after_bytes < before_bytes) {
        executor->used_bytes -= (before_bytes - after_bytes);
      }
      executor->entries[executor->cursor - 1u].bytes = after_bytes;
      command_destroy(command);
      command_executor_prune_budget(executor);
      return 1;
    }
  }

  bytes = command_measure(command);
  executor->entries[executor->cursor].command = command;
  executor->entries[executor->cursor].bytes = bytes;
  executor->cursor++;
  executor->entry_count = executor->cursor;
  executor->used_bytes += bytes;
  command_executor_refresh_counts(executor);
  command_executor_prune_budget(executor);
  return 1;
}

static GraphicObject *document_find_for_command(Document *document, ObjectId id) {
  return document_find_object(document, id);
}

static CommandExecuteCheck create_object_check(const Document *document,
                                               const CreateObjectCommand *command) {
  LayerId layer_id = 0u;

  if (!document || !command || !command->object_snapshot) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }

  layer_id = command->object_snapshot->layer_id;
  if (layer_id == 0u) {
    layer_id = document_active_layer_id(document);
  }

  if (document_layer_is_locked(document, layer_id)) {
    return COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED;
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck delete_selection_check(const Document *document,
                                                  const DeleteSelectionCommand *command) {
  int i = 0;

  if (!document || !command || command->object_count <= 0) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }

  for (i = 0; i < command->object_count; ++i) {
    const GraphicObject *object = command->objects[i];
    if (!object) {
      return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }
    if (!document_find_object(document, object->id)) {
      return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
    }
    if (document_object_is_locked(document, object->id)) {
      return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
    }
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck move_objects_check(const Document *document,
                                              const MoveObjectsCommand *command) {
  int i = 0;

  if (!document || !command || command->object_count <= 0) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }

  for (i = 0; i < command->object_count; ++i) {
    if (!document_find_object(document, command->object_ids[i])) {
      return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
    }
    if (document_object_is_locked(document, command->object_ids[i])) {
      return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
    }
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck paste_objects_check(const Document *document,
                                               const PasteObjectsCommand *command) {
  int i = 0;

  if (!document || !command || command->object_count <= 0 || command->layer_id == 0u) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }
  if (document_layer_is_locked(document, command->layer_id)) {
    return COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED;
  }

  for (i = 0; i < command->object_count; ++i) {
    if (!command->object_snapshots[i] || command->object_ids[i] == 0u) {
      return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }
    if (document_find_object(document, command->object_ids[i])) {
      return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
    }
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

static CommandExecuteCheck set_property_check(const Document *document,
                                             const SetPropertyCommand *command) {
  if (!document || !command) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }

  if (!document_find_object(document, command->object_id)) {
    return COMMAND_EXECUTE_CHECK_TARGET_MISSING;
  }
  if (document_object_is_locked(document, command->object_id)) {
    return COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED;
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

CommandExecuteCheck command_check_execute(const Command *command,
                                         const Document *document) {
  if (!command || !command->vtable || !document) {
    return COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;
  }

  if (command->vtable == &CREATE_OBJECT_VTABLE) {
    return create_object_check(document, (const CreateObjectCommand *)command);
  }
  if (command->vtable == &DELETE_SELECTION_VTABLE) {
    return delete_selection_check(document, (const DeleteSelectionCommand *)command);
  }
  if (command->vtable == &MOVE_OBJECTS_VTABLE) {
    return move_objects_check(document, (const MoveObjectsCommand *)command);
  }
  if (command->vtable == &PASTE_OBJECTS_VTABLE) {
    return paste_objects_check(document, (const PasteObjectsCommand *)command);
  }
  if (command->vtable == &SET_PROPERTY_VTABLE) {
    return set_property_check(document, (const SetPropertyCommand *)command);
  }

  return COMMAND_EXECUTE_CHECK_OK;
}

int command_can_execute(const Command *command, const Document *document) {
  return command_check_execute(command, document) == COMMAND_EXECUTE_CHECK_OK;
}

const char *command_execute_check_message(CommandExecuteCheck check) {
  switch (check) {
  case COMMAND_EXECUTE_CHECK_OK:
    return "Command can execute.";
  case COMMAND_EXECUTE_CHECK_TARGET_MISSING:
    return "Target object is missing.";
  case COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED:
    return "Target object is on a locked layer.";
  case COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED:
    return "Active layer is locked.";
  case COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT:
  default:
    return "Command is invalid in the current context.";
  }
}

static int create_object_execute(void *cmd, Document *document) {
  CreateObjectCommand *command = (CreateObjectCommand *)cmd;
  GraphicObject *clone = NULL;

  if (!command || !document || !command->object_snapshot) {
    return 0;
  }

  clone = object_clone(command->object_snapshot);
  if (!clone) {
    return 0;
  }

  if (command->object_id != 0u) {
    if (!document_append_object_with_id_to_layer(document, clone, command->object_id,
                                                 clone->layer_id)) {
      object_destroy(clone);
      return 0;
    }
  } else {
    if (!document_add_object_to_layer(document, clone, clone->layer_id)) {
      object_destroy(clone);
      return 0;
    }
    command->object_id = clone->id;
    command->object_snapshot->id = clone->id;
  }

  return 1;
}

static int create_object_undo(void *cmd, Document *document) {
  CreateObjectCommand *command = (CreateObjectCommand *)cmd;

  if (!command || !document || command->object_id == 0u) {
    return 0;
  }

  return document_remove_object(document, command->object_id);
}

static int create_object_merge(void *cmd, const void *next) {
  (void)cmd;
  (void)next;
  return 0;
}

static void create_object_destroy(void *cmd) {
  CreateObjectCommand *command = (CreateObjectCommand *)cmd;

  if (!command) {
    return;
  }
  object_destroy(command->object_snapshot);
  g_allocator.free_fn(command);
}

static const CommandVTable CREATE_OBJECT_VTABLE = {create_object_execute, create_object_undo,
                                                   create_object_execute, create_object_merge,
                                                   create_object_destroy};

static int delete_selection_execute(void *cmd, Document *document) {
  DeleteSelectionCommand *command = (DeleteSelectionCommand *)cmd;
  int i = 0;

  if (!command || !document) {
    return 0;
  }

  for (i = 0; i < command->object_count; ++i) {
    if (command->objects[i]) {
      document_remove_object(document, command->objects[i]->id);
    }
  }
  return 1;
}

static int delete_selection_undo(void *cmd, Document *document) {
  DeleteSelectionCommand *command = (DeleteSelectionCommand *)cmd;
  int i = 0;

  if (!command || !document) {
    return 0;
  }

  for (i = 0; i < command->object_count; ++i) {
    if (!document_insert_object_clone_at(document, command->objects[i],
                                         command->original_indices[i])) {
      return 0;
    }
  }

  return 1;
}

static int delete_selection_merge(void *cmd, const void *next) {
  (void)cmd;
  (void)next;
  return 0;
}

static void delete_selection_destroy(void *cmd) {
  DeleteSelectionCommand *command = (DeleteSelectionCommand *)cmd;
  int i = 0;

  if (!command) {
    return;
  }
  for (i = 0; i < command->object_count; ++i) {
    object_destroy(command->objects[i]);
  }
  g_allocator.free_fn(command->objects);
  g_allocator.free_fn(command->original_indices);
  g_allocator.free_fn(command);
}

static const CommandVTable DELETE_SELECTION_VTABLE = {
    delete_selection_execute, delete_selection_undo, delete_selection_execute,
    delete_selection_merge, delete_selection_destroy};

static int move_objects_apply(Document *document, const MoveObjectsCommand *command,
                              Vec2 delta) {
  int i = 0;
  int moved = 0;

  if (!document || !command) {
    return 0;
  }

  for (i = 0; i < command->object_count; ++i) {
    GraphicObject *object = document_find_for_command(document, command->object_ids[i]);
    if (object) {
      object_translate(object, delta);
      moved = 1;
    }
  }

  if (moved) {
    document_touch(document);
  }
  return moved;
}

static int move_objects_execute(void *cmd, Document *document) {
  MoveObjectsCommand *command = (MoveObjectsCommand *)cmd;
  return move_objects_apply(document, command,
                            command ? command->delta : vec2_make(0.0f, 0.0f));
}

static int move_objects_undo(void *cmd, Document *document) {
  MoveObjectsCommand *command = (MoveObjectsCommand *)cmd;
  Vec2 undo_delta = vec2_make(0.0f, 0.0f);

  if (!command) {
    return 0;
  }

  undo_delta.x = -command->delta.x;
  undo_delta.y = -command->delta.y;
  return move_objects_apply(document, command, undo_delta);
}

static int move_objects_merge(void *cmd, const void *next) {
  MoveObjectsCommand *command = (MoveObjectsCommand *)cmd;
  const MoveObjectsCommand *next_command = (const MoveObjectsCommand *)next;
  float dot = 0.0f;
  int i = 0;

  if (!command || !next_command || next_command->base.vtable != &MOVE_OBJECTS_VTABLE ||
      command->object_count != next_command->object_count) {
    return 0;
  }

  for (i = 0; i < command->object_count; ++i) {
    if (command->object_ids[i] != next_command->object_ids[i]) {
      return 0;
    }
  }

  dot = command->delta.x * next_command->delta.x + command->delta.y * next_command->delta.y;
  if (dot <= 0.0f) {
    return 0;
  }

  command->delta = vec2_add(command->delta, next_command->delta);
  return 1;
}

static void move_objects_destroy(void *cmd) {
  MoveObjectsCommand *command = (MoveObjectsCommand *)cmd;
  if (!command) {
    return;
  }
  g_allocator.free_fn(command->object_ids);
  g_allocator.free_fn(command);
}

static const CommandVTable MOVE_OBJECTS_VTABLE = {move_objects_execute, move_objects_undo,
                                                  move_objects_execute, move_objects_merge,
                                                  move_objects_destroy};

static int paste_objects_execute(void *cmd, Document *document) {
  PasteObjectsCommand *command = (PasteObjectsCommand *)cmd;
  int i = 0;

  if (!command || !document) {
    return 0;
  }

  for (i = 0; i < command->object_count; ++i) {
    GraphicObject *clone = object_clone(command->object_snapshots[i]);
    if (!clone) {
      break;
    }

    if (!document_append_object_with_id_to_layer(document,
                                                 clone,
                                                 command->object_ids[i],
                                                 command->layer_id)) {
      object_destroy(clone);
      break;
    }
  }

  if (i == command->object_count) {
    return 1;
  }

  /* Best-effort rollback: undo objects already inserted. If any removal
   * fails the document is already in an inconsistent state. Log the failure,
   * destroy the command, and return failure. */
  while (i > 0) {
    --i;
    if (!document_remove_object(document, command->object_ids[i])) {
      LOG_WARN("paste_objects_execute rollback: failed to remove object %u",
               (unsigned)command->object_ids[i]);
    }
  }
  return 0;
}

static int paste_objects_undo(void *cmd, Document *document) {
  PasteObjectsCommand *command = (PasteObjectsCommand *)cmd;
  int i = 0;

  if (!command || !document) {
    return 0;
  }

  for (i = command->object_count - 1; i >= 0; --i) {
    if (!document_remove_object(document, command->object_ids[i])) {
      return 0;
    }
  }

  return 1;
}

static int paste_objects_merge(void *cmd, const void *next) {
  (void)cmd;
  (void)next;
  return 0;
}

static void paste_objects_destroy(void *cmd) {
  PasteObjectsCommand *command = (PasteObjectsCommand *)cmd;
  int i = 0;

  if (!command) {
    return;
  }

  for (i = 0; i < command->object_count; ++i) {
    object_destroy(command->object_snapshots[i]);
  }
  g_allocator.free_fn(command->object_snapshots);
  g_allocator.free_fn(command->object_ids);
  g_allocator.free_fn(command);
}

static const CommandVTable PASTE_OBJECTS_VTABLE = {paste_objects_execute, paste_objects_undo,
                                                   paste_objects_execute, paste_objects_merge,
                                                   paste_objects_destroy};

static int set_property_apply(Document *document, const SetPropertyCommand *command,
                              float value) {
  GraphicObject *object = NULL;

  if (!document || !command) {
    return 0;
  }

  object = document_find_for_command(document, command->object_id);
  if (!object || !object_set_scalar(object, command->key, value)) {
    return 0;
  }

  document_touch(document);
  return 1;
}

static int set_property_execute(void *cmd, Document *document) {
  SetPropertyCommand *command = (SetPropertyCommand *)cmd;
  return set_property_apply(document, command, command ? command->after_value : 0.0f);
}

static int set_property_undo(void *cmd, Document *document) {
  SetPropertyCommand *command = (SetPropertyCommand *)cmd;
  return set_property_apply(document, command, command ? command->before_value : 0.0f);
}

static int set_property_merge(void *cmd, const void *next) {
  SetPropertyCommand *command = (SetPropertyCommand *)cmd;
  const SetPropertyCommand *next_command = (const SetPropertyCommand *)next;

  if (!command || !next_command || next_command->base.vtable != &SET_PROPERTY_VTABLE ||
      command->object_id != next_command->object_id ||
      strcmp(command->key, next_command->key) != 0) {
    return 0;
  }

  command->after_value = next_command->after_value;
  return 1;
}

static void set_property_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable SET_PROPERTY_VTABLE = {set_property_execute, set_property_undo,
                                                  set_property_execute, set_property_merge,
                                                  set_property_destroy};

static int set_active_layer_apply(Document *document, LayerId layer_id) {
  if (!document) {
    return 0;
  }
  return document_set_active_layer(document, layer_id);
}

static int set_active_layer_execute(void *cmd, Document *document) {
  SetActiveLayerCommand *command = (SetActiveLayerCommand *)cmd;
  return set_active_layer_apply(document, command ? command->after_layer_id : 0u);
}

static int set_active_layer_undo(void *cmd, Document *document) {
  SetActiveLayerCommand *command = (SetActiveLayerCommand *)cmd;
  return set_active_layer_apply(document, command ? command->before_layer_id : 0u);
}

static int set_active_layer_merge(void *cmd, const void *next) {
  SetActiveLayerCommand *command = (SetActiveLayerCommand *)cmd;
  const SetActiveLayerCommand *next_command = (const SetActiveLayerCommand *)next;

  if (!command || !next_command ||
      next_command->base.vtable != &SET_ACTIVE_LAYER_VTABLE) {
    return 0;
  }

  command->after_layer_id = next_command->after_layer_id;
  return 1;
}

static void set_active_layer_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable SET_ACTIVE_LAYER_VTABLE = {
    set_active_layer_execute, set_active_layer_undo, set_active_layer_execute,
    set_active_layer_merge, set_active_layer_destroy};

static int set_layer_visibility_apply(Document *document,
                                      LayerId layer_id,
                                      int visible) {
  if (!document) {
    return 0;
  }
  return document_set_layer_visibility(document, layer_id, visible);
}

static int set_layer_visibility_execute(void *cmd, Document *document) {
  SetLayerVisibilityCommand *command = (SetLayerVisibilityCommand *)cmd;
  return set_layer_visibility_apply(document,
                                    command ? command->layer_id : 0u,
                                    command ? command->after_visible : 0);
}

static int set_layer_visibility_undo(void *cmd, Document *document) {
  SetLayerVisibilityCommand *command = (SetLayerVisibilityCommand *)cmd;
  return set_layer_visibility_apply(document,
                                    command ? command->layer_id : 0u,
                                    command ? command->before_visible : 0);
}

static int set_layer_visibility_merge(void *cmd, const void *next) {
  SetLayerVisibilityCommand *command = (SetLayerVisibilityCommand *)cmd;
  const SetLayerVisibilityCommand *next_command = (const SetLayerVisibilityCommand *)next;

  if (!command || !next_command ||
      next_command->base.vtable != &SET_LAYER_VISIBILITY_VTABLE ||
      command->layer_id != next_command->layer_id) {
    return 0;
  }

  command->after_visible = next_command->after_visible;
  return 1;
}

static void set_layer_visibility_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable SET_LAYER_VISIBILITY_VTABLE = {
    set_layer_visibility_execute,
    set_layer_visibility_undo,
    set_layer_visibility_execute,
    set_layer_visibility_merge,
    set_layer_visibility_destroy};

static int set_layer_locked_apply(Document *document, LayerId layer_id, int locked) {
  if (!document) {
    return 0;
  }
  return document_set_layer_locked(document, layer_id, locked);
}

static int set_layer_locked_execute(void *cmd, Document *document) {
  SetLayerLockedCommand *command = (SetLayerLockedCommand *)cmd;
  return set_layer_locked_apply(document,
                                command ? command->layer_id : 0u,
                                command ? command->after_locked : 0);
}

static int set_layer_locked_undo(void *cmd, Document *document) {
  SetLayerLockedCommand *command = (SetLayerLockedCommand *)cmd;
  return set_layer_locked_apply(document,
                                command ? command->layer_id : 0u,
                                command ? command->before_locked : 0);
}

static int set_layer_locked_merge(void *cmd, const void *next) {
  SetLayerLockedCommand *command = (SetLayerLockedCommand *)cmd;
  const SetLayerLockedCommand *next_command = (const SetLayerLockedCommand *)next;

  if (!command || !next_command ||
      next_command->base.vtable != &SET_LAYER_LOCKED_VTABLE ||
      command->layer_id != next_command->layer_id) {
    return 0;
  }

  command->after_locked = next_command->after_locked;
  return 1;
}

static void set_layer_locked_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable SET_LAYER_LOCKED_VTABLE = {
    set_layer_locked_execute,
    set_layer_locked_undo,
    set_layer_locked_execute,
    set_layer_locked_merge,
    set_layer_locked_destroy};

static int rename_layer_apply(Document *document, LayerId layer_id, const char *name) {
  if (!document || !name) {
    return 0;
  }
  return document_rename_layer(document, layer_id, name);
}

static int rename_layer_execute(void *cmd, Document *document) {
  RenameLayerCommand *command = (RenameLayerCommand *)cmd;
  return rename_layer_apply(document,
                            command ? command->layer_id : 0u,
                            command ? command->after_name : NULL);
}

static int rename_layer_undo(void *cmd, Document *document) {
  RenameLayerCommand *command = (RenameLayerCommand *)cmd;
  return rename_layer_apply(document,
                            command ? command->layer_id : 0u,
                            command ? command->before_name : NULL);
}

static int rename_layer_merge(void *cmd, const void *next) {
  RenameLayerCommand *command = (RenameLayerCommand *)cmd;
  const RenameLayerCommand *next_command = (const RenameLayerCommand *)next;

  if (!command || !next_command ||
      next_command->base.vtable != &RENAME_LAYER_VTABLE ||
      command->layer_id != next_command->layer_id) {
    return 0;
  }

  snprintf(command->after_name, sizeof(command->after_name), "%s",
           next_command->after_name);
  return 1;
}

static void rename_layer_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable RENAME_LAYER_VTABLE = {rename_layer_execute, rename_layer_undo,
                                                  rename_layer_execute, rename_layer_merge,
                                                  rename_layer_destroy};

static int move_layer_apply(Document *document, LayerId layer_id, int index) {
  if (!document) {
    return 0;
  }
  return document_move_layer(document, layer_id, index);
}

static int move_layer_execute(void *cmd, Document *document) {
  MoveLayerCommand *command = (MoveLayerCommand *)cmd;
  return move_layer_apply(document,
                          command ? command->layer_id : 0u,
                          command ? command->after_index : 0);
}

static int move_layer_undo(void *cmd, Document *document) {
  MoveLayerCommand *command = (MoveLayerCommand *)cmd;
  return move_layer_apply(document,
                          command ? command->layer_id : 0u,
                          command ? command->before_index : 0);
}

static int move_layer_merge(void *cmd, const void *next) {
  MoveLayerCommand *command = (MoveLayerCommand *)cmd;
  const MoveLayerCommand *next_command = (const MoveLayerCommand *)next;

  if (!command || !next_command || next_command->base.vtable != &MOVE_LAYER_VTABLE ||
      command->layer_id != next_command->layer_id) {
    return 0;
  }

  command->after_index = next_command->after_index;
  return 1;
}

static void move_layer_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable MOVE_LAYER_VTABLE = {move_layer_execute, move_layer_undo,
                                                move_layer_execute, move_layer_merge,
                                                move_layer_destroy};

static int create_layer_execute(void *cmd, Document *document) {
  CreateLayerCommand *command = (CreateLayerCommand *)cmd;
  DocumentLayer *layer = NULL;

  if (!command || !document) {
    return 0;
  }

  if (!command->created_once) {
    command->previous_active_layer_id = document_active_layer_id(document);
    command->layer_index = document_layer_count(document);
    command->layer.id = document_create_layer(document, command->layer.name);
    if (command->layer.id == 0u) {
      return 0;
    }
    layer = document_layer_find(document, command->layer.id);
    if (!layer) {
      return 0;
    }
    command->layer = *layer;
    command->created_once = 1;
    return document_set_active_layer(document, command->layer.id);
  }

  if (!document_insert_layer_at(document, &command->layer, command->layer_index)) {
    return 0;
  }
  return document_set_active_layer(document, command->layer.id);
}

static int create_layer_undo(void *cmd, Document *document) {
  CreateLayerCommand *command = (CreateLayerCommand *)cmd;

  if (!command || !document || command->layer.id == 0u) {
    return 0;
  }

  if (!document_delete_layer(document, command->layer.id)) {
    return 0;
  }
  if (command->previous_active_layer_id != 0u) {
    return document_set_active_layer(document, command->previous_active_layer_id);
  }
  return 1;
}

static int create_layer_merge(void *cmd, const void *next) {
  (void)cmd;
  (void)next;
  return 0;
}

static void create_layer_destroy(void *cmd) { g_allocator.free_fn(cmd); }

static const CommandVTable CREATE_LAYER_VTABLE = {
    create_layer_execute, create_layer_undo, create_layer_execute,
    create_layer_merge, create_layer_destroy};

static int transaction_execute(void *cmd, Document *document) {
  TransactionCommand *transaction = (TransactionCommand *)cmd;
  size_t i = 0u;

  if (!transaction || !document) {
    return 0;
  }

  for (i = 0u; i < transaction->command_count; ++i) {
    if (!transaction->commands[i] || !transaction->commands[i]->vtable ||
        !transaction->commands[i]->vtable->execute ||
        !transaction->commands[i]->vtable->execute(transaction->commands[i], document)) {
      while (i > 0u) {
        --i;
        if (transaction->commands[i]->vtable && transaction->commands[i]->vtable->undo) {
          transaction->commands[i]->vtable->undo(transaction->commands[i], document);
        }
      }
      return 0;
    }
  }
  return 1;
}

static int transaction_undo(void *cmd, Document *document) {
  TransactionCommand *transaction = (TransactionCommand *)cmd;
  size_t i = 0u;

  if (!transaction || !document) {
    return 0;
  }

  for (i = transaction->command_count; i > 0u; --i) {
    if (!transaction->commands[i - 1u] || !transaction->commands[i - 1u]->vtable ||
        !transaction->commands[i - 1u]->vtable->undo ||
        !transaction->commands[i - 1u]->vtable->undo(transaction->commands[i - 1u], document)) {
      return 0;
    }
  }
  return 1;
}

static int transaction_merge(void *cmd, const void *next) {
  (void)cmd;
  (void)next;
  return 0;
}

static void transaction_destroy(void *cmd) {
  TransactionCommand *transaction = (TransactionCommand *)cmd;
  size_t i = 0u;

  if (!transaction) {
    return;
  }

  for (i = 0u; i < transaction->command_count; ++i) {
    command_destroy(transaction->commands[i]);
  }
  g_allocator.free_fn(transaction->commands);
  g_allocator.free_fn(transaction);
}

static const CommandVTable TRANSACTION_VTABLE = {transaction_execute, transaction_undo,
                                                 transaction_execute, transaction_merge,
                                                 transaction_destroy};

void command_set_allocator(const CommandAllocator *allocator) {
  if (!allocator || !allocator->malloc_fn || !allocator->calloc_fn ||
      !allocator->realloc_fn || !allocator->free_fn) {
    command_reset_allocator();
    return;
  }
  g_allocator = *allocator;
}

void command_reset_allocator(void) {
  g_allocator.malloc_fn = command_default_malloc;
  g_allocator.calloc_fn = command_default_calloc;
  g_allocator.realloc_fn = command_default_realloc;
  g_allocator.free_fn = command_default_free;
}

int command_executor_init_with_budget(CommandExecutor *executor,
                                      size_t memory_budget_bytes) {
  if (!executor) {
    return 0;
  }

  memset(executor, 0, sizeof(*executor));
  executor->memory_budget_bytes =
      memory_budget_bytes > 0u ? memory_budget_bytes : COMMAND_DEFAULT_MEMORY_BUDGET_BYTES;
  return 1;
}

int command_executor_init(CommandExecutor *executor) {
  return command_executor_init_with_budget(executor, COMMAND_DEFAULT_MEMORY_BUDGET_BYTES);
}

void command_executor_shutdown(CommandExecutor *executor) {
  size_t i = 0u;

  if (!executor) {
    return;
  }

  for (i = 0u; i < executor->entry_count; ++i) {
    command_executor_discard_entry(&executor->entries[i]);
  }
  for (i = 0u; i < executor->transaction_count; ++i) {
    command_destroy(executor->transaction_commands[i]);
  }

  g_allocator.free_fn(executor->entries);
  g_allocator.free_fn(executor->transaction_commands);
  memset(executor, 0, sizeof(*executor));
}

int command_executor_execute(CommandExecutor *executor, Command *command,
                             Document *document) {
  if (!executor || !command || !command->vtable || !command->vtable->execute || !document) {
    command_destroy(command);
    return 0;
  }
  if (command_check_execute(command, document) != COMMAND_EXECUTE_CHECK_OK) {
    command_destroy(command);
    return 0;
  }

  if (!command->vtable->execute(command, document)) {
    command_destroy(command);
    return 0;
  }

  if (!command_executor_append_internal(executor, command)) {
    if (command->vtable->undo) {
      if (!command->vtable->undo(command, document)) {
        LOG_ERROR("%s", "command_executor_execute: undo after append failure also "
                         "failed - document may be in inconsistent state");
      }
    }
    command_destroy(command);
    return 0;
  }

  return 1;
}

int command_executor_record_executed(CommandExecutor *executor, Command *command) {
  if (!command_executor_append_internal(executor, command)) {
    command_destroy(command);
    return 0;
  }
  return 1;
}

int command_executor_undo(CommandExecutor *executor, Document *document) {
  Command *command = NULL;

  if (!executor || !document || executor->cursor == 0u) {
    return 0;
  }

  command = executor->entries[executor->cursor - 1u].command;
  if (!command || !command->vtable || !command->vtable->undo ||
      !command->vtable->undo(command, document)) {
    return 0;
  }

  executor->cursor--;
  command_executor_refresh_counts(executor);
  return 1;
}

int command_executor_redo(CommandExecutor *executor, Document *document) {
  Command *command = NULL;

  if (!executor || !document || executor->cursor >= executor->entry_count) {
    return 0;
  }

  command = executor->entries[executor->cursor].command;
  if (!command || !command->vtable || !command->vtable->redo ||
      !command->vtable->redo(command, document)) {
    return 0;
  }

  executor->cursor++;
  command_executor_refresh_counts(executor);
  return 1;
}

int command_executor_can_undo(const CommandExecutor *executor) {
  return executor && executor->cursor > 0u;
}

int command_executor_can_redo(const CommandExecutor *executor) {
  return executor && executor->cursor < executor->entry_count;
}

int command_executor_begin_transaction(CommandExecutor *executor) {
  if (!executor || executor->transaction_active) {
    return 0;
  }
  executor->transaction_active = 1;
  executor->transaction_count = 0u;
  executor->transaction_bytes = 0u;
  return 1;
}

int command_executor_commit_transaction(CommandExecutor *executor) {
  TransactionCommand *transaction = NULL;
  size_t i = 0u;

  if (!executor || !executor->transaction_active) {
    return 0;
  }

  executor->transaction_active = 0;
  if (executor->transaction_count == 0u) {
    return 1;
  }
  if (executor->transaction_count == 1u) {
    Command *command = executor->transaction_commands[0];
    executor->transaction_commands[0] = NULL;
    executor->transaction_count = 0u;
    executor->transaction_bytes = 0u;
    if (!command_executor_append_internal(executor, command)) {
      command_destroy(command);
      return 0;
    }
    return 1;
  }

  transaction = (TransactionCommand *)g_allocator.calloc_fn(1u, sizeof(*transaction));
  if (!transaction) {
    for (i = 0u; i < executor->transaction_count; ++i) {
      command_destroy(executor->transaction_commands[i]);
    }
    executor->transaction_count = 0u;
    executor->transaction_bytes = 0u;
    return 0;
  }

  transaction->commands = (Command **)g_allocator.calloc_fn(
      executor->transaction_count, sizeof(transaction->commands[0]));
  if (!transaction->commands) {
    g_allocator.free_fn(transaction);
    for (i = 0u; i < executor->transaction_count; ++i) {
      command_destroy(executor->transaction_commands[i]);
    }
    executor->transaction_count = 0u;
    executor->transaction_bytes = 0u;
    return 0;
  }

  transaction->base.vtable = &TRANSACTION_VTABLE;
  transaction->command_count = executor->transaction_count;
  for (i = 0u; i < executor->transaction_count; ++i) {
    transaction->commands[i] = executor->transaction_commands[i];
    executor->transaction_commands[i] = NULL;
  }
  executor->transaction_count = 0u;
  executor->transaction_bytes = 0u;
  if (!command_executor_append_internal(executor, (Command *)transaction)) {
    command_destroy((Command *)transaction);
    return 0;
  }
  return 1;
}

void command_executor_rollback_transaction(CommandExecutor *executor, Document *document) {
  size_t i = 0u;

  if (!executor || !executor->transaction_active) {
    return;
  }

  if (document) {
    for (i = executor->transaction_count; i > 0u; --i) {
      Command *command = executor->transaction_commands[i - 1u];
      if (command && command->vtable && command->vtable->undo) {
        if (!command->vtable->undo(command, document)) {
          LOG_ERROR("%s", "command_executor_rollback_transaction: undo failed during "
                           "transaction rollback - document may be in inconsistent state");
        }
      }
    }
  }
  for (i = 0u; i < executor->transaction_count; ++i) {
    command_destroy(executor->transaction_commands[i]);
    executor->transaction_commands[i] = NULL;
  }
  executor->transaction_active = 0;
  executor->transaction_count = 0u;
  executor->transaction_bytes = 0u;
}

void command_executor_set_memory_budget(CommandExecutor *executor,
                                        size_t memory_budget_bytes) {
  if (!executor) {
    return;
  }
  executor->memory_budget_bytes =
      memory_budget_bytes > 0u ? memory_budget_bytes : COMMAND_DEFAULT_MEMORY_BUDGET_BYTES;
  command_executor_prune_budget(executor);
}

Command *command_create_create_object(GraphicObject *object_snapshot) {
  CreateObjectCommand *command = NULL;

  if (!object_snapshot) {
    return NULL;
  }

  command = (CreateObjectCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    object_destroy(object_snapshot);
    return NULL;
  }

  command->base.vtable = &CREATE_OBJECT_VTABLE;
  command->object_snapshot = object_snapshot;
  command->object_id = object_snapshot->id;
  return (Command *)command;
}

Command *command_create_delete_selection(const Document *document,
                                         const SelectionSet *selection) {
  DeleteSelectionCommand *command = NULL;
  int i = 0;
  int count = 0;

  if (!document || !selection || selection->count <= 0) {
    return NULL;
  }

  command = (DeleteSelectionCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }
  command->base.vtable = &DELETE_SELECTION_VTABLE;
  command->objects = (GraphicObject **)g_allocator.calloc_fn((size_t)selection->count,
                                                             sizeof(command->objects[0]));
  command->original_indices = (int *)g_allocator.calloc_fn((size_t)selection->count,
                                                           sizeof(command->original_indices[0]));
  if (!command->objects || !command->original_indices) {
    delete_selection_destroy(command);
    return NULL;
  }

  for (i = 0; i < document->count; ++i) {
    const GraphicObject *object = document->objects[i];
    if (object && selection_set_contains(selection, object->id)) {
      command->objects[count] = object_clone(object);
      if (!command->objects[count]) {
        delete_selection_destroy(command);
        return NULL;
      }
      command->original_indices[count] = i;
      count++;
    }
  }

  if (count <= 0) {
    delete_selection_destroy(command);
    return NULL;
  }

  command->object_count = count;
  return (Command *)command;
}

Command *command_create_move_objects(const ObjectId *object_ids, int object_count,
                                     Vec2 delta) {
  MoveObjectsCommand *command = NULL;

  if (!object_ids || object_count <= 0 || vec2_length_sq(delta) <= 1e-6f) {
    return NULL;
  }

  command = (MoveObjectsCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->object_ids =
      (ObjectId *)g_allocator.calloc_fn((size_t)object_count, sizeof(command->object_ids[0]));
  if (!command->object_ids) {
    move_objects_destroy(command);
    return NULL;
  }

  command->base.vtable = &MOVE_OBJECTS_VTABLE;
  memcpy(command->object_ids, object_ids, (size_t)object_count * sizeof(object_ids[0]));
  command->object_count = object_count;
  command->delta = delta;
  return (Command *)command;
}

Command *command_create_paste_objects(const Document *document,
                                      GraphicObject *const *object_snapshots,
                                      int object_count,
                                      Vec2 delta,
                                      LayerId layer_id,
                                      SelectionSet *out_selection) {
  PasteObjectsCommand *command = NULL;
  int i = 0;

  if (!document || !object_snapshots || object_count <= 0 || layer_id == 0u) {
    return NULL;
  }

  command = (PasteObjectsCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->object_snapshots =
      (GraphicObject **)g_allocator.calloc_fn((size_t)object_count,
                                              sizeof(command->object_snapshots[0]));
  command->object_ids =
      (ObjectId *)g_allocator.calloc_fn((size_t)object_count, sizeof(command->object_ids[0]));
  if (!command->object_snapshots || !command->object_ids) {
    paste_objects_destroy(command);
    return NULL;
  }

  command->base.vtable = &PASTE_OBJECTS_VTABLE;
  command->object_count = object_count;
  command->layer_id = layer_id;
  if (out_selection) {
    selection_set_clear(out_selection);
    if (!selection_set_reserve(out_selection, object_count)) {
      paste_objects_destroy(command);
      return NULL;
    }
  }

  for (i = 0; i < object_count; ++i) {
    command->object_snapshots[i] = object_clone(object_snapshots[i]);
    if (!command->object_snapshots[i]) {
      paste_objects_destroy(command);
      return NULL;
    }

    object_translate(command->object_snapshots[i], delta);
    command->object_snapshots[i]->layer_id = layer_id;
    command->object_snapshots[i]->id = document->next_id + (ObjectId)i;
    command->object_ids[i] = command->object_snapshots[i]->id;

    if (out_selection) {
      out_selection->ids[i] = command->object_ids[i];
      out_selection->count++;
    }
  }

  return (Command *)command;
}

Command *command_create_set_property(ObjectId object_id, const char *key,
                                     float before_value, float after_value) {
  SetPropertyCommand *command = NULL;

  if (object_id == 0u || !key || key[0] == '\0' ||
      strlen(key) >= sizeof(((SetPropertyCommand *)0)->key)) {
    return NULL;
  }

  command = (SetPropertyCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &SET_PROPERTY_VTABLE;
  command->object_id = object_id;
  snprintf(command->key, sizeof(command->key), "%s", key);
  command->before_value = before_value;
  command->after_value = after_value;
  return (Command *)command;
}

Command *command_create_set_property_from_document(const Document *document,
                                                   ObjectId object_id, const char *key,
                                                   float after_value) {
  GraphicObject *object = NULL;
  float before_value = 0.0f;

  if (!document || object_id == 0u || !key || key[0] == '\0') {
    return NULL;
  }

  object = document_find_object(document, object_id);
  if (!object || !object_get_scalar(object, key, &before_value)) {
    return NULL;
  }

  return command_create_set_property(object_id, key, before_value, after_value);
}

Command *command_create_set_active_layer(const Document *document,
                                         LayerId after_layer_id) {
  SetActiveLayerCommand *command = NULL;

  if (!document || after_layer_id == 0u ||
      !document_layer_find_const(document, after_layer_id) ||
      document_active_layer_id(document) == after_layer_id) {
    return NULL;
  }

  command = (SetActiveLayerCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &SET_ACTIVE_LAYER_VTABLE;
  command->before_layer_id = document_active_layer_id(document);
  command->after_layer_id = after_layer_id;
  return (Command *)command;
}

Command *command_create_set_layer_visibility(const Document *document,
                                             LayerId layer_id,
                                             int visible) {
  const DocumentLayer *layer = NULL;
  SetLayerVisibilityCommand *command = NULL;

  if (!document || layer_id == 0u) {
    return NULL;
  }

  layer = document_layer_find_const(document, layer_id);
  if (!layer || layer->visible == (visible ? 1 : 0)) {
    return NULL;
  }

  command = (SetLayerVisibilityCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &SET_LAYER_VISIBILITY_VTABLE;
  command->layer_id = layer_id;
  command->before_visible = layer->visible;
  command->after_visible = visible ? 1 : 0;
  return (Command *)command;
}

Command *command_create_set_layer_locked(const Document *document,
                                         LayerId layer_id,
                                         int locked) {
  const DocumentLayer *layer = NULL;
  SetLayerLockedCommand *command = NULL;

  if (!document || layer_id == 0u) {
    return NULL;
  }

  layer = document_layer_find_const(document, layer_id);
  locked = locked ? 1 : 0;
  if (!layer || layer->locked == locked) {
    return NULL;
  }

  command = (SetLayerLockedCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &SET_LAYER_LOCKED_VTABLE;
  command->layer_id = layer_id;
  command->before_locked = layer->locked;
  command->after_locked = locked;
  return (Command *)command;
}

Command *command_create_rename_layer(const Document *document,
                                     LayerId layer_id,
                                     const char *name) {
  const DocumentLayer *layer = NULL;
  RenameLayerCommand *command = NULL;

  if (!document || layer_id == 0u || !name || name[0] == '\0') {
    return NULL;
  }

  layer = document_layer_find_const(document, layer_id);
  if (!layer || strcmp(layer->name, name) == 0) {
    return NULL;
  }

  command = (RenameLayerCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &RENAME_LAYER_VTABLE;
  command->layer_id = layer_id;
  snprintf(command->before_name, sizeof(command->before_name), "%s", layer->name);
  snprintf(command->after_name, sizeof(command->after_name), "%s", name);
  return (Command *)command;
}

Command *command_create_move_layer(const Document *document,
                                   LayerId layer_id,
                                   int target_index) {
  MoveLayerCommand *command = NULL;
  int current_index = 0;

  if (!document || layer_id == 0u) {
    return NULL;
  }

  current_index = document_layer_index(document, layer_id);
  if (current_index < 0) {
    return NULL;
  }

  if (target_index < 0) {
    target_index = 0;
  }
  if (target_index >= document_layer_count(document)) {
    target_index = document_layer_count(document) - 1;
  }
  if (current_index == target_index) {
    return NULL;
  }

  command = (MoveLayerCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &MOVE_LAYER_VTABLE;
  command->layer_id = layer_id;
  command->before_index = current_index;
  command->after_index = target_index;
  return (Command *)command;
}

Command *command_create_create_layer(const char *name) {
  CreateLayerCommand *command = NULL;

  command = (CreateLayerCommand *)g_allocator.calloc_fn(1u, sizeof(*command));
  if (!command) {
    return NULL;
  }

  command->base.vtable = &CREATE_LAYER_VTABLE;
  snprintf(command->layer.name, sizeof(command->layer.name), "%s",
           (name && name[0] != '\0') ? name : "Layer");
  command->layer.visible = 1;
  command->layer.locked = 0;
  command->layer.blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
  command->layer_index = -1;
  command->previous_active_layer_id = 0u;
  command->created_once = 0;
  return (Command *)command;
}
