/**
 * @file extension_manifest.c
 * @brief Central extension registration manifest for object types.
 *
 * To add a new object type extension, write its implementation file with a
 * register_*() function, then add one entry to g_object_extensions[].
 * No other source files need modification.
 *
 * Tool extensions are registered via register_builtin_tools() in
 * tool_controller.c, which also uses a manifest-based pattern.
 */
#include <app/extension_loader.h>

#include <stddef.h>

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

typedef int (*ExtensionInitFn)(void);

static const ExtensionInitFn g_object_extensions[] = {
    register_line_object_type,
    register_rect_object_type,
    register_ellipse_object_type,
    register_fake_star_object_extension,
};

static int run_manifest(const ExtensionInitFn* fns, size_t count)
{
    size_t i = 0;
    int ok = 1;

    for (i = 0; i < count; ++i) {
        if (!fns[i]()) {
            ok = 0;
        }
    }
    return ok;
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

int extension_loader_register_builtins(void)
{
    /* Core object types only (skip fake_star, which is the last entry). */
    return run_manifest(g_object_extensions,
                        sizeof(g_object_extensions) / sizeof(g_object_extensions[0]) - 1);
}

int extension_loader_register_fake_star(void)
{
    return register_fake_star_object_extension();
}

int extension_loader_register_all(void)
{
    return run_manifest(g_object_extensions,
                        sizeof(g_object_extensions) / sizeof(g_object_extensions[0]));
}
