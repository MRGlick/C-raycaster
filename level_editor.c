#include "game_utils.c"
#include "globals.h"
#include "mystring.c"

SDL_Renderer *renderer;
SDL_Window *window;

#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 600
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

enum {
    PAINTMODE_FILL,
    PAINTMODE_DRAW
};

// typedef struct Tile {
//     v2 pos;
//     enum Tiles type;
// } Tile;

v2 get_mouse_pos(); 

void tick(u64 delta);

void render(u64 delta);

void handle_input(SDL_Event event);

void place_object(int row, int col);

void remove_object(int row, int col);

void init();

void test();

void save(SaveData data, char *file);

void load_level(char *file);

SaveData load(char *file);

void mouse_just_pressed(int button);

Placeable get_current_selection();


bool running = true;
int current_selection;
int **wall_tilemap;
int **floor_tilemap;
int **entity_tilemap;
int **ceiling_tilemap;
bool l_mouse_down = false;
bool r_mouse_down = false;

int tile_size = WINDOW_WIDTH / ROOM_WIDTH;
int paint_mode = PAINTMODE_DRAW;
PlaceMode place_mode;
char *level_file;
LevelEditorEntity *player;

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
        level_file = argv[1];
        StringRef str = StringRef(level_file);
        if (!String_equal(String_slice(str, str.len - 8, str.len), StringRef(".hclevel"))) {
            printf("Invalid file format. Only '.hclevel' files are allowed. \n");
        }
    } else {
        printf("No file was entered. Results will not be saved. \n");
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

    SaveData data;
    data.wallTilemap = wall_tilemap;
    data.floorTilemap = floor_tilemap;
    data.ceilingTilemap = ceiling_tilemap;
    data.entityTilemap = entity_tilemap;
    

    save(data, level_file);

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

void init() {
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, wall_tilemap);
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, floor_tilemap);
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, ceiling_tilemap);
    init_grid(int, ROOM_HEIGHT, ROOM_WIDTH, -1, entity_tilemap);

    place_mode = PLACEMODE_CEILING;

    player = malloc(sizeof(LevelEditorEntity));
    player->pos = (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    player->id = ENTITY_PLAYER;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);


    load_level(level_file);

}
// _______
// | | | |

void print_tilemap() {
    for (int i = 0; i < ROOM_HEIGHT; i++) {
        for (int j = 0; j < ROOM_WIDTH; j++) {
            printf("%d ", wall_tilemap[i][j]);
        }
        printf("\n");
    }
}

void remove_player() {
    for (int i = 0; i < ROOM_WIDTH; i++) {
        for (int j = 0; j < ROOM_HEIGHT; j++) {
            if (entity_tilemap[j][i] == (int)P_PLAYER) {
                entity_tilemap[j][i] = -1;
                return;
            }
        }
    }
}

void place_object(int r, int c) {

    if (!in_range(r, 0, ROOM_HEIGHT - 1) || !in_range(c, 0, ROOM_WIDTH - 1)) return;
    


    Placeable s = get_current_selection();
    
    if (place_mode == PLACEMODE_ENTITY && s == entity_tilemap[r][c]) return;
    if (place_mode == PLACEMODE_WALL && s == wall_tilemap[r][c]) return;
    if (place_mode == PLACEMODE_FLOOR && s == floor_tilemap[r][c]) return;
    if (place_mode == PLACEMODE_CEILING && s == ceiling_tilemap[r][c]) return;


    switch (place_mode) {
        case (int)PLACEMODE_ENTITY:
            if (s == P_PLAYER) remove_player();
            entity_tilemap[r][c] = s;
            break;
        case (int)PLACEMODE_FLOOR:
            floor_tilemap[r][c] = s;
            break;
        case (int)PLACEMODE_CEILING:
            ceiling_tilemap[r][c] = s;
            break;
        case (int)PLACEMODE_WALL:
            wall_tilemap[r][c] = s;
            break;
    }

    if (paint_mode == PAINTMODE_FILL) {
        place_object(r - 1, c);
        place_object(r + 1, c);
        place_object(r, c - 1);
        place_object(r, c + 1);
    }

    
}

void remove_object(int row, int col) {
    if (!in_range(row, 0, ROOM_HEIGHT - 1) || !in_range(col, 0, ROOM_WIDTH - 1)) return;



    switch (place_mode) {
        case PLACEMODE_ENTITY:
            if (entity_tilemap[row][col] == -1) return;
            entity_tilemap[row][col] = -1;
            break;
        case PLACEMODE_WALL:
            if (wall_tilemap[row][col] == -1) return;
            wall_tilemap[row][col] = -1;
            break;
        case PLACEMODE_CEILING:
            if (ceiling_tilemap[row][col] == -1) return;
            ceiling_tilemap[row][col] = -1;
            break;
        case PLACEMODE_FLOOR:
            if (floor_tilemap[row][col] == -1) return;
            floor_tilemap[row][col] = -1;
            break;
    }

    if (paint_mode == PAINTMODE_FILL) {
        remove_object(row - 1, col);
        remove_object(row + 1, col);
        remove_object(row, col + 1);
        remove_object(row, col - 1);
    }
    
}


void key_pressed(SDL_Keycode key) {

    if (in_range(key, SDLK_1, SDLK_9)) {
        current_selection = key - SDLK_1;
        return;
    }

    switch (key) {
        case SDLK_f:
            current_selection = 0;
            paint_mode = PAINTMODE_DRAW;
            place_mode = PLACEMODE_FLOOR;
            break;
        case SDLK_c:
            current_selection = 0;
            paint_mode = PAINTMODE_DRAW;
            place_mode = PLACEMODE_CEILING;
            break;
        case SDLK_e:
            current_selection = 0;
            paint_mode = PAINTMODE_DRAW;
            place_mode = PLACEMODE_ENTITY;
            break;
        case SDLK_w:
            current_selection = 0;
            paint_mode = PAINTMODE_DRAW;
            place_mode = PLACEMODE_WALL;
            break;
        case SDLK_F8:
            running = false;
            break;
        case SDLK_b:
            paint_mode = PAINTMODE_DRAW;
            break;
        case SDLK_g:
            paint_mode = PAINTMODE_FILL;
            break;
    }
}

void mouse_down(int button) {
    mouse_just_pressed(button);
    if (button == SDL_BUTTON_LEFT) {
        l_mouse_down = true;
        
    } else if (button == SDL_BUTTON_RIGHT) {
        r_mouse_down = true;
    }
}

void mouse_just_pressed(int button) {
    if (button == SDL_BUTTON_LEFT && place_mode == PLACEMODE_ENTITY) {
        v2 mouse_pos = get_mouse_pos();
        int row = mouse_pos.y / tile_size;
        int col = mouse_pos.x / tile_size;
        place_object(row, col);
    }
}

void mouse_up(int button) {
    if (button == SDL_BUTTON_LEFT) {
        l_mouse_down = false;
    } else if (button == SDL_BUTTON_RIGHT) {
        r_mouse_down = false;
    }
}

void handle_input(SDL_Event event) {
    switch (event.type)
    {
        case SDL_QUIT:
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

Placeable get_current_selection() {
    switch (place_mode) {
        case PLACEMODE_WALL:
            switch(current_selection) {
                case 0:
                    return P_WALL;
                    break;
                default:
                    return P_IDK;
                    break;
            }
            break;
        case PLACEMODE_ENTITY:
            switch(current_selection) {
                case 0:
                    return P_PLAYER;
                    break;
                case 1:
                    return P_SHOOTER;
                    break;
                case 2:
                    return P_EXPLODER;
                    break;
                default:
                    return P_IDK;
                    break;
            }
            break;
        case PLACEMODE_FLOOR:
            switch(current_selection) {
                case 0:
                    return P_FLOOR;
                    break;
                case 1:
                    return P_FLOOR_LIGHT;
                    break;
                default:
                    return P_IDK;
                    break;
            }
            break;
        case PLACEMODE_CEILING:
            switch(current_selection) {
                case 0:
                    return P_CEILING;
                    break;
                case 1:
                    return P_CEILING_LIGHT;
                    break;
                default:
                    return P_IDK;
                    break;
            }
            break;
    }
}

String get_current_selection_string() {

    //     IDK = -1,
    // WALL,
    // DOOR,
    // PLAYER,
    // FLOOR,
    // CEILING

    StringRef res = StringRef("Current: ");

    switch (get_current_selection()) {
        case P_PLAYER:
            return String_concat(res, StringRef("Player"));
            break;
        case P_SHOOTER:
            return String_concat(res, StringRef("Shooter"));
            break;
        case P_WALL:
            return String_concat(res, StringRef("Wall"));
            break;
        case P_FLOOR:
            return String_concat(res, StringRef("Floor"));
            break;
        case P_FLOOR_LIGHT:
            return String_concat(res, StringRef("Floor light"));
            break;
        case P_CEILING:
            return String_concat(res, StringRef("Ceiling"));
            break;
        case P_CEILING_LIGHT:
            return String_concat(res, StringRef("Ceiling light"));
            break;
        case P_EXPLODER:
            return String_concat(res, StringRef("Exploder"));
            break;
        case P_IDK:
            return String_concat(res, StringRef("IDK"));
            break;
    }
}

v2 get_mouse_pos() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    return (v2){x, y};
}

void tick(u64 delta) {
    if (l_mouse_down && place_mode != PLACEMODE_ENTITY) {
        v2 mouse_pos = get_mouse_pos();
        int row = mouse_pos.y / tile_size;
        int col = mouse_pos.x / tile_size;
        place_object(row, col);
    } else if (r_mouse_down) {
        v2 mouse_pos = get_mouse_pos();
        int row = mouse_pos.y / tile_size;
        int col = mouse_pos.x / tile_size;
        remove_object(row, col);
    }

    String title;

    switch (place_mode) {
        case PLACEMODE_FLOOR:
            title = String("Place mode: FLOOR");
            break;
        case PLACEMODE_WALL:
            title = String("Place mode: WALL");
            break;
        case PLACEMODE_ENTITY:
            title = String("Place mode: ENTITY");
            break;
        case PLACEMODE_CEILING:
            title = String("Place mode: CEILING");
            break;
        default:
            title = String("Place mode: IDK");
            break;
    }
    String_append(&title, StringRef(" | "));


    String currentTileText = get_current_selection_string();
    title = String_concatf(title, currentTileText);

    String_append(&title, StringRef(" | "));


    if (paint_mode == PAINTMODE_DRAW) String_append(&title, StringRef("Paint mode: DRAW"));
    else String_append(&title, StringRef("Paint mode: FILL"));

    

    SDL_SetWindowTitle(window, title.data);


    String_delete(&title);
}

void draw_player() {
    SDL_Rect rect = {
        player->pos.x - 10,
        player->pos.y - 10,
        20,
        20
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

    SDL_RenderFillRect(renderer, &rect);
}

void draw_gridlines() {
    switch (place_mode) {
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
        SDL_RenderDrawLine(renderer, i * tile_size, 0, i * tile_size, WINDOW_HEIGHT);
    }
    for (int i = 0; i < ROOM_HEIGHT; i++) {
        SDL_RenderDrawLine(renderer, 0, i * tile_size, WINDOW_WIDTH, i * tile_size);        
    }
}


void set_color_by_type(Placeable type, int opacity) {
    switch (type) {
        case P_WALL:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, opacity);
            break;
        case P_PLAYER:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, opacity);
            break;
        case P_SHOOTER:
            SDL_SetRenderDrawColor(renderer, 20, 120, 20, opacity);
            break;
        case P_FLOOR:
            SDL_SetRenderDrawColor(renderer, 200, 100, 0, opacity);
            break;
        case P_FLOOR_LIGHT:
            SDL_SetRenderDrawColor(renderer, 230, 130, 30, opacity);
            break;
        case P_CEILING:
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, opacity);
            break;
        case P_CEILING_LIGHT:
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, opacity);
            break;
        case P_EXPLODER:
            SDL_SetRenderDrawColor(renderer, 230, 200, 30, opacity);
            break;
    }
} 

void draw() {
    for (int x = 0; x < ROOM_WIDTH; x++) {
        for (int y = 0; y < ROOM_HEIGHT; y++) {
            SDL_Rect rect = {
                x * tile_size,
                y * tile_size,
                tile_size,
                tile_size
            };

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);



            set_color_by_type(floor_tilemap[y][x], place_mode == PLACEMODE_FLOOR? 255 : 30);

            SDL_RenderFillRect(renderer, &rect);

            set_color_by_type(wall_tilemap[y][x], place_mode == PLACEMODE_WALL? 255 : 30);

            SDL_RenderFillRect(renderer, &rect);

            set_color_by_type(entity_tilemap[y][x], place_mode == PLACEMODE_ENTITY? 255 : 30);

            SDL_RenderFillRect(renderer, &rect);

            set_color_by_type(ceiling_tilemap[y][x], place_mode == PLACEMODE_CEILING? 255 : 30);

            SDL_RenderFillRect(renderer, &rect);

            

            
        }
    }

    draw_gridlines();

}

void render(u64 delta) {

    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    

    draw();

    

    SDL_RenderPresent(renderer);
}


void save(SaveData saveData, char *file) {
    FILE *fh = fopen(level_file, "w");
    if (fh == NULL) {
        printf("Couldn't open file for writing. Error code: %d \n", errno);
    }
    
    size_t dataSize = sizeof(int) * ROOM_HEIGHT * ROOM_WIDTH * 4;
    char data[dataSize];
    int idx = 0;

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            data[idx++] = floor_tilemap[r][c];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            data[idx++] = wall_tilemap[r][c];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            data[idx++] = ceiling_tilemap[r][c];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            data[idx++] = entity_tilemap[r][c];
        }
    }


    fwrite(&data, 1, sizeof(data), fh);

    fclose(fh);
}

void load_level(char *file) {
    FILE *fh = fopen(level_file, "r");
    if (fh == NULL) {
        printf("File doesnt exist. ");
        if (strlen(level_file) != 0) {
            printf("Creating new file: '%s' \n", level_file);
        } else {
            printf("\n");
        }
        return;
    } else {
        printf("Loading file: '%s' \n", level_file);
    }

    fseek(fh, 0L, SEEK_END);
    int fileSize = ftell(fh);
    rewind(fh);

    char *data = malloc(sizeof(char) * fileSize); // sizeof char is 1 but i do it for clarity
    fgets(data, fileSize, fh);
    int idx = 0;

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            floor_tilemap[r][c] = data[idx++];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            wall_tilemap[r][c] = data[idx++];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            ceiling_tilemap[r][c] = data[idx++];
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            entity_tilemap[r][c] = data[idx++];
        }
    }


    free(data);

    fclose(fh);
}