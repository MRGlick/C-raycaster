#version 120

attribute vec2 gpu_Vertex;
attribute vec2 gpu_TexCoord;
attribute vec4 gpu_Color;
uniform mat4 gpu_ModelViewProjectionMatrix;

varying vec2 texCoord;
varying vec4 color;

void main() {
    texCoord = gpu_TexCoord;
    color = gpu_Color;
    gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);
}