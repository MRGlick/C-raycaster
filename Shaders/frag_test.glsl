#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform vec2 texResolution;

void main(void)
{
    // Sample the texture at the given texture coordinates
    vec4 texColor = texture(tex, texCoord);

    if (texColor.r + texColor.g + texColor.b > 1.5) {
        fragColor = texColor;
    } else {
        fragColor = vec4(texColor.rgb * 0.15, 1.0);
    }

}
