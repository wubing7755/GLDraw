#ifndef NUKLEAR_UI_H
#define NUKLEAR_UI_H

#include <GLFW/glfw3.h>

int init_nuklear_ui(GLFWwindow* window);
void nuklear_new_frame(void);
void nuklear_build_ui(void);
void nuklear_render(void);
void shutdown_nuklear_ui(void);

#endif /* NUKLEAR_UI_H */
