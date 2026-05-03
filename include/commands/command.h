/**
 * @file command.h
 * @brief Undoable editor command interface and executor.
 */
#ifndef GLDRAW_COMMANDS_COMMAND_H
#define GLDRAW_COMMANDS_COMMAND_H

#include <base/types.h>
#include <document/document.h>
#include <model/selection.h>

#include <stddef.h>

typedef struct CommandVTable {
    int (*execute)(void* cmd, Document* doc);
    int (*undo)(void* cmd, Document* doc);
    int (*redo)(void* cmd, Document* doc);
    int (*merge)(void* cmd, const void* next);
    void (*destroy)(void* cmd);
} CommandVTable;

typedef struct Command {
    const CommandVTable* vtable;
} Command;

typedef struct CommandLogEntry {
    Command* command;
    size_t bytes;
} CommandLogEntry;

typedef struct CommandAllocator {
    void* (*malloc_fn)(size_t size);
    void* (*calloc_fn)(size_t count, size_t size);
    void* (*realloc_fn)(void* ptr, size_t size);
    void (*free_fn)(void* ptr);
} CommandAllocator;

typedef enum CommandExecuteCheck {
    COMMAND_EXECUTE_CHECK_OK = 0,
    COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT,
    COMMAND_EXECUTE_CHECK_TARGET_MISSING,
    COMMAND_EXECUTE_CHECK_TARGET_OBJECT_LOCKED,
    COMMAND_EXECUTE_CHECK_TARGET_LAYER_LOCKED
} CommandExecuteCheck;

typedef struct CommandExecutor {
    CommandLogEntry* entries;
    size_t entry_count;
    size_t entry_capacity;
    size_t cursor;
    size_t used_bytes;
    size_t memory_budget_bytes;
    Command** transaction_commands;
    size_t transaction_count;
    size_t transaction_capacity;
    size_t transaction_bytes;
    int transaction_active;
    size_t undo_count;
    size_t redo_count;
} CommandExecutor;

void command_set_allocator(const CommandAllocator* allocator);
void command_reset_allocator(void);

int command_executor_init(CommandExecutor* executor);
int command_executor_init_with_budget(CommandExecutor* executor, size_t memory_budget_bytes);
void command_executor_shutdown(CommandExecutor* executor);
CommandExecuteCheck command_check_execute(const Command* command, const Document* document);
int command_can_execute(const Command* command, const Document* document);
const char* command_execute_check_message(CommandExecuteCheck check);
int command_executor_execute(CommandExecutor* executor, Command* command, Document* document);
int command_executor_record_executed(CommandExecutor* executor, Command* command);
int command_executor_undo(CommandExecutor* executor, Document* document);
int command_executor_redo(CommandExecutor* executor, Document* document);
int command_executor_can_undo(const CommandExecutor* executor);
int command_executor_can_redo(const CommandExecutor* executor);
int command_executor_begin_transaction(CommandExecutor* executor);
int command_executor_commit_transaction(CommandExecutor* executor);
void command_executor_rollback_transaction(CommandExecutor* executor, Document* document);
void command_executor_set_memory_budget(CommandExecutor* executor, size_t memory_budget_bytes);

Command* command_create_create_object(GraphicObject* object_snapshot);
Command* command_create_delete_selection(const Document* document, const SelectionSet* selection);
Command* command_create_move_objects(const ObjectId* object_ids, int object_count, Vec2 delta);
Command* command_create_paste_objects(const Document* document,
                                      GraphicObject* const* object_snapshots,
                                      int object_count,
                                      Vec2 delta,
                                      LayerId layer_id,
                                      SelectionSet* out_selection);
Command* command_create_set_property(ObjectId object_id,
                                     const char* key,
                                     float before_value,
                                     float after_value);
Command* command_create_set_property_from_document(const Document* document,
                                                   ObjectId object_id,
                                                   const char* key,
                                                   float after_value);
Command* command_create_set_active_layer(const Document* document,
                                         LayerId after_layer_id);
Command* command_create_set_layer_visibility(const Document* document,
                                             LayerId layer_id,
                                             int visible);
Command* command_create_set_layer_locked(const Document* document,
                                         LayerId layer_id,
                                         int locked);
Command* command_create_rename_layer(const Document* document,
                                     LayerId layer_id,
                                     const char* name);
Command* command_create_move_layer(const Document* document,
                                   LayerId layer_id,
                                   int target_index);
Command* command_create_create_layer(const char* name);

#endif /* GLDRAW_COMMANDS_COMMAND_H */
