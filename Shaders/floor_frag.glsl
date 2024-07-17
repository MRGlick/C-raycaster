#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D floorTex;
uniform sampler2D tilemapTex;
uniform sampler2D lightmapTex;

uniform vec2 windowSize;
// uniform vec2 tilemapRes;
uniform vec2 lightmapSize;
uniform float pitch;

#define RES_Y 360
#define TILESIZE 34.0
#define FLOOR_TEX_SIZE 36.0
#define BAKED_LIGHT_RESOLUTION 36.0


uniform vec2 lValues[RES_Y];
uniform vec2 rValues[RES_Y];

vec2 lerp_vec2(vec2 a, vec2 b, float w) {
    return a + w * (b - a);
}

float loop_clamp(float val, float _min, float _max) {
    while (val > _max) val -= (_max - _min);
    while (val < _min) val += (_max - _min);

    return val;
}

void main(void)
{

    int yIdx = int(texCoord.y / 2.0 * RES_Y);

    if (yIdx == RES_Y) {
        fragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    vec2 currentPixelPos = lerp_vec2(lValues[yIdx], rValues[yIdx], texCoord.x);

    vec4 texColor = texture(
        floorTex,
        vec2(loop_clamp(currentPixelPos.x / TILESIZE * FLOOR_TEX_SIZE, 0.0, 36.0), loop_clamp(currentPixelPos.y / TILESIZE * FLOOR_TEX_SIZE, 0.0, 36.0)) / 36.0
    );

    fragColor.rgb = texColor.rgb;

    vec2 light_idx = (currentPixelPos / TILESIZE * BAKED_LIGHT_RESOLUTION) / lightmapSize;
    vec3 light_color = texture(lightmapTex, light_idx).rgb * 5;

    fragColor.rgb *= light_color.rgb;

    float thing = min(1, abs(texCoord.y + pitch / windowSize.y - 0.5) * 2 + 0.1);
    fragColor.rgb *= thing;

}