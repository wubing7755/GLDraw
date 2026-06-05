#include "command_internal.h"

#include <base/log.h>

#include <stdlib.h>
#include <string.h>

static void* command_default_malloc(size_t size) { return malloc(size); }
static void* command_default_calloc(size_t count, size_t size) { return calloc(count, size); }
static void* command_default_realloc(void* ptr, size_t size) { return realloc(ptr, size); }
static void command_default_free(void* ptr) { free(ptr); }

CommandAllocator g_allocator = {command_default_malloc, command_default_calloc,
                                command_default_realloc, command_default_free};

static void command_executor_refresh_counts(CommandExecutor* executor)
{
    if (!executor) {
        return;
    }

    executor->undo_count = executor->cursor;
    executor->redo_count =
        (executor->entry_count >= executor->cursor) ? (executor->entry_count - executor->cursor) : 0u;
}

static int command_reserve_log(CommandExecutor* executor, size_t needed)
{
    CommandLogEntry* entries = NULL;
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

    entries = (CommandLogEntry*)g_allocator.realloc_fn(
        executor->entries,
        capacity * sizeof(executor->entries[0]));
    if (!entries) {
        return 0;
    }

    executor->entries = entries;
    executor->entry_capacity = capacity;
    return 1;
}

static int command_reserve_transaction(CommandExecutor* executor, size_t needed)
{
    Command** commands = NULL;
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

    commands = (Command**)g_allocator.realloc_fn(
        executor->transaction_commands,
        capacity * sizeof(executor->transaction_commands[0]));
    if (!commands) {
        return 0;
    }

    executor->transaction_commands = commands;
    executor->transaction_capacity = capacity;
    return 1;
}

static void command_executor_discard_entry(CommandLogEntry* entry)
{
    if (!entry) {
        return;
    }

    command_destroy(entry->command);
    entry->command = NULL;
    entry->bytes = 0u;
}

static void command_executor_clear_tail(CommandExecutor* executor)
{
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

static void command_executor_prune_budget(CommandExecutor* executor)
{
    size_t prune_count = 0u;
    size_t remaining_count = 0u;

    if (!executor || executor->memory_budget_bytes == 0u) {
        return;
    }

    while (executor->used_bytes > executor->memory_budget_bytes &&
           prune_count + 1u < executor->cursor) {
        executor->used_bytes =
            (executor->used_bytes >= executor->entries[prune_count].bytes)
                ? executor->used_bytes - executor->entries[prune_count].bytes
                : 0u;
        command_executor_discard_entry(&executor->entries[prune_count]);
        prune_count++;
    }

    if (prune_count > 0u) {
        remaining_count = executor->entry_count - prune_count;
        memmove(executor->entries,
                executor->entries + prune_count,
                remaining_count * sizeof(executor->entries[0]));
        memset(executor->entries + remaining_count,
               0,
               prune_count * sizeof(executor->entries[0]));
        executor->entry_count = remaining_count;
        executor->cursor -= prune_count;
    }
    command_executor_refresh_counts(executor);
}

int command_executor_append_internal(CommandExecutor* executor, Command* command)
{
    Command* previous = NULL;
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

void command_set_allocator(const CommandAllocator* allocator)
{
    if (!allocator || !allocator->malloc_fn || !allocator->calloc_fn ||
        !allocator->realloc_fn || !allocator->free_fn) {
        command_reset_allocator();
        return;
    }

    g_allocator = *allocator;
}

void command_reset_allocator(void)
{
    g_allocator.malloc_fn = command_default_malloc;
    g_allocator.calloc_fn = command_default_calloc;
    g_allocator.realloc_fn = command_default_realloc;
    g_allocator.free_fn = command_default_free;
}

int command_executor_init_with_budget(CommandExecutor* executor,
                                      size_t memory_budget_bytes)
{
    if (!executor) {
        return 0;
    }

    memset(executor, 0, sizeof(*executor));
    executor->memory_budget_bytes =
        memory_budget_bytes > 0u ? memory_budget_bytes : COMMAND_DEFAULT_MEMORY_BUDGET_BYTES;
    return 1;
}

int command_executor_init(CommandExecutor* executor)
{
    return command_executor_init_with_budget(executor, COMMAND_DEFAULT_MEMORY_BUDGET_BYTES);
}

void command_executor_shutdown(CommandExecutor* executor)
{
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

int command_executor_execute(CommandExecutor* executor, Command* command,
                             Document* document)
{
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

int command_executor_record_executed(CommandExecutor* executor, Command* command)
{
    if (!command_executor_append_internal(executor, command)) {
        command_destroy(command);
        return 0;
    }

    return 1;
}

int command_executor_undo(CommandExecutor* executor, Document* document)
{
    Command* command = NULL;

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

int command_executor_redo(CommandExecutor* executor, Document* document)
{
    Command* command = NULL;

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

int command_executor_can_undo(const CommandExecutor* executor)
{
    return executor && executor->cursor > 0u;
}

int command_executor_can_redo(const CommandExecutor* executor)
{
    return executor && executor->cursor < executor->entry_count;
}

void command_executor_set_memory_budget(CommandExecutor* executor,
                                        size_t memory_budget_bytes)
{
    if (!executor) {
        return;
    }

    executor->memory_budget_bytes =
        memory_budget_bytes > 0u ? memory_budget_bytes : COMMAND_DEFAULT_MEMORY_BUDGET_BYTES;
    command_executor_prune_budget(executor);
}
