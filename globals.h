

#define TILEMAP_WIDTH 30
#define TILEMAP_HEIGHT 20

typedef enum Placeable {
    IDK = -1,
    WALL,
    DOOR,
    PLAYER,
    FLOOR,
    CEILING,
    SHOOTER
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

