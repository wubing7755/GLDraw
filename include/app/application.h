/**
 * @file application.h
 * @brief Process-level application entry point.
 *
 * Role in project:
 * - Exposes the single runtime entry used by `main`.
 * - Owns startup, main loop, and shutdown orchestration.
 *
 * Module relationships:
 * - Called by `src/main.c`.
 * - Coordinates workspace, window, render, tools, and UI systems.
 */
#ifndef GLDRAW_APP_APPLICATION_H
#define GLDRAW_APP_APPLICATION_H

/**
 * @brief Run the GLDraw application loop.
 * @param None.
 * @return `0` on normal exit, `-1` on unrecoverable init/runtime failure.
 *
 * Edge cases:
 * - Returns early if critical resources fail to initialize.
 * - Performs internal cleanup on failure paths.
 *
 * Time complexity:
 * - Init/teardown are effectively `O(1)`.
 * - Per-frame work depends on scene size and active UI/tool state.
 */
int app_run(void);

#endif /* GLDRAW_APP_APPLICATION_H */
