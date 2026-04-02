#include <stdio.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <core/renderer.h>
#include <core/shader.h>

static GLuint s_VAO = 0;
static GLuint s_VBO = 0;
static GLuint s_EBO = 0;

/* Encapsulated triangle geometry - not scattered across static globals */
typedef struct {
    /* Interleaved vertex data: (x, y, r, g, b) per vertex */
    float vertices[15];
    unsigned int indices[3];
} TriangleGeometry;

static const TriangleGeometry s_triangle = {
    .vertices = {
         0.0f,  0.5f,  1.0f, 0.0f, 0.0f,   /* top    - red   */
        -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   /* bottom - green */
         0.5f, -0.5f,  0.0f, 0.0f, 1.0f    /* right  - blue  */
    },
    .indices = { 0, 1, 2 }
};

int init_renderer(void)
{
    /* Step 1: Create VAO (Vertex Array Object) */
    glGenVertexArrays(1, &s_VAO);
    glBindVertexArray(s_VAO);
    printf("[Renderer] VAO created: %u\n", s_VAO);

    /* Step 2: Create VBO (Vertex Buffer Object) */
    glGenBuffers(1, &s_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_triangle.vertices),
                 s_triangle.vertices, GL_STATIC_DRAW);
    printf("[Renderer] VBO created and data uploaded\n");

    /* Step 3: Create EBO (Element Buffer Object) */
    glGenBuffers(1, &s_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_triangle.indices),
                 s_triangle.indices, GL_STATIC_DRAW);
    printf("[Renderer] EBO created and data uploaded\n");

    /* Step 4: Configure vertex attributes */
    /* Attribute 0: position (x, y) - 2 floats */
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    /* Attribute 1: color (r, g, b) - 3 floats, offset by 2 */
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    printf("[Renderer] Vertex attributes configured\n");

    glBindVertexArray(0);

    return 0;
}

void render_frame(const AppState* state)
{
    if (!state)
    {
        return;
    }

    /* 1. Clear screen */
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* 2. Use shader program - via shader module's setters */
    shader_set_color(state->color[0], state->color[1], state->color[2]);

    /* Pass position offset to GPU via shader module */
    shader_set_offset(0.0f, state->offset_y);

    /* 3. Bind VAO and draw */
    glBindVertexArray(s_VAO);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

    /* 4. Unbind */
    glBindVertexArray(0);
}

void cleanup_renderer(void)
{
    glDeleteVertexArrays(1, &s_VAO);
    glDeleteBuffers(1, &s_VBO);
    glDeleteBuffers(1, &s_EBO);
    s_VAO = 0;
    s_VBO = 0;
    s_EBO = 0;
    printf("[Renderer] Cleaned up\n");
}
