

#define TILEMAP_WIDTH 30
#define TILEMAP_HEIGHT 20

typedef enum Placeables {
    IDK = -1,
    WALL,
    DOOR,
    PLAYER,
    FLOOR,
    CEILING,
    SHOOTER
} Placeable;

typedef enum EntityID {
    ENTITY_PLAYER = 0,
    ENTITY_SHOOTER = 1
} EntityID;