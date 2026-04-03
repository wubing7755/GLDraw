#version 330 core

/* Per-vertex color from vertex shader */
in vec4 vertexColor;

/* Final fragment color — direct pass-through from vertex color */
out vec4 FragColor;

void main()
{
    FragColor = vertexColor;
}
