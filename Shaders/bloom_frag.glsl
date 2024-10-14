// #version 330 core

// in vec4 color;
// in vec2 texCoord;
// out vec4 fragColor;

// uniform sampler2D tex;
// uniform vec2 texResolution; // resolution of the texture

// void main()
// {
//     vec2 texOffset = 1.0 / texResolution; // size of a texel

//     float pi = 6.28318530718;
    
//     float directions = 26.0; // Default 16.0
//     float quality = 3.0; // Default 4.0
//     float size = 12.0;
//     float thresh = 2.0;
   
//     vec2 Radius = size / texResolution;
    
//     // Normalized pixel coordinates (from 0 to 1)
//     vec2 uv = texCoord;
//     // pixel colour
//     vec4 Color = texture(tex, uv);
    
//     // Blur calculations
//     for(float d = 0.0; d < pi; d += pi / directions)
//     {
//         for(float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality)
//         {   
//             vec4 col = texture(tex, uv + vec2(cos(d), sin(d)) * Radius * i);

//             if (col.r + col.b + col.g > thresh) {
//                 Color += col;
//             }

//         }
//     }
    
//     // Output to screen
//     Color /= quality * directions;
//     fragColor = Color;

//     // Combine the original color with the blurred color for the bloom effect
//     fragColor = texture(tex, texCoord) + Color * 0.8;
// }

#version 120
uniform sampler2D sceneTexture;
uniform vec2 resolution;
uniform float threshold;
uniform float bloomIntensity;

varying vec2 v_TexCoord;

void main() {
    vec2 texCoord = v_TexCoord;
    vec3 sceneColor = texture2D(sceneTexture, texCoord).rgb;

    // 1. Brightness check and threshold pass
    vec3 brightColor = max(sceneColor - vec3(threshold), vec3(0.0));

    // 2. Gaussian blur (approximate, single pass)
    // Sample offsets for blur (based on 5-tap separable Gaussian)
    float offsets[5] = float[]( -2.0, -1.0, 0.0, 1.0, 2.0 );
    float weights[5] = float[]( 0.12, 0.23, 0.3, 0.23, 0.12 );

    vec3 blurredColor = vec3(0.0);
    for (int i = 0; i < 5; ++i) {
        vec2 offsetCoordH = vec2(offsets[i] / resolution.x, 0.0); // Horizontal blur
        vec2 offsetCoordV = vec2(0.0, offsets[i] / resolution.y); // Vertical blur

        blurredColor += texture2D(sceneTexture, texCoord + offsetCoordH).rgb * weights[i];
        blurredColor += texture2D(sceneTexture, texCoord + offsetCoordV).rgb * weights[i];
    }

    // 3. Combine blurred bloom with original scene
    vec3 bloom = brightColor * blurredColor * bloomIntensity;
    vec3 finalColor = sceneColor + bloom;

    gl_FragColor = vec4(finalColor, 1.0);
}

