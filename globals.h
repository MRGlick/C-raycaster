

#define TILEMAP_WIDTH 30
#define TILEMAP_HEIGHT 20

typedef enum Placeable {
    P_IDK = -1,
    P_WALL = 0,
    P_DOOR = 1,
    P_PLAYER = 2,
    P_FLOOR = 3,
    P_CEILING = 4,
    P_SHOOTER = 5
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

