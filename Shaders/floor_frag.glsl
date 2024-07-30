#version 330 core

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

#define RES_Y 360
#define TILESIZE 34.0
#define FLOOR_TEX_SIZE 36.0
#define BAKED_LIGHT_RESOLUTION 36.0
#define SPRITESHEET_SIZE vec2(10.0, 2.0)
#define TILEMAP_WIDTH 60
#define TILEMAP_HEIGHT 40

uniform sampler2D floorTex;
uniform sampler2D lightmapTex;
uniform sampler2D spritesheet;
uniform sampler2D tilemapTex;

uniform vec2 offsets;
uniform vec2 windowSize;
uniform vec2 tilemapSize;
uniform vec2 lightmapSize;
uniform float pitch;

uniform vec2 lValues[RES_Y];
uniform vec2 rValues[RES_Y];

uniform float texIds[20];

vec2 lerp_vec2(vec2 a, vec2 b, float w) {
    return a + w * (b - a);
}

float loop_clamp(float val, float _min, float _max) {
    while (val > _max) val -= (_max - _min);
    while (val < _min) val += (_max - _min);

    return val;
}

vec3 get_tile(vec2 pos) {
    return texture(tilemapTex, pos / tilemapSize).rgb;
}

void main(void) {

    int yIdx = int(texCoord.y * RES_Y / 2.0);

    if (yIdx == RES_Y) {
        fragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    vec2 currentPixelPos = lerp_vec2(lValues[yIdx], rValues[yIdx], texCoord.x);

    vec2 texturePos = vec2(0.0, 0.0);
    
    bool isCeiling = texCoord.y + pitch / windowSize.y < 0.5;

    vec2 tilemapPos = currentPixelPos / TILESIZE;
    vec3 tileFloorAndCeiling = get_tile(tilemapPos);

    float ceilingTile = round(tileFloorAndCeiling.r * 25.5);
    float floorTile = round(tileFloorAndCeiling.b * 25.5);

    if (isCeiling) {
        if (ceilingTile == texIds[1]) {
            texturePos = vec2(0.0, 1.0);
        } else if (ceilingTile == texIds[3]) {
            texturePos = vec2(1.0, 1.0);
        }
    } else {
        if (floorTile == texIds[0]) {
            texturePos = vec2(0.0, 0.0);
        } else if (floorTile == texIds[2]) {
            texturePos = vec2(1.0, 0.0);
        }
    }

    vec2 coord = vec2(loop_clamp(currentPixelPos.x / TILESIZE * FLOOR_TEX_SIZE, 0.0, 36.0), loop_clamp(currentPixelPos.y / TILESIZE * FLOOR_TEX_SIZE, 0.0, 36.0)) / 36.0;

    vec4 texColor = texture(
        spritesheet,
        floor(texturePos) / SPRITESHEET_SIZE + coord / SPRITESHEET_SIZE
    );

    fragColor.a = 1.0; // wtf?
    fragColor.rgb = texColor.rgb;

    vec2 light_idx = (currentPixelPos / TILESIZE * BAKED_LIGHT_RESOLUTION) / lightmapSize;
    vec3 light_color = texture(lightmapTex, light_idx).rgb * 5.0;

    float thing = min(1, abs(texCoord.y + pitch / windowSize.y - 0.5) * 2 + 0.1);
    
    fragColor.rgb *= thing;

    fragColor.rgb = fragColor.rgb * light_color.rgb;

}