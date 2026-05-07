#ifndef GLDRAW_APP_MANIFEST_RUNNER_H
#define GLDRAW_APP_MANIFEST_RUNNER_H

#include <stddef.h>

typedef int (*ManifestInitFn)(void);

typedef enum ManifestRunMode {
    MANIFEST_RUN_CONTINUE_ON_FAILURE = 0,
    MANIFEST_RUN_STOP_ON_FAILURE
} ManifestRunMode;

int manifest_runner_run(const ManifestInitFn* fns, size_t count, ManifestRunMode mode);

#endif /* GLDRAW_APP_MANIFEST_RUNNER_H */
