#version 330 core

/* Vertex attribute — matches renderer vertex format: x, y, r, g, b, a (6 floats) */
layout (location = 0) in vec2 aPos;    /* position */
layout (location = 1) in vec4 aColor;  /* RGBA color */

/* Output to fragment shader */
out vec4 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
}
