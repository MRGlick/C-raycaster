

// #define TILEMAP_WIDTH 60
// #define TILEMAP_HEIGHT 40

#define ROOM_WIDTH 30
#define ROOM_HEIGHT 20

#define DUNGEON_SIZE 4

#define TILEMAP_WIDTH 120
#define TILEMAP_HEIGHT 80

typedef enum Placeable {
    P_IDK = -1,
    P_WALL_START,
        P_WALL,
        P_DOOR,
    P_WALL_END,
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

typedef enum SaveType {
    ST_LEVEL,
    ST_ROOM
} SaveType;

typedef struct SaveData {
    int **wallTilemap;
    int **floorTilemap;
    int **ceilingTilemap;
    int **entityTilemap;
    SaveType type;
} SaveData;

