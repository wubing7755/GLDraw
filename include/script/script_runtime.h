#ifndef GLDRAW_SCRIPT_SCRIPT_RUNTIME_H
#define GLDRAW_SCRIPT_SCRIPT_RUNTIME_H

#include <commands/command.h>
#include <document/document.h>
#include <model/selection.h>
#include <tools/tool.h>

typedef struct ScriptRuntime ScriptRuntime;

int script_runtime_init(ScriptRuntime* runtime);
void script_runtime_shutdown(ScriptRuntime* runtime);
void script_runtime_set_context(ScriptRuntime* runtime,
                                Document* document,
                                SelectionSet* selection,
                                const ToolPorts* ports);
int script_runtime_execute_file_event(ScriptRuntime* runtime,
                                      const char* script_path,
                                      const char* event_name,
                                      const ToolEvent* event);

#endif /* GLDRAW_SCRIPT_SCRIPT_RUNTIME_H */
