#include "app/manifest_runner.h"

int manifest_runner_run(const ManifestInitFn* fns, size_t count, ManifestRunMode mode)
{
    size_t i = 0;
    int ok = 1;

    if (!fns) {
        return 0;
    }

    for (i = 0; i < count; ++i) {
        if (!fns[i]()) {
            ok = 0;
            if (mode == MANIFEST_RUN_STOP_ON_FAILURE) {
                return 0;
            }
        }
    }

    return ok;
}
