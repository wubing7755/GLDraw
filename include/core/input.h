#ifndef INPUT_H
#define INPUT_H

#include <GLFW/glfw3.h>

typedef struct Tool Tool;

void init_input(GLFWwindow* window);
void input_init_tools(Tool* draw_tool, Tool* select_tool, Tool* default_tool);
void process_input(void);
SelectionManager* input_get_selection(void);

#endif /* INPUT_H */
