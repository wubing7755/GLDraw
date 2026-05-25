/**
 * @file registration_manifest.c
 * @brief Unified application-level registration entrypoint for built-in manifests.
 */
#include <app/registration_manifest.h>

#include <app/object_manifest.h>
#include <app/tool_manifest.h>

int app_register_all_manifests(void)
{
    static int registered = 0;

    if (registered) {
        return 1;
    }

    if (!object_manifest_register_all()) {
        return 0;
    }

    if (!tool_manifest_register_all()) {
        return 0;
    }

    registered = 1;
    return 1;
}
