#version 330 core

// 顶点属性 - 与 VAO 配置对应
// layout (location = 0): 位置 (x, y)
// layout (location = 1): 颜色 (r, g, b)

layout (location = 0) in vec2 aPos;    // 顶点位置
layout (location = 1) in vec3 aColor; // 顶点颜色

// 统一变量 - 位置偏移（W/S 键控制）
uniform vec2 u_offset;

// 输出到片段着色器
out vec3 vertexColor;

void main()
{
    // 将顶点位置加上偏移量
    gl_Position = vec4(aPos + u_offset, 0.0, 1.0);
    vertexColor = aColor;
}
