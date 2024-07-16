#version 120

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform vec2 texResolution;

void main() {
    texCoord = gpu_TexCoord;
    color = gpu_Color;
    gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);
}