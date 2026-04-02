#version 330 core

// 从顶点着色器接收的颜色（每顶点颜色）
in vec3 vertexColor;

// 统一的颜色（来自 C 语言的滑块）
uniform vec3 u_color;

// 最终输出颜色
out vec4 FragColor;

void main()
{
    // 混合：70% 滑块颜色 + 30% 顶点颜色
    // 这样既能用滑块控制整体色调，又能看到顶点属性的效果
    vec3 finalColor = mix(u_color, vertexColor, 0.3);
    FragColor = vec4(finalColor, 1.0);
}
