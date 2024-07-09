

#define TILEMAP_WIDTH 60
#define TILEMAP_HEIGHT 40

typedef enum Placeable {
    P_IDK = -1,
    P_WALL,
    P_DOOR,
    P_PLAYER,
    P_FLOOR,
    P_FLOOR_LIGHT,
    P_CEILING,
    P_CEILING_LIGHT,
    P_SHOOTER,
    P_EXPLODER
} Placeable;

typedef struct LevelEditorEntity {
    v2 pos;
    char id;
} LevelEditorEntity;

typedef enum EntityID {
    ENTITY_PLAYER = 0,
    ENTITY_SHOOTER = 1
} EntityID;


typedef struct SaveData {
    int **wallTilemap;
    int **floorTilemap;
    int **ceilingTilemap;
    int **entityTilemap;
} SaveData;

