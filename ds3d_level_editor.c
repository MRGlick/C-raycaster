#include "game_utils.c"


SDL_Renderer *renderer;
SDL_Window *window;

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720
#define FPS 60
#define TPS 60
#define ROOM_WIDTH 30
#define ROOM_HEIGHT 20


// enum Tiles {
//     WALL,
//     DOOR,
//     PLAYER,
//     FLOOR
// };

typedef enum Placeables {
    IDK = -1,
    WALL,
    DOOR,
    PLAYER,
    FLOOR,
    CEILING
} Placeable;

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
    int id;
} Entity;

void tick(u64 delta);

void render(u64 delta);

void handle_input(SDL_Event event);

void place(v2 pos);

void remove(v2 pos);

void init();

void test();

void saveLevel();

void loadLevel();

bool running = true;
int currentSelection;
int **wallTileMap;
int **floorTileMap;
int **ceilingTileMap;
bool lMouseDown = false;
bool rMouseDown = false;
const int tileSize = WINDOW_WIDTH / ROOM_WIDTH;
PlaceMode placeMode;
char *levelFile;


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
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, &wallTileMap);
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, &floorTileMap);
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, &ceilingTileMap);

    placeMode = PLACEMODE_CEILING;

    loadLevel();

}
// _______
// | | | |

void printTileMap() {
    for (int i = 0; i < ROOM_HEIGHT; i++) {
        for (int j = 0; j < ROOM_WIDTH; j++) {
            printf("%d ", wallTileMap[i][j]);
        }
        printf("\n");
    }
}

void removePlayer() {
    for (int i = 0; i < ROOM_WIDTH; i++) {
        for (int j = 0; j < ROOM_HEIGHT; j++) {
            if (wallTileMap[j][i] == (int)PLAYER) {
                wallTileMap[j][i] = -1;
                return;
            }
        }
    }
}

void place(v2 pos) {
    v2 tileMapPos = v2_floor(v2_div(pos, to_vec(tileSize)));
    int x = tileMapPos.x;
    int y = tileMapPos.y;
    
    switch (currentSelection) {
        case (int)WALL:
            wallTileMap[y][x] = WALL;
            break;
        case (int)CEILING:
            ceilingTileMap[y][x] = CEILING;
            break;
        case (int)PLAYER:
            removePlayer();
            wallTileMap[y][x] = PLAYER;
    }
}

void remove(v2 pos) {
    v2 tileMapPos = v2_floor(v2_div(pos, to_vec(tileSize)));
    int x = tileMapPos.x;
    int y = tileMapPos.y;

    wallTileMap[y][x] = -1;
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
    if (button == SDL_BUTTON_LEFT) {
        lMouseDown = true;
    } else if (button == SDL_BUTTON_RIGHT) {
        rMouseDown = true;
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
    switch (currentSelection) {
        case 0:
            switch (placeMode) {
                case (int)PLACEMODE_WALL:
                    return WALL;
                    break;
                case (int)PLACEMODE_FLOOR:
                    return FLOOR;
                    break;
                case (int)PLACEMODE_ENTITY:
                    return PLAYER;
                    break;
                case (int)PLACEMODE_CEILING:
                    return CEILING;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
        case 1:
            switch (placeMode) {
                case (int)PLACEMODE_WALL:
                    return DOOR;
                    break;
                default:
                    return IDK;
                    break;
            }
            break;
        default:
            return IDK;
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


void tick(u64 delta) {
    if (lMouseDown) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        place((v2){x, y});
    } else if (rMouseDown) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        remove((v2){x, y});
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

    for (int i = 0; i < ROOM_WIDTH; i++) {
        SDL_RenderDrawLine(renderer, i * tileSize, 0, i * tileSize, WINDOW_HEIGHT);
    }
    for (int i = 0; i < ROOM_HEIGHT; i++) {
        SDL_RenderDrawLine(renderer, 0, i * tileSize, WINDOW_WIDTH, i * tileSize);        
    }
}


void setColorByType(Placeable type) {
    switch (type) {
        case WALL:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
        case PLAYER:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case DOOR:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            break;
    }
} 

void drawPlacedTiles() {
    for (int i = 0; i < ROOM_WIDTH; i++) {
        for (int j = 0; j < ROOM_HEIGHT; j++) {
            SDL_Rect rect = {
                i * tileSize,
                j * tileSize,
                tileSize,
                tileSize
            };

            switch (wallTileMap[j][i]) {
                case PLAYER:
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    break;
                case WALL:
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                    break;
                case DOOR:
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    break;
                default:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                    break;
            }

            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void render(u64 delta) {

    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    

    drawPlacedTiles();

    drawGridLines();

    SDL_RenderPresent(renderer);
}

void saveLevel() {
    FILE *fh = fopen(levelFile, "w");
    if (fh == NULL) {
        printf("Failed to open file for writing. %d\n", errno);
        return;
    }

    char *str = malloc(sizeof(char) * (ROOM_HEIGHT * ROOM_WIDTH + 1));
    if (str == NULL) {
        printf("malloc failed.\n");
        fclose(fh); // Close the file stream before returning
        return;
    }

    int strIdx = 0;
    for (int i = 0; i < ROOM_HEIGHT; i++) {
        for (int j = 0; j < ROOM_WIDTH; j++) {
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

    char *str = malloc(ROOM_HEIGHT * ROOM_WIDTH + 1);
    fgets(str, ROOM_WIDTH * ROOM_HEIGHT, fh);

    for (int row = 0; row < ROOM_HEIGHT; row++) {
        for (int col = 0; col < ROOM_WIDTH; col++) {
            char tile = str[row * ROOM_WIDTH + col];
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