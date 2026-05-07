/**
 * @file tool_manifest.c
 * @brief Central tool registration manifest for built-in tool descriptors.
 */
#include <app/tool_manifest.h>

#include "app/manifest_runner.h"
#include <tools/tool_controller.h>

#if defined(GLDRAW_ENABLE_SCRIPTING)
void gldraw_register_script_tool(void);
#endif

int register_select_tool(void);
int register_pan_tool(void);
int register_shape_tool_line(void);
int register_shape_tool_rect(void);
int register_shape_tool_ellipse(void);

static const ManifestInitFn g_tool_extensions[] = {
    register_select_tool,
    register_pan_tool,
    register_shape_tool_line,
    register_shape_tool_rect,
    register_shape_tool_ellipse,
};

int tool_manifest_register_all(void)
{
    tool_registry_init();
    if (!manifest_runner_run(g_tool_extensions,
                             sizeof(g_tool_extensions) / sizeof(g_tool_extensions[0]),
                             MANIFEST_RUN_STOP_ON_FAILURE)) {
        return 0;
    }

#if defined(GLDRAW_ENABLE_SCRIPTING)
    gldraw_register_script_tool();
#endif
    return 1;
}
