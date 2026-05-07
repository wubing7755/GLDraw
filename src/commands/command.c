#include <commands/command.h>

#include "command_internal.h"

void command_destroy(Command *command) {
  if (command && command->vtable && command->vtable->destroy) {
    command->vtable->destroy(command);
  }
}

size_t command_measure(const Command *command) {
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
