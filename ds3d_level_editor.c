#include "game_utils.c"
#include "globals.h"

SDL_Renderer *renderer;
SDL_Window *window;

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720
#define FPS 60
#define TPS 60



// enum Tiles {
//     WALL,
//     DOOR,
//     PLAYER,
//     FLOOR
// };

typedef enum PlaceMode {
    PLACEMODE_FLOOR,
    PLACEMODE_WALL,
    PLACEMODE_CEILING,
    PLACEMODE_ENTITY
} PlaceMode;

// typedef struct Tile {
//     v2 pos;
//     enum Tiles type;
// } Tile;

typedef struct Entity {
    v2 pos;
    EntityID id;
} Entity;

v2 getMousePos(); 

void tick(u64 delta);

void render(u64 delta);

void handle_input(SDL_Event event);

void placeObject(v2 pos);

void removeObject(v2 pos);

void init();

void test();

void saveLevel();

void loadLevel();

void mouse_just_pressed(int button);

Placeable getCurrentSelection();

bool running = true;
int currentSelection;
int **wallTileMap;
int **floorTileMap;
int **ceilingTileMap;
bool lMouseDown = false;
bool rMouseDown = false;
const int tileSize = WINDOW_WIDTH / TILEMAP_WIDTH;
PlaceMode placeMode;
char *levelFile;
arraylist *entityList;
Entity *player;

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Shit \n");
    }

    window = SDL_CreateWindow(
        "Level editor!",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0
    );

    renderer = SDL_CreateRenderer(
        window,
        -1,
        RENDERER_FLAGS
    );

    if (argc >= 2) {
        levelFile = argv[1];
    }
    init();

    u64 tick_timer = 0, render_timer = 0;
    u64 last_time = SDL_GetTicks64();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_input(event);
        }

        u64 now = SDL_GetTicks64();
        u64 delta = now - last_time;
        last_time = now;
        tick_timer += delta;
        render_timer += delta;
        if (mili_to_sec(tick_timer) >= 1.0/TPS) {
            tick(delta);
            tick_timer = 0;
        }
        if (mili_to_sec(render_timer) >= 1.0/FPS) {
            render(delta);
            render_timer = 0;
        }
    }

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

void test() {
    saveLevel();
}

void init() {
    init_grid(int, TILEMAP_HEIGHT, TILEMAP_WIDTH, -1, &wallTileMap);
    init_grid(int, TILEMAP_HEIGHT, TILEMAP_WIDTH, -1, &floorTileMap);
    init_grid(int, TILEMAP_HEIGHT, TILEMAP_WIDTH, -1, &ceilingTileMap);
    entityList = create_arraylist(5);

    placeMode = PLACEMODE_CEILING;

    player = malloc(sizeof(Entity));
    player->pos = (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    player->id = ENTITY_PLAYER;

    loadLevel();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

}
// _______
// | | | |

void printTileMap() {
    for (int i = 0; i < TILEMAP_HEIGHT; i++) {
        for (int j = 0; j < TILEMAP_WIDTH; j++) {
            printf("%d ", wallTileMap[i][j]);
        }
        printf("\n");
    }
}

void removePlayer() {
    for (int i = 0; i < TILEMAP_WIDTH; i++) {
        for (int j = 0; j < TILEMAP_HEIGHT; j++) {
            if (wallTileMap[j][i] == (int)PLAYER) {
                wallTileMap[j][i] = -1;
                return;
            }
        }
    }
}

void placeObject(v2 pos) {
    v2 tileMapPos = v2_floor(v2_div(pos, to_vec(tileSize)));
    int x = tileMapPos.x;
    int y = tileMapPos.y;

    switch (getCurrentSelection()) {
        case (int)WALL:
            wallTileMap[y][x] = WALL;
            break;
        case (int)CEILING:
            ceilingTileMap[y][x] = CEILING;
            break;
        case (int)FLOOR:
            floorTileMap[y][x] = FLOOR;
            break;
        case (int)PLAYER:
            player->pos = pos;
            break;
        case (int)SHOOTER: ;
            Entity *shooter = malloc(sizeof(Entity));
            shooter->id = ENTITY_SHOOTER;
            shooter->pos = pos;
            arraylist_add(entityList, shooter, -1);
            break;
    }
}

void removeEntitiesAt(v2 pos, double radius) {
    arraylist *entitiesToRemove = create_arraylist(10);
    for (int i = 0; i < entityList->length; i++) {
        Entity *entity = (Entity *)arraylist_get(entityList, i)->val;
        if (v2_distance_squared(pos, entity->pos) < radius * radius) {
            arraylist_add(entitiesToRemove, entity, -1);
        }
    }

    for (int i = 0; i < entitiesToRemove->length; i++) {
        Entity *entity = arraylist_get(entitiesToRemove, i)->val;
        free(entity);
        arraylist_remove(entityList, arraylist_find(entityList, entity));
    }
}

void removeObject(v2 pos) {
    v2 tileMapPos = v2_floor(v2_div(pos, to_vec(tileSize)));
    int x = tileMapPos.x;
    int y = tileMapPos.y;

    switch (placeMode) {
        case PLACEMODE_ENTITY:
            removeEntitiesAt(pos, 10);
            break;
        case PLACEMODE_WALL:
            wallTileMap[y][x] = -1;
            break;
        case PLACEMODE_CEILING:
            ceilingTileMap[y][x] = -1;
            break;
        case PLACEMODE_FLOOR:
            floorTileMap[y][x] = -1;
            break;
    }
    
}


void key_pressed(SDL_Keycode key) {
    switch (key) {
        case SDLK_1:
            currentSelection = 0;
            break;
        case SDLK_2:
            currentSelection = 1;
            break;
        case SDLK_3:
            currentSelection = 2;
            break;
        case SDLK_f:
            placeMode = PLACEMODE_FLOOR;
            break;
        case SDLK_c:
            placeMode = PLACEMODE_CEILING;
            break;
        case SDLK_e:
            placeMode = PLACEMODE_ENTITY;
            break;
        case SDLK_w:
            placeMode = PLACEMODE_WALL;
            break;
    }
}

void mouse_down(int button) {
    mouse_just_pressed(button);
    if (button == SDL_BUTTON_LEFT) {
        lMouseDown = true;
        
    } else if (button == SDL_BUTTON_RIGHT) {
        rMouseDown = true;
    }
}

void mouse_just_pressed(int button) {
    if (button == SDL_BUTTON_LEFT && placeMode == PLACEMODE_ENTITY) {
        placeObject(getMousePos());
    }
}

void mouse_up(int button) {
    if (button == SDL_BUTTON_LEFT) {
        lMouseDown = false;
    } else if (button == SDL_BUTTON_RIGHT) {
        rMouseDown = false;
    }
}

void handle_input(SDL_Event event) {
    switch (event.type)
    {
        case SDL_QUIT:
            saveLevel();
            running = false;
            break;
        case SDL_KEYDOWN:
            key_pressed(event.key.keysym.sym);
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouse_down(event.button.button);
            break;
        case SDL_MOUSEBUTTONUP:
            mouse_up(event.button.button);
            break;
    }

}

Placeable getCurrentSelection() {
    switch (placeMode) {
        case PLACEMODE_WALL:
            switch(currentSelection) {
                case 0:
                    return WALL;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
        case PLACEMODE_ENTITY:
            switch(currentSelection) {
                case 0:
                    return PLAYER;
                    break;
                case 1:
                    return SHOOTER;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
        case PLACEMODE_FLOOR:
            switch(currentSelection) {
                case 0:
                    return FLOOR;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
        case PLACEMODE_CEILING:
            switch(currentSelection) {
                case 0:
                    return CEILING;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
    }
}

char *getCurrentSelectionString() {

    //     IDK = -1,
    // WALL,
    // DOOR,
    // PLAYER,
    // FLOOR,
    // CEILING

    switch (getCurrentSelection()) {
        case PLAYER:
            return "Current selection: Player";
            break;
        case SHOOTER:
            return "Current selection: Shooter";
            break;
        case WALL:
            return "Current selection: wall";
            break;
        case DOOR:
            return "Current selection: Door";
            break;
        case FLOOR:
            return "Current selection: Floor";
            break;
        case CEILING:
            return "Current selection: Ceiling";
            break;
        case IDK:
            return "Current selection: IDK";
            break;
    }
}

v2 getMousePos() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return (v2){x, y};
}

void tick(u64 delta) {
    if (lMouseDown && placeMode != PLACEMODE_ENTITY) {
        placeObject(getMousePos());
    } else if (rMouseDown) {
        removeObject(getMousePos());
    }

    char *title;
    char *placemode;

    switch (placeMode) {
        case PLACEMODE_FLOOR:
            placemode = "Place mode: FLOOR";
            break;
        case PLACEMODE_WALL:
            placemode = "Place mode: WALL";
            break;
        case PLACEMODE_ENTITY:
            placemode = "Place mode: ENTITY";
            break;
        case PLACEMODE_CEILING:
            placemode = "Place mode: CEILING";
            break;
        default:
            placemode = "Place mode: unknown";
            break;
    }

    char *currentTileText = getCurrentSelectionString();
    title = concat(concat(currentTileText, "  "), placemode);

    SDL_SetWindowTitle(window, title);
}

void drawPlayer() {
    SDL_Rect rect = {
        player->pos.x - 10,
        player->pos.y - 10,
        20,
        20
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    SDL_RenderFillRect(renderer, &rect);
}

void drawEntity(v2 pos, EntityID id) {
    SDL_Rect rect = {
        pos.x - 10,
        pos.y - 10,
        20,
        20
    };

    switch (id) {
        case ENTITY_SHOOTER:
            SDL_SetRenderDrawColor(renderer, 10, 80, 10, 255);
            break;
        default:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
    }

    SDL_RenderFillRect(renderer, &rect);
}

void drawGridLines() {
    switch (placeMode) {
        case (int)PLACEMODE_FLOOR:
            SDL_SetRenderDrawColor(renderer, 200, 100, 0, 255);
            break;
        case (int)PLACEMODE_WALL:
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            break;
        case (int)PLACEMODE_ENTITY:
            SDL_SetRenderDrawColor(renderer, 50, 255, 100, 255);
            break;
        case (int)PLACEMODE_CEILING:
            SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
            break;
    }

    for (int i = 0; i < TILEMAP_WIDTH; i++) {
        SDL_RenderDrawLine(renderer, i * tileSize, 0, i * tileSize, WINDOW_HEIGHT);
    }
    for (int i = 0; i < TILEMAP_HEIGHT; i++) {
        SDL_RenderDrawLine(renderer, 0, i * tileSize, WINDOW_WIDTH, i * tileSize);        
    }
}


void setColorByType(Placeable type) {
    int opacity = 125;
    switch (type) {
        case WALL:
            if (placeMode == PLACEMODE_WALL) {
                opacity = 255;
            }
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, opacity);
            
            break;
        case PLAYER:
            if (placeMode == PLACEMODE_ENTITY) {
                opacity = 255;
            }
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, opacity);
            break;
        case FLOOR:
            if (placeMode == PLACEMODE_FLOOR) {
                opacity = 255;
            }
            SDL_SetRenderDrawColor(renderer, 200, 100, 0, opacity);
            break;
        case CEILING:
            if (placeMode == PLACEMODE_CEILING) {
                opacity = 255;
            }
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, opacity);
            break;
        case DOOR:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            break;
    }
} 

void draw() {
    for (int x = 0; x < TILEMAP_WIDTH; x++) {
        for (int y = 0; y < TILEMAP_HEIGHT; y++) {
            SDL_Rect rect = {
                x * tileSize,
                y * tileSize,
                tileSize,
                tileSize
            };

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);



            setColorByType(floorTileMap[y][x]);

            SDL_RenderFillRect(renderer, &rect);

            setColorByType(wallTileMap[y][x]);

            SDL_RenderFillRect(renderer, &rect);

            setColorByType(ceilingTileMap[y][x]);

            SDL_RenderFillRect(renderer, &rect);

            

            
        }
    }

    if (placeMode == PLACEMODE_ENTITY) {
        for (int i = 0; i < entityList->length; i++) {
            Entity *entity = arraylist_get(entityList, i)->val;
            drawEntity(entity->pos, entity->id);
        }
    }

    drawPlayer();

    drawGridLines();

}

void render(u64 delta) {

    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    

    draw();

    

    SDL_RenderPresent(renderer);
}

void saveLevel() {
    FILE *fh = fopen(levelFile, "w");
    if (fh == NULL) {
        printf("Failed to open file for writing. %d\n", errno);
        return;
    }

    char *str = malloc(sizeof(char) * (TILEMAP_HEIGHT * TILEMAP_WIDTH + 1));
    if (str == NULL) {
        printf("malloc failed.\n");
        fclose(fh); // Close the file stream before returning
        return;
    }

    int strIdx = 0;
    for (int i = 0; i < TILEMAP_HEIGHT; i++) {
        for (int j = 0; j < TILEMAP_WIDTH; j++) {
            if (wallTileMap[i][j] == -1) {
                str[strIdx++] = '-';
            } else {
                str[strIdx++] = '0' + wallTileMap[i][j];
            }
        }
    }


    str[strIdx] = '\0';

    fputs(str, fh);

    fclose(fh); // Close the file stream
    free(str); // Free the allocated memory
}

void loadLevel() {
    FILE *fh = fopen(levelFile, "r");

    if (fh == NULL) {
        return;
    }

    char *str = malloc(TILEMAP_HEIGHT * TILEMAP_WIDTH + 1);
    fgets(str, TILEMAP_WIDTH * TILEMAP_HEIGHT, fh);

    for (int row = 0; row < TILEMAP_HEIGHT; row++) {
        for (int col = 0; col < TILEMAP_WIDTH; col++) {
            char tile = str[row * TILEMAP_WIDTH + col];
            if (tile == '-') {
                wallTileMap[row][col] = -1;
            } else {
                wallTileMap[row][col] = tile - '0';
            }
        }
    }

    free(str);
    fclose(fh);
}