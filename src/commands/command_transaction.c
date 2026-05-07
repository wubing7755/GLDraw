#include "command_internal.h"

#include <base/log.h>

static int transaction_execute(void* cmd, Document* document)
{
    TransactionCommand* transaction = (TransactionCommand*)cmd;
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

static int transaction_undo(void* cmd, Document* document)
{
    TransactionCommand* transaction = (TransactionCommand*)cmd;
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

static int transaction_merge(void* cmd, const void* next)
{
    (void)cmd;
    (void)next;
    return 0;
}

static void transaction_destroy(void* cmd)
{
    TransactionCommand* transaction = (TransactionCommand*)cmd;
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

const CommandVTable TRANSACTION_VTABLE = {transaction_execute, transaction_undo,
                                          transaction_execute, transaction_merge,
                                          transaction_destroy};

int command_executor_begin_transaction(CommandExecutor* executor)
{
    if (!executor || executor->transaction_active) {
        return 0;
    }

    executor->transaction_active = 1;
    executor->transaction_count = 0u;
    executor->transaction_bytes = 0u;
    return 1;
}

int command_executor_commit_transaction(CommandExecutor* executor)
{
    TransactionCommand* transaction = NULL;
    size_t i = 0u;

    if (!executor || !executor->transaction_active) {
        return 0;
    }

    executor->transaction_active = 0;
    if (executor->transaction_count == 0u) {
        return 1;
    }
    if (executor->transaction_count == 1u) {
        Command* command = executor->transaction_commands[0];
        executor->transaction_commands[0] = NULL;
        executor->transaction_count = 0u;
        executor->transaction_bytes = 0u;
        if (!command_executor_append_internal(executor, command)) {
            command_destroy(command);
            return 0;
        }
        return 1;
    }

    transaction = (TransactionCommand*)g_allocator.calloc_fn(1u, sizeof(*transaction));
    if (!transaction) {
        for (i = 0u; i < executor->transaction_count; ++i) {
            command_destroy(executor->transaction_commands[i]);
        }
        executor->transaction_count = 0u;
        executor->transaction_bytes = 0u;
        return 0;
    }

    transaction->commands = (Command**)g_allocator.calloc_fn(
        executor->transaction_count,
        sizeof(transaction->commands[0]));
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
    if (!command_executor_append_internal(executor, (Command*)transaction)) {
        command_destroy((Command*)transaction);
        return 0;
    }
    return 1;
}

void command_executor_rollback_transaction(CommandExecutor* executor, Document* document)
{
    size_t i = 0u;

    if (!executor || !executor->transaction_active) {
        return;
    }

    if (document) {
        for (i = executor->transaction_count; i > 0u; --i) {
            Command* command = executor->transaction_commands[i - 1u];
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
