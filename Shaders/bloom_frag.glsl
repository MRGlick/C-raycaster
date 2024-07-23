#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform vec2 texResolution; // resolution of the texture

void main()
{
    vec2 texOffset = 1.0 / texResolution; // size of a texel

    float pi = 6.28318530718;
    
    float directions = 26.0; // Default 16.0
    float quality = 3.0; // Default 4.0
    float size = 12.0;
    float thresh = 2.0;
   
    vec2 Radius = size / texResolution;
    
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = texCoord;
    // pixel colour
    vec4 Color = texture(tex, uv);
    
    // Blur calculations
    for(float d = 0.0; d < pi; d += pi / directions)
    {
        for(float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality)
        {   
            vec4 col = texture(tex, uv + vec2(cos(d), sin(d)) * Radius * i);

            if (col.r + col.b + col.g > thresh) {
                Color += col;
            }

        }
    }
    
    // Output to screen
    Color /= quality * directions;
    fragColor = Color;

    // Combine the original color with the blurred color for the bloom effect
    fragColor = texture(tex, texCoord) + Color * 0.8;
}
