#ifndef INPUT_H
#define INPUT_H

#include <GLFW/glfw3.h>

void init_input(GLFWwindow* window);
void process_input(void);

/* Input callbacks update shared state directly.
 * Callbacks are registered internally, state is owned elsewhere. */

#endif /* INPUT_H */
