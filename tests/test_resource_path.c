#include <base/resource_path.h>

#include <stdio.h>
#include <string.h>

static int expect_true(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "[FAIL] %s\n", message);
        return 0;
    }
    return 1;
}

static int write_marker_file(const char* path)
{
    FILE* file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    fputs("resource-path-test", file);
    fclose(file);
    return 1;
}

int main(void)
{
    char resolved[260];
    const char* marker_path = "gldraw_resource_path_test.tmp";

    remove(marker_path);
    if (!expect_true(write_marker_file(marker_path), "failed to create marker file")) {
        return 1;
    }

    if (!expect_true(resource_path_resolve(marker_path, resolved, sizeof(resolved)),
                     "existing cwd resource should resolve")) {
        remove(marker_path);
        return 1;
    }
    if (!expect_true(strcmp(resolved, marker_path) == 0,
                     "cwd resource should keep the relative path")) {
        remove(marker_path);
        return 1;
    }

    remove(marker_path);

    if (!expect_true(!resource_path_resolve("missing-resource-path-test.file",
                                            resolved,
                                            sizeof(resolved)),
                     "missing resource should report unresolved")) {
        return 1;
    }
    if (!expect_true(strcmp(resolved, "missing-resource-path-test.file") == 0,
                     "missing resource should fall back to original path")) {
        return 1;
    }

    printf("[PASS] resource path lookup preserves cwd behavior and fallback\n");
    return 0;
}
