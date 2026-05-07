/**
 * @file registration_manifest.c
 * @brief Unified application-level registration entrypoint for built-in manifests.
 */
#include <app/registration_manifest.h>

#include <app/object_manifest.h>
#include <app/tool_manifest.h>

int app_register_all_manifests(void)
{
    if (!object_manifest_register_all()) {
        return 0;
    }

    if (!tool_manifest_register_all()) {
        return 0;
    }

    return 1;
}
