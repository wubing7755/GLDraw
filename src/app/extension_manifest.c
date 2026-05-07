/**
 * @file extension_manifest.c
 * @brief Central object registration manifest plus legacy extension-loader wrappers.
 *
 * To add a new object type extension, write its implementation file with a
 * register_*() function, then add one entry to g_object_extensions[].
 * No other source files need modification.
 *
 * Tool extensions are registered via app/tool_manifest.c, which follows the
 * same manifest-based pattern as object types.
 */
#include <app/extension_loader.h>
#include <app/object_manifest.h>

#include "app/manifest_runner.h"

/* ---------------------------------------------------------------------------
 * Forward declarations — each object type extension provides register_*().
 * --------------------------------------------------------------------------- */

int register_line_object_type(void);
int register_rect_object_type(void);
int register_ellipse_object_type(void);
int register_fake_star_object_extension(void);

/* ---------------------------------------------------------------------------
 * Manifest — the single place where object type extensions are listed.
 * --------------------------------------------------------------------------- */

static const ManifestInitFn g_object_extensions[] = {
    register_line_object_type,
    register_rect_object_type,
    register_ellipse_object_type,
    register_fake_star_object_extension,
};

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

int object_manifest_register_builtins(void)
{
    /* Core object types only (skip fake_star, which is the last entry). */
    return manifest_runner_run(
        g_object_extensions,
        sizeof(g_object_extensions) / sizeof(g_object_extensions[0]) - 1,
        MANIFEST_RUN_CONTINUE_ON_FAILURE);
}

int object_manifest_register_fake_star(void)
{
    return register_fake_star_object_extension();
}

int object_manifest_register_all(void)
{
    return manifest_runner_run(
        g_object_extensions,
        sizeof(g_object_extensions) / sizeof(g_object_extensions[0]),
        MANIFEST_RUN_CONTINUE_ON_FAILURE);
}

int extension_loader_register_builtins(void)
{
    return object_manifest_register_builtins();
}

int extension_loader_register_fake_star(void)
{
    return object_manifest_register_fake_star();
}

int extension_loader_register_all(void)
{
    return object_manifest_register_all();
}
