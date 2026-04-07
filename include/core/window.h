#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "GLDraw"

int init_window(void);
GLFWwindow* window_get_handle(void);
void poll_events(void);
int window_should_close(void);
void swap_buffers(void);
void shutdown_window(void);

#endif /* WINDOW_H */
