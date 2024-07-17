#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D lightmapTex;
uniform sampler2D tilemapTex;

uniform vec2 windowSize;
uniform vec2 resolution;
uniform vec2 lightmapRes;
uniform vec2 tilemapRes;

#define RES_Y 360

uniform vec2 lValues[RES_Y];
uniform vec2 rValues[RES_Y];

vec2 lerp_vec2(vec2 a, vec2 b, float w) {
    return a + w * (b - a);
}

void main(void)
{

    int yIdx = int(texCoord.y / 2 * RES_Y);

    if (yIdx == RES_Y) {
        fragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    vec2 currentPixelPos = lerp_vec2(lValues[yIdx], rValues[yIdx], texCoord.x);

    if (int(currentPixelPos.x) % 36 == 0 || int(currentPixelPos.y) % 36 == 0) {
        fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    } else {
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    //fragColor.rgb *= abs(texCoord.y - 0.5) * 2;

}