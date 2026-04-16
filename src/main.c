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
 * @param None.
 * @return Propagates `app_run()` result.
 *
 * Edge cases:
 * - None here; failures are handled inside `app_run()`.
 *
 * Time complexity: `O(1)` for this wrapper.
 */
int main(void)
{
    return app_run();
}
