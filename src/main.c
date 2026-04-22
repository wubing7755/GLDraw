/**
 * @file main.c
 * @brief Process entry point.
 *
 * Role in project:
 * - Minimal bridge from OS runtime into `app_run()`.
 *
 * Module relationships:
 * - Delegates all real lifecycle work to `app/application`.
 */
#include <app/application.h>

/**
 * @brief Program entry.
 * @return Propagates `app_run()` result.
 *
 * Edge cases:
 * - Failures are handled inside `app_run()`.
 */

/**
 * @brief Program entry point.
 * @param void No parameters.
 * @return Application exit code.
 */
int main(void)
{
    return app_run();
}
