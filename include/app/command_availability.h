/**
 * @file command_availability.h
 * @brief Editor command availability and unavailable-reason queries.
 */
#ifndef GLDRAW_APP_COMMAND_AVAILABILITY_H
#define GLDRAW_APP_COMMAND_AVAILABILITY_H

#include <app/command_registry.h>

struct Workspace;

/** Check whether a command can run in the current workspace state. */
int command_availability_is_available(const struct Workspace* workspace,
                                      EditorCommand command);

/**
 * Get a short human-readable reason when a command is unavailable.
 * Returns an empty string when the command is available or no specific reason is known.
 */
const char* command_availability_unavailable_reason(const struct Workspace* workspace,
                                                    EditorCommand command);

/** Check whether a menu-backed command is available in the current workspace state. */
int command_availability_is_menu_action_available(const struct Workspace* workspace,
                                                  int id);

#endif /* GLDRAW_APP_COMMAND_AVAILABILITY_H */
