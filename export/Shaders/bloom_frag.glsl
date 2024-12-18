#version 330 core

uniform sampler2D u_screenTexture;
uniform vec2 u_resolution;
uniform float u_bloomThreshold = 0.5;
uniform float u_bloomIntensity = 1.0;
uniform float u_bloomSpread = 1.6;

in vec2 texCoord;

out vec4 fragColor;

// Extract bright pixels
vec4 extractBrightPixels(sampler2D tex, vec2 uv) {
    vec4 color = texture(tex, uv);
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    return brightness > u_bloomThreshold ? color : vec4(0.0);
}

// Box blur with downscaling
vec4 downscaledBlur(sampler2D tex, vec2 uv, vec2 resolution, float spread) {
    vec2 pixelSize = 1.0 / resolution;
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    
    int size = 8;

    // Simple box blur with configurable spread
    for (int x = -size; x <= size; x++) {
        for (int y = -size; y <= size; y++) {
            vec2 offset = vec2(x, y) * pixelSize * spread;
            vec2 sampleCoord = uv + offset;
            
            // Ensure we don't sample outside texture
            if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 && 
                sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
                
                vec4 sampleColor = extractBrightPixels(tex, sampleCoord);
                color += sampleColor;
                totalWeight += 1.0;
            }
        }
    }
    
    // Normalize
    return color / max(totalWeight, 1.0);
}

void main() {
    vec2 uv = texCoord;
    
    // Sample original scene
    vec4 originalColor = texture(u_screenTexture, uv);
    
    // Downscaled bloom
    vec2 downscaledResolution = u_resolution * 0.5;
    vec4 bloomColor = downscaledBlur(u_screenTexture, uv, downscaledResolution, u_bloomSpread);
    
    // Combine original color with bloom
    vec4 finalColor = originalColor + bloomColor * u_bloomIntensity;
    
    fragColor = finalColor;
}
