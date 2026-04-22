/**
 * @file application.h
 * @brief Application layer entry point interface.
 */
#ifndef GLDRAW_APP_APPLICATION_H
#define GLDRAW_APP_APPLICATION_H

/**
 * @brief Start and run the GLDraw main loop.
 *
 * This function initializes the window, rendering system, UI system, and workspace,
 * and performs cleanup and finalization on exit.
 *
 * @return `0` on normal exit; `-1` on initialization failure or unrecoverable runtime error.
 */
int app_run(void);

#endif /* GLDRAW_APP_APPLICATION_H */
