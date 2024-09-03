
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include "game_utils.c"
#include "globals.h"
#include "ui.c"
#include "mystring.c"
#include "multiplayer.c"


// #DEFINITIONS

#define DEBUG_FLAG false

#define SERVER_IP "84.95.65.201"
#define SERVER_PORT 1155
#define TPS 300
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 580
#define RESOLUTION_X 720
#define RESOLUTION_Y 360
#define X_SENSITIVITY .1
#define Y_SENSITIVITY .8
#define COLOR_BLACK \
    (SDL_Color) { 0, 0, 0 }
#define RENDER_DISTANCE 350
#define WALL_HEIGHT 30
#define WALL_HEIGHT_MULTIPLIER 2
#define NUM_WALL_THREADS 1
#define NUM_FLOOR_THREADS 2
#define MAX_LIGHT 9
#define BAKED_LIGHT_RESOLUTION 36
#define BAKED_LIGHT_CALC_RESOLUTION 8
#define CLIENT_UPDATE_RATE 20
#define PLAYER_COLLIDER_RADIUS 10


#define PARTICLE_GRAVITY -50000
#define PROJECTILE_GRAVITY -2000

#define OUT_OF_SCREEN_POS \
    (v2) { WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100 }


#define get_window() SDL_GetWindowFromID(actual_screen->context->windowID)


// # PACKET TYPES AND STRUCTS
enum PacketTypes {
    PACKET_UPDATE_PLAYER_ID,
    PACKET_PLAYER_POS,
    PACKET_PLAYER_JOINED,
    PACKET_DUNGEON_SEED,
    PACKET_REQUEST_DUNGEON_SEED,
    PACKET_ABILITY_SHOOT,
    PACKET_HOST_LEFT,
    PACKET_PLAYER_LEFT,
    PACKET_ABILITY_BOMB
};

struct ability_bomb_packet {
    v2 pos;
    double height;
    v2 vel;
    double height_vel;
    int sender_id;
};

struct player_left_packet {
    int id;
};

struct ability_shoot_packet {
    int shooter_id;
    int hit_id;
    v2 hit_pos;
    double hit_height;
};

struct dungeon_seed_packet {
    long seed;
};

struct player_pos_packet {
    v2 pos; // 16
    double height; // 8
    v2 dir;
    int id; // 4
    SDL_Color color;
    // 4 padding
};

struct player_joined_packet {
    int id;
    // ... more stuff later, maybe?
};


// #TYPES

enum Types {
    PLAYER,
    RAYCAST,
    CIRCLE_COLLIDER,
    RAY_COLL_DATA,
    RENDER_OBJECT,
    WALL_STRIPE,
    LIGHT_POINT,
    SPRITE,
    PARTICLE_SPAWNER,
    
    DIR_SPRITE,
    
    ENTITY_START,
    
        ENTITY,

        BULLET,

        PLAYER_ENTITY,

        PROJECTILE_START,
            PROJECTILE,
        PROJECTILE_END,

        EFFECT_START,

            EFFECT,
            PARTICLE,

        EFFECT_END,
        
        ENEMY_START,
            
            ENEMY,
            ENEMY_SHOOTER,
            ENEMY_EXPLODER,
            
        ENEMY_END,

    ENTITY_END
};

enum Tiles { WALL1 = 1, WALL2 = 2 };

typedef enum AbilityType {
    A_PRIMARY,
    A_SECONDARY,
    A_UTILITY,
    A_SPECIAL
} AbilityType;

typedef enum {
    STATE_IDLE,
    STATE_PURSUING
} EnemyState;

// #STRUCTS

typedef struct Animation {
    GPU_Image **frames;
    int frameCount;
    int frame;
    double fps;
    double timeToNextFrame;
    bool loop;
    bool playing;
    int priority;
} Animation;

typedef struct Sprite {
    GPU_Image *texture;  // not used if animated
    bool isAnimated;
    int currentAnimationIdx;
    Animation *animations;
    int animCount;
} Sprite;

typedef struct DirectionalSprite {
    Sprite **sprites;
    v2 dir;  // global direction
    int dirCount;

    int current_anim;
    int playing;
    int fps;
} DirectionalSprite;

typedef struct Raycast {
    v2 pos, dir;
} Raycast;

typedef struct RayCollisionData {
    v2 startpos, collpos, normal;
    void *collider;
    int colliderType;
    GPU_Image *colliderTexture;
    bool hit;
    double collIdx, wallWidth;
} RayCollisionData;

typedef struct CircleCollider {
    v2 pos;
    double radius;
} CircleCollider;

typedef struct Ability {
    void (*activate)(struct Ability *);
    void (*tick)(struct Ability *, double);
    void (*before_activate)(struct Ability *);
    bool can_use;
    double cooldown;
    double timer;
    double delay;
    double delay_timer;
    AbilityType type;

    GPU_Image *texture;

} Ability;

typedef struct Player {
    v2 pos, vel;
    double speed, angle, torque, collSize;
    double pitch_angle;
    double height;
    double height_vel;
    double pitch;
    bool sprinting;
    bool crouching;
    bool canShoot;
    double shootCooldown;
    double shootChargeTimer;
    int pendingShots;
    int max_pending_shots;
    double ShootTickTimer;
    v2 handOffset;
    double health, maxHealth;
    CircleCollider *collider;
    double tallness;

    Ability *primary, *secondary, *utility, *special;
} Player;

typedef struct Entity {
    v2 pos, size;
    Sprite *sprite;
    double height;
    bool affected_by_light;
    SDL_Color color;
} Entity;

typedef struct Projectile {
    Entity entity;
    v2 vel;
    double height_vel;
    v2 accel;
    CircleCollider collider;
    double height_accel;
    double life_time, life_timer;
    double bounciness;
    bool destroy_on_floor;
    bool _created;
    void (*on_create)(struct Projectile *);
    void (*on_tick)(struct Projectile *, double);
    void (*on_destruction)(struct Projectile *);

} Projectile;

typedef struct PlayerEntity {
    Entity entity;
    v2 desired_pos;
    double desired_height;
    CircleCollider collider;
    v2 dir;
    int id;
    Entity *direction_indicator;
} PlayerEntity;

typedef struct LightPoint {
    v2 pos;
    double strength;
    double radius;
    SDL_Color color;
} LightPoint;

typedef struct Effect {
    Entity entity;
    double life_time;
    double life_timer;
} Effect;

typedef struct Particle {
    Effect effect;
    v2 vel;
    double h_vel;
    v2 accel;
    double h_accel;
    double bounciness;
    double floor_drag;
    v2 initial_size;
    bool fade;
    bool fade_scale;
    SDL_Color start_color, end_color;
} Particle;

typedef struct ParticleSpawner {
    v2 pos;
    double height;

    v2 dir;
    double height_dir;

    double spread;

    v2 accel;
    double height_accel;

    double min_speed, max_speed;

    v2 min_size, max_size;

    int spawn_rate; // per second

    double spawn_timer;

    double bounciness;

    double floor_drag;

    Sprite sprite;

    double particle_lifetime;

    bool active;

    Entity *target;

    bool fade;

    bool fade_scale;

    bool affected_by_light;

    int explode_particle_amount;

    SDL_Color start_color, end_color;

} ParticleSpawner;


typedef struct RenderObject {
    void *val;
    double dist_squared;
    int type;
    bool isnull;
} RenderObject;

typedef struct Bullet {
    Entity entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
    ParticleSpawner *particle_spawner;
    v2 dir;
    double dmg;
    double speed;
    double lifeTime;
    double lifeTimer;
    bool hit_enemies;
    bool hit_player;

    void (*on_hit)(struct Bullet *);
    

} Bullet;

typedef struct BakedLightColor {
    float r, g, b;
} BakedLightColor;


typedef struct CollisionData {
    v2 offset;  // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;

typedef struct Room {
    v2 room_idx; // 0, 0 -> 3, 3
    String room_file_name;
    bool is_start, is_boss;

    struct Room *left, *right, *up, *down;
    v2 left_entrance_pos, right_entrance_pos, top_entrance_pos, bottom_entrance_pos;

    bool initialized;
} Room;

// #FUNC

void randomize_player_abilities();

void bomb_on_destroy(Projectile *projectile);

Projectile create_bomb_projectile(v2 pos, v2 start_vel);

Projectile create_default_projectile(double life_time);

void projectile_destroy(Projectile *projectile);

void projectile_tick(Projectile *projectile, double delta);

bool is_projectile_type(int type);

void ability_bomb_activate();

Ability ability_bomb_create();

void send_dmg_packet(int id, double dmg);

void player_entity_take_dmg(PlayerEntity *player_entity, double dmg);

PlayerEntity *find_or_add_player_entity_by_id(int id);

PlayerEntity *find_player_entity_by_id(int id);

void init_textures();

void player_entity_tick(PlayerEntity *player_entity, double delta);

void init_player(v2 pos);

void on_client_recv(MPPacket packet, void *data);

void on_server_recv(SOCKET socket, MPPacket packet, void *data);

void on_player_connect(SOCKET player_socket);

void on_player_disconnect(SOCKET player_socket);

void enemy_bullet_destroy(Bullet *bullet);

void write_to_debug_label(String string);

bool is_player_on_floor();

double set_time_scale_for_duration(double time_scale, double duration);

double get_player_height();

void load_dungeon();

Room Room_new(v2 pos);

void generate_dungeon();

void particle_spawner_explode(ParticleSpawner *spawner);

void toggle_pause();

void make_ui();

void _player_die();

void ability_dash_before_activate(Ability *ability);

void activate_ability(Ability *ability);

void ability_dash_activate(Ability *ability);

void ability_dash_tick(Ability *ability, double delta);

void shooter_bullet_effect(Bullet *bullet);

void place_entity(v2 pos, int type);

void particle_tick(Particle *particle, double delta);

void particle_spawner_spawn(ParticleSpawner *spawner);

void particle_spawner_tick(ParticleSpawner *spawner, double delta);

ParticleSpawner create_particle_spawner(v2 pos, double height);

void draw_3d_line(v2 pos1, double h1, v2 pos2, double h2);

void _shoot(double spread);

void ability_secondary_shoot_activate(Ability *ability);

Ability ability_dash_create();

Ability ability_secondary_shoot_create();

Ability ability_primary_shoot_create();

void default_ability_tick(Ability *ability, double delta);


void dir_sprite_play_anim(DirectionalSprite *dir_sprite, int anim);


bool is_entity_type(int type);

int charge_time_to_shots(double charge_time);

SDL_Color lerp_color(SDL_Color col1, SDL_Color col2, double w);

void remove_loading_screen();

void update_loading_progress(double progress);

void init_loading_screen();

void player_die();

void player_take_dmg(double dmg);

void reset_level();

double get_max_height();

v2 worldToScreen(v2 pos, double height, bool allow_out_of_screen);

void clampColors(int rgb[3]);

BakedLightColor get_light_color_by_pos(v2 pos, int row_offset, int col_offset);

void bake_lights();

void update_fullscreen();

void drawSkybox();

void update_entity_collisions(void *val, int type);

void init_tilemap(int ***gridPtr, int cols, int rows);

void reset_tilemap(int ***gridPtr, int cols, int rows);

void load_level(char *file);

void init();

void render(double delta);

RayCollisionData ray_circle(Raycast ray, CircleCollider circle);

RayCollisionData ray_object(Raycast ray, obj *object);

RayCollisionData castRayForEntities(v2 pos, v2 dir);

RayCollisionData castRay(v2 pos, v2 dir);

RayCollisionData castRayForAll(v2 pos, v2 dir);

CollisionData getCircleTileCollision(CircleCollider circle, v2 tilePos);

CollisionData getCircleTileMapCollision(CircleCollider circle);

void key_pressed(SDL_Keycode key);

void key_released(SDL_Keycode key);

void tick(double delta);

void handle_input(SDL_Event event);

void add_game_object(void *val, int type);

void remove_game_object(void *val, int type);

double mili_to_sec(u64 mili);

v2 get_player_forward();

Animation create_animation(int frameCount, int priority, GPU_Image **frames);

void freeAnimation(Animation *anim);

GPU_Image *getSpriteCurrentTexture(Sprite *sprite);

Sprite *dir_sprite_current_sprite(DirectionalSprite *dSprite, v2 spritePos);

void objectTick(void *obj, int type, double delta);

void playerTick(double delta);

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, double delta);

void spriteTick(Sprite *sprite, double delta);

void animationTick(Animation *anim, double delta);

void effectTick(Effect *effect, double delta);

void bulletTick(Bullet *bullet, double delta);

Sprite *createSprite(bool isAnimated, int animCount);

DirectionalSprite *createDirSprite(int dirCount);

void spritePlayAnim(Sprite *sprite, int idx);

void ability_shoot_activate(Ability *ability);

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light, SDL_Color custom_color);

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height);

double angleDist(double a1, double a2);

void shakeCamera(double strength, int ticks, bool fade, int priority);



bool isValidLevel(char *file);

void getTextureFiles(char *fileName, int fileCount, GPU_Image ***textures);

// #FUNC END

GPU_Target *screen;
GPU_Image *screen_image;
GPU_Target *hud;
GPU_Image *hud_image;
GPU_Target *actual_screen;

// #UI
UIComponent *pause_menu;
UILabel *debug_label;

// #TEXTURES
GPU_Image *blood_particle;
GPU_Image **dash_screen_anim;
GPU_Image *lightmap_image;
GPU_Image *tilemap_image;
GPU_Image *floor_and_ceiling_spritesheet;
GPU_Image *dash_icon;
GPU_Image *ability_icon_frame;
GPU_Image *shoot_icon;
GPU_Image *shotgun_icon;
GPU_Image *exploder_hit;
GPU_Image **exploder_explosion_texture;
GPU_Image **shooter_dirs_textures;
GPU_Image *default_particle_texture;
GPU_Image *mimran_jumpscare;
GPU_Image *shooter_hit_texture;
GPU_Image *healthbar_texture;
GPU_Image *vignette_texture;
GPU_Image *floorAndCeiling;
GPU_Image *floor_and_ceiling_target_image;
GPU_Image *wallTexture;
GPU_Image *entityTexture;
GPU_Image *crosshair;
GPU_Image *fenceTexture;
GPU_Image *skybox_texture;
GPU_Image **wallFrames;
GPU_Image **shootHitEffectFrames;
GPU_Image **exclam_notice_anim;
GPU_Image **exploder_frames;
GPU_Image **shooter_bullet_default_frames;
GPU_Image **shooter_bullet_explode_frames;

GPU_Image *floorTexture;
GPU_Image *floorTexture2;
GPU_Image *floorLightTexture;
GPU_Image *ceilingTexture;
GPU_Image *ceilingLightTexture;

// #SPRITES
Sprite *dash_anim_sprite;
Sprite *leftHandSprite;

// #SOUNDS
Sound *rapidfire_sound;
Sound *exploder_explosion;
Sound *player_default_shoot;
Sound *player_default_hurt;

// #SHADERS
int floor_shader;
GPU_ShaderBlock floor_shader_block;

int bloom_shader;
GPU_ShaderBlock bloom_shader_block;

// #PARTICLES (the global ones)
ParticleSpawner *player_hit_particles;
ParticleSpawner *bomb_explode_particles;

// #ROOMGEN
Room rooms[DUNGEON_SIZE][DUNGEON_SIZE] = {0};


// #VAR

SDL_Color client_self_color = {0};

v2 spawn_point = {500, 500};

bool can_exit = false;

double client_dungeon_seed_request_timer = 0;

bool loading_map = false;

long server_dungeon_seed = -1;
long client_dungeon_seed = -1;

int server_client_id_list[MP_MAX_CLIENTS] = {0};

int client_self_id = -1;
int next_id = 1234;

double game_speed_duration_timer = 0;
double game_speed = 1;
bool paused = false;

bool queued_player_death = false;

double screen_modulate_r = 1;
double screen_modulate_g = 1;
double screen_modulate_b = 1;

SDL_Color vignette_color = {0, 0, 0};
bool is_loading = false;
double loading_progress = 0;
BakedLightColor baked_light_grid[TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION][TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION];
bool fullscreen = false;
bool running = true;
arraylist *gameobjects = NULL;
Player *player = NULL;
bool keyPressArr[26];
bool render_debug = false;

bool lock_and_hide_mouse = true;
bool isLMouseDown = false;
bool isRMouseDown = false;
double startFov = 100;
double fov = 100;
const char *font = "font.ttf";
const SDL_Color fogColor = {0, 0, 0, 255};
double tanHalfFOV;
double tanHalfStartFOV;
double ambient_light = 0.5;
int floorRenderStart;
int **levelTileMap = NULL;
int **floorTileMap = NULL;
int **ceilingTileMap = NULL;
const int tileSize = WINDOW_WIDTH / 30;
double realFps;
bool isCameraShaking = false;
int camerashake_current_priority = 0;
int cameraShakeTicks;
int cameraShakeTicksLeft = 0;
double cameraShakeTimeToNextTick = 0.02;
double cameraShakeCurrentStrength = 0;
bool cameraShakeFadeActive = false;
RenderObject wallStripesToRender[RESOLUTION_X];
v2 cameraOffset = {0, 0};
v2 playerForward;
char *levelToLoad = NULL;
const double PLAYER_SHOOT_COOLDOWN = 0.5;
// #VAR END

// #DEBUG VAR


// #MAIN
int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) printf("Shit. \n");

    actual_screen = GPU_Init(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_INIT_DISABLE_VSYNC);
    SDL_SetWindowTitle(SDL_GetWindowFromID(actual_screen->context->windowID), "Goofy");

    screen_image = GPU_CreateImage(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_FORMAT_RGBA);
    GPU_SetImageFilter(screen_image, GPU_FILTER_NEAREST);
    screen = GPU_LoadTarget(screen_image);

    hud_image = GPU_CreateImage(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_FORMAT_RGBA);
    GPU_SetImageFilter(hud_image, GPU_FILTER_NEAREST);
    hud = GPU_LoadTarget(hud_image);

    // renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);

    if (argc >= 2) {
        levelToLoad = argv[1];
    }

    UI_init(get_window(), (v2){WINDOW_WIDTH, WINDOW_HEIGHT});

    make_ui();

    MP_init(SERVER_PORT);
    _MP_client_handle_recv = on_client_recv;
    _MP_on_client_connected = on_player_connect;
    _MP_on_client_disconnected = on_player_disconnect;
    _MP_server_handle_recv = on_server_recv;

    

    gameobjects = create_arraylist(10);

    init_textures();

    init_player(to_vec(500));

    add_game_object(player, PLAYER);

    reset_tilemap(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    bake_lights();

    init();
    
    //printf("Reached after init");
    // loading_map = true;
    // generate_dungeon();
    // load_dungeon();
    // loading_map = false;

    bool is_server = argc == 2 && !strcmp(argv[1], "server");
    if (DEBUG_FLAG) {
        is_server = !is_server;
    }

    if (is_server) {
        printf("creating server \n");
        MPServer();
    }
    MPClient(SERVER_IP);

    

    //printf("Reached after server client shit \n");

    u64 tick_timer = 0, render_timer = 0;
    u64 last_time = SDL_GetTicks64();
    while (running) {  // #GAME LOOP
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_input(event);
            UI_handle_event(event);
        }

        u64 now = SDL_GetTicks64();
        u64 delta = now - last_time;
        last_time = now;
        tick_timer += delta;
        // render_timer += delta;
        if (tick_timer >= 1000 / TPS) {
            realFps = 1000.0 / tick_timer;
            tick(mili_to_sec(tick_timer) * game_speed);
            render(mili_to_sec(tick_timer) * game_speed);
            tick_timer = 0;
        }
    }

    struct player_left_packet packet_data = {.id = client_self_id};

    MPPacket packet = {.type = PACKET_PLAYER_LEFT, .is_broadcast = true, .len = sizeof(packet_data)};

    MPClient_send(packet, &packet_data);


    if (MP_is_server) {
        MPPacket hl_packet = {.type = PACKET_HOST_LEFT, .len = 0, .is_broadcast = true};
        MPServer_send(hl_packet, NULL);
    }

    while (!can_exit) {

    }

    GPU_FreeImage(screen_image);

    GPU_Quit();

    SDL_Quit();
}

void init() {  // #INIT

    randomize();
    client_self_color.r = randi_range(25, 255);
    client_self_color.g = randi_range(25, 255);
    client_self_color.b = randi_range(25, 255);
    client_self_color.a = 255;

    GPU_SetBlendMode(screen_image, GPU_BLEND_NORMAL);


    //exit(1);

    bomb_explode_particles = malloc(sizeof(ParticleSpawner));
    *bomb_explode_particles = create_particle_spawner(V2_ZERO, 0);

    player_hit_particles = malloc(sizeof(ParticleSpawner));
    *player_hit_particles = create_particle_spawner(V2_ZERO, 0);
    player_hit_particles->spread = 2;
    player_hit_particles->dir = (v2){0.5, 1};
    player_hit_particles->min_size = to_vec(1000);
    player_hit_particles->max_size = to_vec(2000);
    player_hit_particles->min_speed = 70;
    player_hit_particles->max_speed = 160;
    player_hit_particles->target = NULL;
    player_hit_particles->sprite = (Sprite){.isAnimated = false, .texture = blood_particle};
    player_hit_particles->start_color = (SDL_Color){255, 0, 0, 255};
    player_hit_particles->end_color = (SDL_Color){180, 0, 0, 255};
    player_hit_particles->height_dir = 1;
    player_hit_particles->particle_lifetime = 2;
    player_hit_particles->bounciness = 0.1;
    player_hit_particles->floor_drag = 0.4;


    rapidfire_sound = create_sound("Sounds/shotgun_ability.wav");

    exploder_explosion = create_sound("Sounds/exploder_explosion.wav");

    

    init_cd_print();

    tanHalfFOV = tan(deg_to_rad(fov / 2));
    tanHalfStartFOV = tan(deg_to_rad(startFov / 2));

    

    for (int i = 0; i < 26; i++) keyPressArr[i] = false;

}  // #INIT END

v2 get_player_forward() {
    return v2_rotate_to((v2){1, 0}, deg_to_rad(player->angle));
}

void handle_input(SDL_Event event) {
    switch (event.type) {
        case SDL_QUIT:

            running = false;
            break;
        case SDL_KEYDOWN:
            key_pressed(event.key.keysym.sym);
            break;
        case SDL_KEYUP:
            key_released(event.key.keysym.sym);
            break;
        case SDL_MOUSEMOTION:
            if (player == NULL) {
                printf("player is null \n");
                return;
            }
            if (lock_and_hide_mouse) {
                player->angle += event.motion.xrel * X_SENSITIVITY;
                player->handOffset.x = lerp(player->handOffset.x, -event.motion.xrel * 2, 0.06);
                player->pitch += event.motion.yrel * Y_SENSITIVITY;
                player->handOffset.y = lerp(player->handOffset.y, -event.motion.yrel * 2, 0.06);
                player->pitch = clamp(player->pitch, -280, 280);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                isLMouseDown = true;
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                isRMouseDown = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                isLMouseDown = false;
            }
            if (event.button.button == SDL_BUTTON_RIGHT) {
                isRMouseDown = false;
            }
            break;
    }
}

void key_pressed(SDL_Keycode key) {
    if (key == SDLK_F8) {
        running = false;
        return;
    }
    if (key == SDLK_ESCAPE) {
        toggle_pause();
        return;
    }
    if (key == SDLK_o) {
        render_debug = !render_debug;
        return;
    }

    if (key == SDLK_LCTRL && !player->crouching) {
        
        player->crouching = true;
        
        double current_speed_sqr = v2_length_squared(player->vel);
        double crouch_speed = 2;
        v2 crouch_dir = playerForward;

        if (current_speed_sqr < crouch_speed * crouch_speed) {
            player->vel = v2_mul(crouch_dir, to_vec(crouch_speed));
        }
        
        
    }

    if (key == SDLK_r) {
        if (player->special != NULL) {
            activate_ability(player->special);
        }
    }

    if (key == SDLK_SPACE) {
        if (is_player_on_floor()) {
            player->height_vel = 370;
        }
    }
    player->height = clamp(player->height, 0, get_max_height());

    if (key == SDLK_LSHIFT) {
        activate_ability(player->utility);
    }
    if (key == SDLK_F11) {
        fullscreen = !fullscreen;
        update_fullscreen();
    }

    if (!in_range((char)key, 'a', 'z')) return;

    keyPressArr[(char)key - 'a'] = true;
}

void key_released(SDL_Keycode key) {
    if (key == SDLK_LSHIFT) {
        player->sprinting = false;
    }
    if (key == SDLK_LCTRL) {
        player->crouching = false;
    }

    if (!in_range((char)key, 'a', 'z')) return;

    keyPressArr[(char)key - 'a'] = false;
}

bool is_key_pressed(SDL_Keycode key) {
    return keyPressArr[(char)key - 'a'];
}

int get_key_axis(SDL_Keycode key1, SDL_Keycode key2) {
    return (int)is_key_pressed(key2) - (int)is_key_pressed(key1);
}

v2 get_key_vector(SDL_Keycode k1, SDL_Keycode k2, SDL_Keycode k3, SDL_Keycode k4) {
    return (v2){get_key_axis(k1, k2), get_key_axis(k3, k4)};
}



void playerTick(double delta) {

    static double update_pos_timer = 1.0 / CLIENT_UPDATE_RATE;

    if (player->collider == NULL) {
        exit(12);
    }

    //String height_str = String_from_double(player->height, 2);

    //write_to_debug_label(height_str);
    //printf("%.2f \n", player->height);

    double speed_multiplier = 1;



    player->height_vel -= 750 * delta;
    player->height += player->height_vel * (delta * 144);
    if (player->height > get_max_height() - player->tallness * 0.2) {
        player->height_vel = 0;
    }
    player->height = clamp(player->height, player->tallness * 0.8, get_max_height());

    player->collider->pos = player->pos;

    if (player->primary != NULL) player->primary->tick(player->primary, delta);
    if (player->secondary != NULL) player->secondary->tick(player->secondary, delta);
    
    if (player->utility != NULL) player->utility->tick(player->utility, delta);
    if (player->special != NULL) player->special->tick(player->special, delta);

    if (isLMouseDown) {
        activate_ability(player->primary);
    }
    if (isRMouseDown) {
        activate_ability(player->secondary);
    }

    v2 right = {1, 0};
    v2 move_dir = v2_rotate_to(right, deg_to_rad(player->angle));
    v2 move_dir_rotated = v2_rotate(move_dir, PI / 2);

    v2 keyVec = get_key_vector(SDLK_s, SDLK_w, SDLK_a, SDLK_d);

    if (!v2_equal(keyVec, to_vec(0))) {
        double t = sin(mili_to_sec(SDL_GetTicks64()) * (15 + (int)player->sprinting * 5)) * 3;
        player->handOffset.y = t * 2.5;
    } else {
        player->handOffset.y = lerp(player->handOffset.y, 0, 0.1);
    }

    double drag = 0;
    if (is_player_on_floor()) {
        if (player->crouching) {
            drag = 0.15;
        } else {
            drag = 1;
        }
    } 

    v2 movement_vec = v2_add(v2_mul(move_dir, to_vec(keyVec.x)), v2_mul(move_dir_rotated, to_vec(keyVec.y)));
    player->vel = v2_lerp(player->vel, movement_vec, delta * 10.0 * drag);


    
    if (player->sprinting) {
        speed_multiplier *= 1.25;
        fov = lerp(fov, startFov * 1.2, delta * 7);
    } else {
        fov = lerp(fov, startFov, delta * 7);
    }

    v2 finalVel = v2_mul(player->vel, to_vec(player->speed * speed_multiplier));

    bool move_without_ray = true;

    double movement = v2_length(finalVel) * delta;

    v2 movement_dir = v2_normalize(finalVel);

    RayCollisionData ray = castRay(player->pos, movement_dir);
    if (ray.hit) {
        double dist = v2_distance(ray.startpos, ray.collpos);
        if (dist < movement) {
            move_without_ray = false;
            player->pos = v2_add(player->pos, v2_mul(movement_dir, to_vec(dist - 10)));
        }
    }
    
    CollisionData player_coldata = getCircleTileMapCollision(*player->collider);
    if (player_coldata.didCollide) {
        player->pos = v2_add(player->pos, player_coldata.offset);
        //player->vel = v2_slide(player->vel, player_coldata.offset);
    }

    if (move_without_ray) 
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(delta)));
    
    

    update_pos_timer -= delta;
    if (update_pos_timer <= 0) {
        update_pos_timer = 1.0 / CLIENT_UPDATE_RATE;
        struct player_pos_packet packet_data = {
            .pos = player->pos,
            .height = player->height,
            .dir = playerForward,
            .id = client_self_id,
            .color = client_self_color
        };

        MPPacket packet = {
            .type = PACKET_PLAYER_POS,
            .len = sizeof(packet_data),
            .is_broadcast = true
        };

        MPClient_send(packet, &packet_data);
    }

    
    

}

void spriteTick(Sprite *sprite, double delta) {
    if (sprite == NULL) return;
    if (!sprite->isAnimated) return;
    if (sprite->animations == NULL) return;
    if (sprite->currentAnimationIdx == -1) return;
    animationTick(&sprite->animations[sprite->currentAnimationIdx], delta);
}

void objectTick(void *obj, int type, double delta) {
    
    update_entity_collisions(obj, type);
    
    if (is_projectile_type(type)) {
        projectile_tick(obj, delta);
        return;
    }

    switch (type) {
        case (int)PLAYER:
            playerTick(delta);
            break;
        case (int)PARTICLE:
            particle_tick(obj, delta);
            break;
        case (int)EFFECT:
            effectTick(obj, delta);
            break;
        case (int)SPRITE:
            spriteTick(obj, delta);
            break;
        case (int)PARTICLE_SPAWNER:
            particle_spawner_tick(obj, delta);
            break;
        case (int)BULLET:
            bulletTick(obj, delta);
            break;
        case (int)PLAYER_ENTITY:
            player_entity_tick(obj, delta);
            break; 
    }
}

// #TICK
void tick(double delta) {

    //printf("tick start");

    if (loading_map) {
        init_loading_screen();
        generate_dungeon();
        update_loading_progress(0.1);
        load_dungeon();
        loading_map = false;
    }

    if (client_dungeon_seed == -1) {
        client_dungeon_seed_request_timer -= delta;
        if (client_dungeon_seed_request_timer <= 0) {
            MPClient_send((MPPacket){.type = PACKET_REQUEST_DUNGEON_SEED, .len = 0, .is_broadcast = false}, NULL);
            client_dungeon_seed_request_timer = 0.5;
        }
    }


    if (game_speed_duration_timer <= 0) {
        game_speed = 1;
    } else {
        game_speed_duration_timer -= delta / game_speed; // so it counts by real time instead of scaled time
    }

    vignette_color = lerp_color(vignette_color, (SDL_Color){0, 0, 0}, delta);

    playerForward = get_player_forward();

    

    tanHalfFOV = tan(deg_to_rad(fov / 2));

    SDL_SetRelativeMouseMode(lock_and_hide_mouse); // bro this function was made for me

    

    if (isCameraShaking) {
        cameraShakeTimeToNextTick -= delta;
        if (cameraShakeTimeToNextTick <= 0) {
            cameraShakeTicksLeft -= 1;
            cameraShakeTimeToNextTick = 0.02;
            if (cameraShakeTicksLeft <= 0) {
                isCameraShaking = false;
                camerashake_current_priority = -9999;
            } else {
                v2 rawShake = {randf_range(-cameraShakeCurrentStrength, cameraShakeCurrentStrength), randf_range(-cameraShakeCurrentStrength, cameraShakeCurrentStrength)};
                double fade_factor = cameraShakeFadeActive? (double)cameraShakeTicksLeft / cameraShakeTicks : 1;
                cameraOffset = v2_mul(rawShake, to_vec(fade_factor));
            }
        }
    }
    cameraOffset = v2_lerp(cameraOffset, to_vec(0), 0.2);

    spriteTick(leftHandSprite, delta);

    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);
        objectTick(object->val, object->type, delta);
    }


    if (queued_player_death) {
        queued_player_death = false;
        _player_die();
    }
}

// for slower decay, make 'a' smaller.
double distance_to_color(double distance, double a) {
    return exp(-a * distance);
}

void renderDebug() {  // #DEBUG
    int tile_size = WINDOW_HEIGHT / (ROOM_HEIGHT * DUNGEON_SIZE);

    for (int r = 0; r < ROOM_HEIGHT * DUNGEON_SIZE; r++) {
        for (int c = 0; c < ROOM_WIDTH * DUNGEON_SIZE; c++) {
            bool is_wall = levelTileMap[r][c] == -1? 0 : 1;
            bool is_floor_light = floorTileMap[r][c] == P_FLOOR_LIGHT? true : false;

            if (is_wall) {
                GPU_RectangleFilled2(screen, (GPU_Rect){c * tile_size / 2, r * tile_size / 2, tile_size / 2, tile_size / 2}, GPU_MakeColor(255 * is_floor_light, 255, 255, 255));
            } else {
                GPU_RectangleFilled2(screen, (GPU_Rect){c * tile_size / 2, r * tile_size / 2, tile_size, tile_size / 2}, GPU_MakeColor(255 * is_floor_light, 0, 0, 255));
            }  
        }
    }

    GPU_RectangleFilled2(screen, (GPU_Rect){player->pos.x / tileSize * tile_size, player->pos.y / tileSize * tile_size, tile_size / 2, tile_size / 2}, GPU_MakeColor(255, 0, 0, 255));
    
}

v2 getRayDirByIdx(int i) {
    double x = tanHalfFOV;
    double idx = lerp(-x, x, ((double)(i + 1)) / RESOLUTION_X);

    v2 temp = (v2){1, idx};

    temp = v2_normalize(temp);
    Player *pref = player;
    v2 rayDir = v2_dir((v2){0, 0}, v2_rotate(temp, deg_to_rad(pref->angle)));
    return rayDir;
}

v2 worldToScreen(v2 pos, double height, bool allow_out_of_screen) { // gotta refactor this.
    
    if (v2_equal(pos, player->pos)) {
        return (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT};
    }

    double signed_angle_to_forward = v2_signed_angle_between(playerForward, v2_sub(pos, player->pos));

    double signed_angle_degrees = rad_to_deg(signed_angle_to_forward);

    if (!allow_out_of_screen && !in_range(signed_angle_degrees, -0.5 * fov, 0.5 * fov)) {
        return OUT_OF_SCREEN_POS;
    } else {
        if (!in_range(signed_angle_degrees, -90, 90)) {
            double dist_to_viewplane = abs(v2_distance(pos, player->pos) * v2_cos_angle_between(playerForward, v2_sub(pos, player->pos)));

            pos = v2_add(pos, v2_mul(playerForward, to_vec(dist_to_viewplane + 2)));

            // we need to recalculate prev variables
            signed_angle_to_forward = v2_signed_angle_between(playerForward, v2_sub(pos, player->pos));

            signed_angle_degrees = deg_to_rad(signed_angle_to_forward);
        }
    }


    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    double dist_to_player = v2_distance(pos, player->pos);


    double dist_to_viewplane = dist_to_player * cos_angle_to_forward;
    
    if (dist_to_viewplane == 0) dist_to_viewplane = 0.001;

    double fov_width_at_texture = 2 * dist_to_viewplane * tanHalfFOV;

    double angle = acos(cos_angle_to_forward);

    double texture_thing_width = dist_to_player * sin(angle); 

    double x_pos_sign = signed_angle_to_forward >= 0 ? 1 : -1;
    double ratio = fov_width_at_texture == 0? 0 : texture_thing_width / fov_width_at_texture;
    double x_pos = WINDOW_WIDTH / 2 + (ratio * WINDOW_WIDTH) * x_pos_sign;

    double fov_factor = tanHalfStartFOV / tanHalfFOV;
    double wallSize = WALL_HEIGHT * WALL_HEIGHT_MULTIPLIER * WINDOW_HEIGHT / dist_to_viewplane * fov_factor;
    double y_pos = WINDOW_HEIGHT / 2 + wallSize / 2 - ((height - get_player_height()) + WINDOW_HEIGHT / 2) / dist_to_viewplane;

    x_pos += cameraOffset.x;
    y_pos += -player->pitch + cameraOffset.y;

    return (v2){x_pos, y_pos};
}

v2 screenToFloor(v2 pos) {
    v2 rayDir = getRayDirByIdx((int)pos.x);

    double wallSize = abs(pos.y - WINDOW_HEIGHT / 2) * 2;  // size of a wall at that position
    if (wallSize == 0) {
        return to_vec(0);
    }

    double fovFactor = tanHalfStartFOV / tanHalfFOV;
    double dist = (WALL_HEIGHT * WALL_HEIGHT_MULTIPLIER * WINDOW_HEIGHT / wallSize) * fovFactor;

    double cosAngleToForward = v2_cos_angle_between(rayDir, playerForward);

    dist /= cosAngleToForward;


    bool is_ceiling = pos.y < WINDOW_HEIGHT / 2;

    double normalized_height = get_player_height() / get_max_height();

    double m = is_ceiling? 1 - normalized_height : normalized_height;

    dist *= m * WALL_HEIGHT_MULTIPLIER;

    return v2_add(player->pos, v2_mul(rayDir, to_vec(dist)));
}

struct floor_and_ceiling_thread_data {
    int start_row;
    int end_row; // exclusive
    void **pixels;
};

void render_floor_and_ceiling() {
    // Use the shader for everything.

    float l_positions[RESOLUTION_Y];
    float r_positions[RESOLUTION_Y];

    for (int i = 0; i < RESOLUTION_Y / 2; i++) {
        double screenY = i * WINDOW_HEIGHT / (RESOLUTION_Y / 2);

        v2 left = screenToFloor((v2){SDL_clamp(-cameraOffset.x, 0, RESOLUTION_X - 1), screenY + player->pitch - cameraOffset.y});
        v2 right = screenToFloor((v2){SDL_clamp(-cameraOffset.x + RESOLUTION_X - 1, 0, RESOLUTION_X - 1), screenY + player->pitch - cameraOffset.y});

        l_positions[i * 2] = left.x;
        l_positions[i * 2 + 1] = left.y;
        r_positions[i * 2] = right.x;
        r_positions[i * 2 + 1] = right.y;
    }


    drawSkybox();

    GPU_ActivateShaderProgram(floor_shader, &floor_shader_block);

    int floorTexLoc = GPU_GetUniformLocation(floor_shader, "floorTex");

    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "lValues"), 2, RESOLUTION_Y / 2, l_positions);
    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "rValues"), 2, RESOLUTION_Y / 2, r_positions);

    float window_size[2] = {WINDOW_WIDTH, WINDOW_HEIGHT};
    float lightmap_size[2] = {TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION, TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION};
    float tilemap_size[2] = {TILEMAP_WIDTH,  TILEMAP_HEIGHT};

    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "windowSize"), 2, 1, window_size);
    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "lightmapSize"), 2, 1, lightmap_size);
    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "tilemapSize"), 2, 1, tilemap_size);
    GPU_SetUniformf(GPU_GetUniformLocation(floor_shader, "pitch"), player->pitch + cameraOffset.y);
    
    float tex_ids[20] = {P_FLOOR, P_CEILING, P_FLOOR_LIGHT, P_CEILING_LIGHT};
    GPU_SetUniformfv(GPU_GetUniformLocation(floor_shader, "texIds"), 1, 20, tex_ids);

    GPU_SetShaderImage(floorTexture, GPU_GetUniformLocation(floor_shader, "floorTex"), 1);
    GPU_SetShaderImage(lightmap_image, GPU_GetUniformLocation(floor_shader, "lightmapTex"), 2);
    GPU_SetShaderImage(floor_and_ceiling_spritesheet, GPU_GetUniformLocation(floor_shader, "spritesheet"), 3);

    GPU_SetImageFilter(tilemap_image, GPU_FILTER_NEAREST);
    GPU_SetShaderImage(tilemap_image, GPU_GetUniformLocation(floor_shader, "tilemapTex"), 4);
    
    
    GPU_Target *image_target = GPU_LoadTarget(floor_and_ceiling_target_image);

    GPU_Blit(floorAndCeiling, NULL, screen, cameraOffset.x, cameraOffset.y);

    GPU_FreeTarget(image_target);

    GPU_BlitRect(floor_and_ceiling_target_image, NULL, screen, NULL);
    
    GPU_DeactivateShaderProgram();
}

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light, SDL_Color custom_color) {
    

    v2 screen_pos = worldToScreen(pos, height, false);

    if (v2_equal(screen_pos, OUT_OF_SCREEN_POS)) {
        return;
    }
    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    double dist_to_player = v2_distance(pos, player->pos);

    double dist_to_viewplane = dist_to_player * cos_angle_to_forward;

    double fov_factor = tanHalfFOV / tanHalfStartFOV;

    v2 final_size = v2_div(size, to_vec(dist_to_viewplane * fov_factor));

    GPU_Rect dstRect = {
        screen_pos.x - final_size.x / 2,
        screen_pos.y - final_size.y / 2,
        final_size.x,
        final_size.y
    };

    int rgb[3] = {255, 255, 255};
    if (affected_by_light) {
        double light;
        if (dist_to_player > 150) {
            light = distance_to_color(dist_to_player - 150, 0.005) * 0.8;
        } else {
            light = 0.8;
        }

        int color = 255 * light;
    
        BakedLightColor baked_color = get_light_color_by_pos(pos, 0, 0);

        

        rgb[0] = color * baked_color.r;
        rgb[1] = color * baked_color.g;
        rgb[2] = color * baked_color.b;

        clampColors(rgb);
    }

    rgb[0] *= (double)(custom_color.r) / 255;
    rgb[1] *= (double)(custom_color.g) / 255;
    rgb[2] *= (double)(custom_color.b) / 255;
    

    GPU_SetRGB(texture, rgb[0], rgb[1], rgb[2]);

    
    GPU_BlitRect(texture, NULL, screen, &dstRect);
    GPU_SetRGB(texture, 255, 255, 255);
}

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height) {
    GPU_Image *texture = getSpriteCurrentTexture(dir_sprite_current_sprite(dSprite, pos));

    renderTexture(texture, pos, size, height, true, (SDL_Color){255, 255, 255, 255});
}

void renderEntity(Entity entity) {  // RENDER ENTITY
    GPU_Image *texture = getSpriteCurrentTexture(entity.sprite);

    GPU_SetRGBA(texture, entity.color.r, entity.color.g, entity.color.b, 20);

    renderTexture(texture, entity.pos, entity.size, entity.height, entity.affected_by_light, entity.color);
}

typedef struct WallStripe {
    v2 pos, normal; // 16, 16
    double size; // 8
    double brightness; // 8  // a bunch of rendering bullshit:
    double collIdx; // 8
    double wallWidth; // 8
    GPU_Image *texture; // 8
    int i; // 4
} WallStripe;

RenderObject getWallStripe(int i) {
    v2 ray_dir = getRayDirByIdx(i);

    RayCollisionData data = castRay(player->pos, ray_dir);

    if (!data.hit) {
        return (RenderObject){.isnull = true};
    }

    WallStripe *stripe = malloc(sizeof(WallStripe));
    stripe->i = i;
    stripe->texture = data.colliderTexture;

    double cos_angle_to_forward = v2_cos_angle_between(ray_dir, playerForward);
    double dist = v2_distance(data.startpos, data.collpos) * cos_angle_to_forward;
    double fov_factor = tanHalfStartFOV / tanHalfFOV;
    double final_size = WALL_HEIGHT * WALL_HEIGHT_MULTIPLIER * WINDOW_HEIGHT / dist * fov_factor;

    stripe->size = final_size;
    
    stripe->brightness = dist > 150? distance_to_color(dist - 150, 0.01) : 1;
    stripe->normal = data.normal;
    stripe->collIdx = data.collIdx;
    stripe->wallWidth = data.wallWidth;
    stripe->pos = data.collpos;

    RenderObject currentRenderObj = {.isnull = false};

    double real_dist = dist / cos_angle_to_forward;

    currentRenderObj.dist_squared = real_dist * real_dist;
    currentRenderObj.val = stripe;
    currentRenderObj.type = WALL_STRIPE;

    return currentRenderObj;
}

typedef struct WallThreadData {
    int idx;
    arraylist *targetlist;
} WallThreadData;

int addWallStripes_Threaded(void *data) {
    int idx = *(int *)data;
    for (int i = RESOLUTION_X / NUM_WALL_THREADS * idx; i < RESOLUTION_X / NUM_WALL_THREADS * (idx + 1); i++) {
        wallStripesToRender[i] = getWallStripe(i);
    }
}

int _cmp(const void *a, const void *b) {
    const RenderObject *ra = a;
    const RenderObject *rb = b;
    double d1 = ra->isnull? 999999999999 : ra->dist_squared;
    double d2 = rb->isnull? 999999999999 : rb->dist_squared;

    return d2 - d1;
}

RenderObject *getRenderList() {
    RenderObject *renderList = array(RenderObject, RESOLUTION_X + gameobjects->length - 1);

    if (NUM_WALL_THREADS == 1) {
        int i = 0;
        addWallStripes_Threaded(&i); // hehe its not threaded
    } else {
        SDL_Thread *threads[NUM_WALL_THREADS];

        int indicies[NUM_WALL_THREADS];

        for (int i = 0; i < NUM_WALL_THREADS; i++) {
            indicies[i] = i;
            threads[i] = SDL_CreateThread(addWallStripes_Threaded, "thread wall", &indicies[i]);
        }
        for (int i = 0; i < NUM_WALL_THREADS; i++) {
            SDL_WaitThread(threads[i], NULL);
        }
    }

    for (int i = RESOLUTION_X - 1; i >= 0; i--) {
        array_append(renderList, wallStripesToRender[i]);
    }

    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);

        RenderObject currentRObj = (RenderObject){.isnull = true};
        v2 pos = V2_ZERO;
        
        if (is_entity_type(object->type)) {

            if (is_projectile_type(object->type)) {
                cd_print(true, "detected \n");
            }

            currentRObj = (RenderObject){.isnull = false};


            currentRObj.val = object->val;
            currentRObj.type = ENTITY;
            pos = ((Entity *)object->val)->pos;

        } else {
            continue;
        }

        currentRObj.dist_squared = v2_distance_squared(pos, player->pos);
        
        array_append(renderList, currentRObj);
        
    }

    int l = array_length(renderList);

    SDL_qsort(renderList, l, sizeof(RenderObject), _cmp);

    return renderList;
}

void clampColors(int rgb[3]) {
    int max_idx = 0;
    for (int i = 1; i < 3; i++) if (rgb[i] > rgb[max_idx]) max_idx = i;

    if (rgb[max_idx] >= 255) {
        double divisor = (double)rgb[max_idx] / 255;
        for (int i = 0; i < 3; i++) {
            rgb[i] /= divisor;   
        }
    }
}

void renderWallStripe(WallStripe *stripe) {

    double angleLightModifier = sin(v2_get_angle(stripe->normal));

    GPU_Image *texture = stripe->texture;
    v2 textureSize = (v2){texture->w, texture->h};

    GPU_Rect srcRect = {(int)loop_clamp(stripe->collIdx * stripe->wallWidth, 0, textureSize.x), 0, 1, textureSize.y};

    double dist_squared = ((RenderObject *)stripe)->dist_squared;
    double dist = sqrt(dist_squared);

    double p_height = (get_player_height() / get_max_height() - 0.5) * (stripe->size);

    GPU_Rect dstRect = {
        stripe->i * WINDOW_WIDTH / RESOLUTION_X + cameraOffset.x, 
        WINDOW_HEIGHT / 2 - stripe->size / 2 + p_height - player->pitch + cameraOffset.y,
        WINDOW_WIDTH / RESOLUTION_X + 1,
        stripe->size
    };
    

    BakedLightColor baked_light_color = get_light_color_by_pos(v2_add(stripe->pos, v2_mul(stripe->normal, to_vec(0.5))), 0, 0);

    double baked_light_brightness = (baked_light_color.r + baked_light_color.g + baked_light_color.b) / 3;

    double brightness = SDL_clamp(stripe->brightness + baked_light_brightness / 2, 0, 1);
    // baked lights
    
    int rgb[3] = {
        125 * baked_light_color.r * brightness,
        125 * baked_light_color.g * brightness,
        125 * baked_light_color.b * brightness
    };
    
    clampColors(rgb);


    

    GPU_SetRGB(texture, rgb[0], rgb[1], rgb[2]);
    GPU_BlitRect(texture, &srcRect, screen, &dstRect);

    // GPU_Rectangle(screen, dstRect.x, dstRect.y, dstRect.x + dstRect.w, dstRect.y + dstRect.h, GPU_MakeColor(255, 0, 0, 255));

    // SDL_SetTextureColorMod(texture, rgb[0], rgb[1], rgb[2]);
    // GPU_BlitRect(texture, &srcRect, &dstRect);
    free(stripe);
}

BakedLightColor get_light_color_by_pos(v2 pos, int row_offset, int col_offset) {
    double py = pos.y / tileSize + ((double)row_offset / BAKED_LIGHT_RESOLUTION);
    double px = pos.x / tileSize + ((double)col_offset / BAKED_LIGHT_RESOLUTION);
    if (in_range(px, 0, TILEMAP_WIDTH - 1) && in_range(py, 0, TILEMAP_HEIGHT - 1)) {
        int baked_light_row = (int)(py * BAKED_LIGHT_RESOLUTION);
        int baked_light_col = (int)(px * BAKED_LIGHT_RESOLUTION);
        BakedLightColor res = baked_light_grid[baked_light_row][baked_light_col];

        return res;
    } else {
        return (BakedLightColor){ambient_light, ambient_light, ambient_light};
    }
}

void render_hand() {

    GPU_Rect leftHandRect = {player->handOffset.x + cameraOffset.x, player->handOffset.y + cameraOffset.y, WINDOW_WIDTH, WINDOW_HEIGHT};

    BakedLightColor baked_light_color;

    Animation current_anim = leftHandSprite->animations[leftHandSprite->currentAnimationIdx];
    int current_anim_idx = leftHandSprite->currentAnimationIdx;

    bool first_check = current_anim_idx == 0;
    bool second_check = current_anim_idx == 1 && current_anim.frame > 1;
    if (first_check || second_check) {
        baked_light_color = get_light_color_by_pos(player->pos, 0, 0);
        baked_light_color.r = 0.3 + baked_light_color.r * 0.7;
        baked_light_color.g = 0.3 + baked_light_color.g * 0.7;
        baked_light_color.b = 0.3 + baked_light_color.b * 0.7;
    } else {
        baked_light_color.r = 2;
        baked_light_color.g = 1.8;
        baked_light_color.b = 1.5;
    }

    GPU_Image *texture = getSpriteCurrentTexture(leftHandSprite);
    

    if (texture != NULL) {
        int rgb[3] = {
            baked_light_color.r * 125,
            baked_light_color.g * 125,
            baked_light_color.b * 125
        };

        clampColors(rgb);
 
        GPU_SetRGB(texture, rgb[0], rgb[1], rgb[2]);
        GPU_BlitRect(texture, NULL, hud, &leftHandRect);
    }
}

void render_health_bar() {
    
    v2 tex_size = get_texture_size(healthbar_texture);

    v2 scale = {3, 3};

    GPU_Rect outline_rect = {
        0,
        0,
        tex_size.x * scale.x,
        tex_size.y * scale.y 
    };

    GPU_Rect health_rect = {
        16 * scale.x,
        18 * scale.y,
        78 * scale.x * ((double)player->health / player->maxHealth),
        11 * scale.y
    };

    GPU_Rect health_bg_rect = {
        16 * scale.x,
        18 * scale.y,
        78 * scale.x,
        11 * scale.y
    };

    // SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);

    // 180d2f

    GPU_RectangleFilled2(screen, health_bg_rect, (SDL_Color){24, 13, 47, 255});
    GPU_RectangleFilled2(screen, health_rect, (SDL_Color){200, 0, 0, 255});

    GPU_BlitRect(healthbar_texture, NULL, hud, &outline_rect);
}

void render_ability_helper(v2 pos, Ability *ability) {
    if (ability == NULL) return;

    v2 size = to_vec(72);

    GPU_Rect rect = {pos.x, pos.y, size.x, size.y};

    GPU_Rect frame_rect = {
        pos.x - 3,
        pos.y - 3,
        size.x + 6,
        size.y + 6
    };

    if (ability->texture != NULL) {
        GPU_BlitRect(ability->texture, NULL, hud, &rect);
    } else {
        printf("What the hell. \n");
    }
    double primary_progress = ability->timer == ability->cooldown? 0 : ability->timer / ability->cooldown;

    GPU_Rect primary_progress_rect = {
        pos.x, 
        pos.y,
        size.x,
        size.y * primary_progress
    };

    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    
    GPU_RectangleFilled2(hud, primary_progress_rect, GPU_MakeColor(0, 0, 0, 170));


    // SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    GPU_BlitRect(ability_icon_frame, NULL, hud, &frame_rect);
}

void render_ability_hud() {
    render_ability_helper((v2){WINDOW_WIDTH * 1/16      , WINDOW_HEIGHT * 5/6}, player->primary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 80 , WINDOW_HEIGHT * 5/6}, player->secondary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 160, WINDOW_HEIGHT * 5/6}, player->utility);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 240, WINDOW_HEIGHT * 5/6}, player->special);
}


void renderHUD(double delta) {

    GPU_BlitRect(getSpriteCurrentTexture(dash_anim_sprite), NULL, hud, NULL);
    spriteTick(dash_anim_sprite, delta);

    GPU_SetRGB(vignette_texture, vignette_color.r, vignette_color.g, vignette_color.b);
    GPU_BlitRect(vignette_texture, NULL, hud, NULL);

    render_hand();

    render_health_bar();

    render_ability_hud();

    GPU_Rect crosshairRect = {WINDOW_WIDTH / 2 - 8, WINDOW_HEIGHT / 2 - 8, 16, 16};

    GPU_BlitRect(crosshair, NULL, hud, &crosshairRect);

    int shots = max(player->pendingShots, (int)(player->shootChargeTimer * 3));

    GPU_Rect playerPendingShotsRect = {WINDOW_WIDTH / 2 + -10 * shots, WINDOW_HEIGHT * 0.8, 20 * shots, WINDOW_HEIGHT * 0.05};

    UI_render(hud, UI_get_root());

}

void drawSkybox() {
    GPU_Image *tex = skybox_texture;

    double x = loop_clamp(player->angle / startFov * WINDOW_WIDTH, 0, WINDOW_WIDTH);

    double yOffsets = -player->pitch;

    GPU_Rect skybox_rect = {-x, yOffsets, WINDOW_WIDTH * 2, WINDOW_HEIGHT / 2};

    GPU_BlitRect(tex, NULL, screen, &skybox_rect);
}

void render(double delta) {  // #RENDER

    if (loading_map) {
        return;
    }

    //printf("render start \n");

    GPU_Clear(screen);
    GPU_Clear(hud);

    String title = String("FPS: ");
    String fps_text = String_new(20);
    decimal_to_text(realFps, fps_text.data);
    String final = String_concat(title, fps_text);

    SDL_SetWindowTitle(get_window(), final.data);

    String_delete(&final);
    String_delete(&fps_text);
    String_delete(&title);

    // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    RenderObject *renderList = getRenderList();

    render_floor_and_ceiling();

    for (int i = 0; i < array_length(renderList); i++) {
        RenderObject rObj = renderList[i];
        if (rObj.isnull) {
            continue;
        }

        switch (rObj.type) {
            case (int)ENTITY:
                renderEntity(*(Entity *)rObj.val);
                break;
            case (int)WALL_STRIPE:
                renderWallStripe((WallStripe *)rObj.val);
                break;
            case (int)BULLET:;
                Bullet *bullet = rObj.val;
                if (bullet->dirSprite != NULL) {
                    renderDirSprite(bullet->dirSprite, bullet->entity.pos, bullet->entity.size, bullet->entity.height);
                } else {
                    renderEntity(bullet->entity);
                }
                break;
        }
    }

    array_free(renderList);

    if (render_debug) renderDebug();

    renderHUD(delta);

    screen_modulate_r = lerp(screen_modulate_r, 1, delta / 2);
    screen_modulate_g = lerp(screen_modulate_g, 1, delta / 2);
    screen_modulate_b = lerp(screen_modulate_b, 1, delta / 2);

    GPU_ActivateShaderProgram(bloom_shader, &bloom_shader_block);

    GPU_SetShaderImage(screen_image, GPU_GetUniformLocation(bloom_shader, "tex"), 1);
    
    float res[2] = {WINDOW_WIDTH, WINDOW_HEIGHT};

    GPU_SetUniformfv(GPU_GetUniformLocation(bloom_shader, "texResolution"), 2, 1, res);

    GPU_Blit(screen_image, NULL, actual_screen, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    GPU_DeactivateShaderProgram();

    GPU_Blit(hud_image, NULL, actual_screen, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    GPU_Flip(actual_screen);
} // #RENDER END

// #PLAYER INIT
void init_player(v2 pos) {
    if (player == NULL) player = malloc(sizeof(Player));
    player->angle = 0;
    player->pos = pos;
    player->vel = V2_ZERO;
    player->speed = 70;
    player->collSize = 7;
    player->sprinting = false;
    player->canShoot = true;
    player->shootCooldown = PLAYER_SHOOT_COOLDOWN;
    player->shootChargeTimer = 0;
    player->ShootTickTimer = 0.15;
    player->pendingShots = 0;
    player->max_pending_shots = 10;
    player->collider = malloc(sizeof(CircleCollider));
    player->collider->radius = PLAYER_COLLIDER_RADIUS;
    player->collider->pos = player->pos;

    if (player->collider == NULL) {
        exit(-123456789);
    }

    player->maxHealth = 10;
    player->health = player->maxHealth;
    player->tallness = 11000;
    player->height = player->tallness;
    player->height_vel = 0;
    player->crouching = false;

    randomize_player_abilities();
}

// CLEAR
RayCollisionData  ray_object(Raycast ray, obj *object) {

    RayCollisionData ray_data = {0};

    switch (object->type) {
        case (int)PLAYER:;
            ray_data = ray_circle(ray, *player->collider);
            if (ray_data.hit) {
                ray_data.collider = player;
                ray_data.colliderType = PLAYER;
            }
            break;
        case (int)PLAYER_ENTITY:;
            PlayerEntity *player_entity = object->val;
            ray_data = ray_circle(ray, player_entity->collider);
            if (ray_data.hit) {
                ray_data.collider = player_entity;
                ray_data.colliderType = PLAYER_ENTITY;
            }
            break;
        default:
            ray_data.hit = false;
            break;
    }

   
    return ray_data;
}

RayCollisionData castRay(v2 pos, v2 dir) {
    // use DDA stupid

    // set tilesize to 1

    v2 start = v2_div(pos, to_vec(tileSize));
    v2 scalingVec = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};

    v2 startCell = v2_floor(v2_div(pos, to_vec(tileSize)));

    v2 currentCell = startCell;

    v2 currentRayLengths;

    v2 lastStepDir = (v2){0, 0};

    v2 step = {1, 1};
    if (dir.x < 0) {
        step.x = -1;
        currentRayLengths.x = (start.x - startCell.x) * scalingVec.x;
    } else {
        currentRayLengths.x = (startCell.x + 1 - start.x) * scalingVec.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        currentRayLengths.y = (start.y - startCell.y) * scalingVec.y;
    } else {
        currentRayLengths.y = (startCell.y + 1 - start.y) * scalingVec.y;
    }

    bool found = false;
    int tile;
    double maxDist = 100;
    double dist = 0;
    while (!found && dist < maxDist) {
        if (currentRayLengths.x < currentRayLengths.y) {
            currentCell.x += step.x;
            dist = currentRayLengths.x;
            currentRayLengths.x += scalingVec.x;
            lastStepDir = (v2){step.x, 0};
        } else {
            currentCell.y += step.y;
            dist = currentRayLengths.y;
            currentRayLengths.y += scalingVec.y;
            lastStepDir = (v2){0, step.y};
        }

        if (in_range((int)currentCell.y, 0, TILEMAP_HEIGHT - 1) && in_range((int)currentCell.x, 0, TILEMAP_WIDTH - 1)) {
            int t = levelTileMap[(int)currentCell.y][(int)currentCell.x];

            if (t != -1) {
                found = true;
                tile = t;
            }
        }
    }

    RayCollisionData data;
    if (found) {
        data.hit = true;
        data.wallWidth = tileSize;
        data.startpos = pos;
        data.collpos = v2_add(pos, v2_mul(dir, to_vec(dist * tileSize)));
        data.normal = v2_mul(lastStepDir, to_vec(-1));
        data.colliderTexture = wallTexture;
        data.colliderType = -1;
        if (lastStepDir.x != 0) {
            data.collIdx = (data.collpos.y - floor(data.collpos.y / tileSize) * tileSize) / tileSize;
        } else {
            data.collIdx = (data.collpos.x - floor(data.collpos.x / tileSize) * tileSize) / tileSize;
        }
        return data;

    } else {
        data.hit = false;
        return data;
    }
}

RayCollisionData ray_circle(Raycast ray, CircleCollider circle) {
    
    RayCollisionData data;
    
    if (v2_distance(ray.pos, circle.pos) <= circle.radius || v2_dot(ray.dir, v2_dir(ray.pos, circle.pos)) < 0) {
        data.hit = false;
        return data;
    }

    

    v2 rayToCircle = v2_sub(circle.pos, ray.pos);
    double a = v2_dot(rayToCircle, ray.dir);
    double cSquared = v2_length_squared(rayToCircle);
    double bSquared = cSquared - a * a;  // pythagoras
    if (circle.radius * circle.radius - bSquared < 0) {
        data.hit = false;
        return data;
    }                                                           // no imaginary numbers pls
    double d = sqrt(circle.radius * circle.radius - bSquared);  // more pythagoras

    // raypos + raydir * (a - d)
    v2 collision_pos = v2_add(ray.pos, v2_mul(ray.dir, to_vec(a - d)));  // woohoo!

    v2 normal = v2_dir(circle.pos, collision_pos);
    double collIdx = v2_get_angle(v2_dir(circle.pos, data.collpos)) / (2 * PI);

    data.hit = true;
    data.normal = normal;
    data.startpos = ray.pos;
    data.collpos = collision_pos;
    data.wallWidth = circle.radius * PI;

    

    data.collIdx = collIdx;
    

    return data;
}

void freeObject(void *val, int type) {
    switch (type) {
        case (int)PARTICLE:
        case (int)EFFECT:;
            Effect *effect = (Effect *)val;
            freeObject(effect->entity.sprite, SPRITE);
            free(effect);
            break;
        case (int)SPRITE:;  // not gonna destroy the texture
            Sprite *s = val;
            free(s->animations);
            free(s);
            break;
        case (int)DIR_SPRITE:;
            DirectionalSprite *dSprite = val;
            for (int i = 0; i < dSprite->dirCount; i++) {
                freeObject(dSprite->sprites[i], SPRITE);
            }
            free(dSprite->sprites);
            free(dSprite);
            break;
        

        case (int)BULLET:;
            Bullet *bullet = val;
            free(bullet->collider);
            if (bullet->dirSprite != NULL) {
                freeObject(bullet->dirSprite, DIR_SPRITE);
            }
            if (bullet->entity.sprite != NULL) {
                freeObject(bullet->entity.sprite, SPRITE);
            }
            if (bullet->particle_spawner != NULL) {
                remove_game_object(bullet->particle_spawner, PARTICLE_SPAWNER);
            }

            free(bullet);
            break;

        default:
            free(val);
            break;
    }
}


void shakeCamera(double strength, int ticks, bool fade, int priority) {
    if (priority < camerashake_current_priority && isCameraShaking) return;
    camerashake_current_priority = priority;
    isCameraShaking = true;
    cameraShakeCurrentStrength = strength;
    cameraShakeTimeToNextTick = 0.02;
    cameraShakeFadeActive = fade;
    cameraShakeTicks = ticks;
    cameraShakeTicksLeft = ticks;
}

void add_game_object(void *val, int type) {
    switch (type) {
        default:
            arraylist_add(gameobjects, val, type);
            break;
    }
}

// Frees and removes the game object.
void remove_game_object(void *val, int type) {
    arraylist_remove(gameobjects, arraylist_find(gameobjects, val));
    freeObject(val, type);
}

void init_tilemap(int ***gridPtr, int cols, int rows) {
    *gridPtr = malloc(sizeof(int *) * rows);
    for (int i = 0; i < rows; i++) {
        (*gridPtr)[i] = malloc(sizeof(int) * cols);
        for (int j = 0; j < cols; j++) {
            (*gridPtr)[i][j] = -1;
        }
    }
}

void reset_tilemap(int ***gridPtr, int cols, int rows) {

    if (*gridPtr == NULL) {
        init_tilemap(gridPtr, cols, rows);
    }

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            (*gridPtr)[r][c] = -1;
        }
    }
}

void clearLevel() {

    for (int i = gameobjects->length - 1; i >= 0; i--) {
        obj *object = arraylist_get(gameobjects, i);

        if (object->type == PLAYER) {
            continue;
        }

        remove_game_object(object->val, object->type);
    }
}

void spawn_floor_light(v2 pos) {
    LightPoint *light = malloc(sizeof(LightPoint));
    light->color = (SDL_Color){255, 50, 50};//{255, 200, 100};
    light->strength = 4;
    light->radius = 140;
    light->pos = pos;
    add_game_object(light, LIGHT_POINT);
}

void spawn_ceiling_light(v2 pos) {
    LightPoint *light = malloc(sizeof(LightPoint));
    light->color = (SDL_Color){randf_range(200, 255), randf_range(130, 160), 70};//{255, 200, 100};
    light->strength = 5;
    light->radius = 400;
    light->pos = pos;
    add_game_object(light, LIGHT_POINT);
}

void place_entity(v2 pos, int type) {
    switch (type) {
        case (int)P_PLAYER:
            player->pos = pos;
            spawn_point = pos;
            break;
    }
}

void load_level(char *file) {
    clearLevel();

    reset_tilemap(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    FILE *fh = fopen(file, "r");
    if (fh == NULL) {
        printf("File doesnt exist. \n");
        return;
    }

    int data_count = TILEMAP_HEIGHT * TILEMAP_WIDTH * 4 + 1; // worst case

    int data[data_count];

    fread(data, sizeof(int), data_count, fh);

    int data_ptr = 0;

    SaveType save_type = (SaveType)data[data_ptr++];

    if (save_type != ST_LEVEL) {
        printf("File is not a level! Not loading that! \n");
        fclose(fh);
        return;
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            floorTileMap[r][c] = data[data_ptr++];
            if (floorTileMap[r][c] == P_FLOOR_LIGHT) {
                v2 tile_mid = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                spawn_floor_light(tile_mid);
            }
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            levelTileMap[r][c] = data[data_ptr++];
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            ceilingTileMap[r][c] = data[data_ptr++];


            if (ceilingTileMap[r][c] == P_CEILING_LIGHT) {
                v2 tile_mid = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                spawn_ceiling_light(tile_mid);
            }
        }
    }
    
    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            int etype = data[data_ptr++];

            v2 tile_mid = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};

            place_entity(tile_mid, etype);
        }
    }


    fclose(fh);
    // create tilemap image for floor shader

    tilemap_image = GPU_CreateImage(TILEMAP_WIDTH, TILEMAP_HEIGHT, GPU_FORMAT_RGBA);

    SDL_Surface *surface = GPU_CopySurfaceFromImage(tilemap_image);

    

    for (int row = 0; row < TILEMAP_HEIGHT; row++) {
        for (int col = 0; col < TILEMAP_WIDTH; col++) {

            int floorTile = floorTileMap[row][col] == -1? 0 : floorTileMap[row][col];
            int ceilingTile = ceilingTileMap[row][col] == -1? 0 : ceilingTileMap[row][col];

            SDL_Color color = {
                SDL_clamp(ceilingTile * 10, 0, 255),
                SDL_clamp(0, 0, 255),
                SDL_clamp(floorTile * 10, 0, 255),
                SDL_clamp(255, 0, 255)
            };

            ((int *)surface->pixels)[row * TILEMAP_WIDTH + col] = color.a << 24 | color.b << 16 | color.g << 8 | color.r;
        }
    }

    GPU_UpdateImage(tilemap_image, NULL, surface, NULL);

    SDL_FreeSurface(surface);

    bake_lights();

}

void freeAnimation(Animation *anim) {
    free(anim->frames);
    free(anim);
}

Animation create_animation(int frameCount, int priority, GPU_Image **frames) {
    Animation anim;
    anim.playing = false;
    anim.frameCount = frameCount;
    if (frames == NULL) {
        anim.frames = malloc(sizeof(GPU_Image *) * frameCount);
    } else {
        anim.frames = frames;
    }
    anim.frame = 0;
    anim.fps = 5;
    anim.loop = true;
    anim.timeToNextFrame = 0;
    anim.priority = priority;

    return anim;
}

void animationTick(Animation *anim, double delta) {
    if (!anim->playing) return;
    anim->timeToNextFrame -= delta;
    if (anim->timeToNextFrame <= 0) {
        anim->timeToNextFrame = 1 / anim->fps;
        if (anim->frame + 1 >= anim->frameCount) {
            if (anim->loop)
                anim->frame = 0;
            else
                anim->playing = false;
        } else {
            anim->frame++;
        }
    }
}

Sprite *createSprite(bool isAnimated, int animCount) {
    Sprite *sprite = malloc(sizeof(Sprite));
    sprite->isAnimated = isAnimated;
    sprite->texture = NULL;
    sprite->currentAnimationIdx = 0;
    sprite->animCount = animCount;
    if (isAnimated) {
        sprite->animations = malloc(sizeof(Animation ) * animCount);
    } else {
        sprite->animations = NULL;
    }
    return sprite;
}

void spritePlayAnim(Sprite *sprite, int idx) {
    bool less_priority = sprite->currentAnimationIdx != -1 
    && sprite->animations[sprite->currentAnimationIdx].priority < sprite->animations[idx].priority
    && sprite->animations[sprite->currentAnimationIdx].playing;
    if (less_priority) {
        return;
    }
    sprite->currentAnimationIdx = idx;
    for (int i = 0; i < sprite->animCount; i++) {
        sprite->animations[i].playing = false;
    }
    sprite->animations[idx].frame = 0;
    sprite->animations[idx].timeToNextFrame = 1 / sprite->animations[idx].fps;
    sprite->animations[idx].playing = true;
}

GPU_Image *getSpriteCurrentTexture(Sprite *sprite) {
    if (!(sprite->isAnimated)) {
        return sprite->texture;
    } else {
        if (sprite->currentAnimationIdx == -1) return NULL;
        Animation current = sprite->animations[sprite->currentAnimationIdx];
        return current.frames[current.frame];
    }
}

Effect *createEffect(v2 pos, v2 size, Sprite *sprite, double lifeTime) {
    Effect *effect = malloc(sizeof(Effect));

    if (effect == NULL) {
        printf("Failed to malloc effect \n");
    }

    effect->entity.pos = pos;
    effect->entity.size = size;
    effect->entity.sprite = sprite;
    effect->entity.color = (SDL_Color){255, 255, 255, 255};
    effect->entity.height = get_max_height() * 0.5;
    effect->life_time = lifeTime;
    effect->life_timer = effect->life_time;

    return effect;
}

// Todo: add shoot cooldown for base shooting
void ability_shoot_activate(Ability *ability) {
    play_sound(player_default_shoot, 0.3);
    _shoot(0);
}

void effectTick(Effect *effect, double delta) {

    effect->life_timer -= delta;
    if (effect->life_timer <= 0) {
        remove_game_object(effect, EFFECT);
        return;
    }
    spriteTick(effect->entity.sprite, delta);
}

RayCollisionData castRayForEntities(v2 pos, v2 dir) {
    Raycast ray = {pos, dir};

    RayCollisionData data;
    double minSquaredDist = INFINITY;

    for (int i = 0; i < gameobjects->length; i++) {
        RayCollisionData newData = ray_object(ray, arraylist_get(gameobjects, i));
        if (!newData.hit) continue;

        double currentSquaredDist = v2_distance_squared(pos, newData.collpos);
        if (currentSquaredDist < minSquaredDist) {
            minSquaredDist = currentSquaredDist;
            data = newData;
        }
    }

    return data;
}

RayCollisionData castRayForAll(v2 pos, v2 dir) {
    RayCollisionData entity_ray_data = castRayForEntities(pos, dir);
    RayCollisionData wall_ray_data = castRay(pos, dir);

    if (!entity_ray_data.hit) return wall_ray_data;
    if (!wall_ray_data.hit) return entity_ray_data;

    double entity_squared_dist = v2_distance_squared(pos, entity_ray_data.collpos);
    double wall_squared_dist = v2_distance_squared(pos, wall_ray_data.collpos);

    if (entity_squared_dist < wall_squared_dist) return entity_ray_data;
    return wall_ray_data;
}

DirectionalSprite *createDirSprite(int dirCount) {
    DirectionalSprite *dSprite = malloc(sizeof(DirectionalSprite));
    dSprite->dir = (v2){1, 0};
    dSprite->dirCount = dirCount;
    dSprite->sprites = malloc(sizeof(Sprite *) * dirCount);
    dSprite->current_anim = 0;
    dSprite->playing = false;
    dSprite->fps = 12;

    return dSprite;
}

Sprite *dir_sprite_current_sprite(DirectionalSprite *dSprite, v2 spritePos) {
    double player_angle_to_sprite = v2_get_angle(v2_dir(player->pos, spritePos));

    if (dSprite->dirCount < 2) {
        return dSprite->sprites[0];
    } else {

        double relative_angle = rad_to_deg(v2_get_angle(dSprite->dir)) - rad_to_deg(player_angle_to_sprite);

        double min = 9999999;
        int min_idx = -1;
        for (int i = 0; i < dSprite->dirCount; i++) {

            double angle = (360.0 / dSprite->dirCount) * i;
            double angle_dist = angleDist(angle, relative_angle);

            if (angle_dist < min && angle_dist != 0) {
                min = angle_dist;
                min_idx = i;
            }
        }

        return dSprite->sprites[min_idx];
    }

    return NULL;
}

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, double delta) {

    if (dSprite == NULL) return;

    for (int i = 0; i < dSprite->dirCount; i++) {

        Animation *anim = &(dSprite->sprites[i]->animations[dSprite->current_anim]);
        if (anim == NULL) {
            continue;
        }
        anim->fps = dSprite->fps;
        anim->playing = dSprite->playing;
        animationTick(anim, delta);
    }

    // spriteTick(dir_sprite_current_sprite(dSprite, spritePos), delta);
}

double angleDist(double a1, double a2) {
    double l1 = loop_clamp(a1, 0, 360);
    double l2 = loop_clamp(a2, 0, 360);

    double max_angle = max(l1, l2);
    double min_angle = min(l1, l2);

    double d1 = fabs(l2 - l1);
    double d2 = min_angle + (360 - max_angle);

    return min(d1, d2);
}

Bullet *createDefaultBullet(v2 pos, v2 dir) {
    Bullet *bullet = malloc(sizeof(Bullet));

    bullet->entity.pos = pos;
    bullet->entity.size = to_vec(8000);
    bullet->entity.sprite = createSprite(true, 1);
    bullet->entity.sprite->animations[0] = create_animation(4, 0, shooter_bullet_default_frames);
    bullet->entity.sprite->animations[0].fps = 12;
    bullet->entity.sprite->animations[0].loop = true;
    spritePlayAnim(bullet->entity.sprite, 0);
    bullet->entity.height = get_max_height() * 0.525;
    bullet->entity.affected_by_light = false;
    bullet->dirSprite = NULL;
    bullet->dmg = 1;
    bullet->speed = 360;
    bullet->dir = dir;
    bullet->lifeTime = 5;
    bullet->lifeTimer = bullet->lifeTime;

    bullet->on_hit = shooter_bullet_effect;
    
    bullet->hit_player = true;
    bullet->hit_enemies = false;

    bullet->collider = malloc(sizeof(CircleCollider));
    bullet->collider->pos = bullet->entity.pos;
    bullet->collider->radius = 5;



    bullet->particle_spawner = malloc(sizeof(ParticleSpawner));
    *bullet->particle_spawner = create_particle_spawner(V2_ZERO, get_max_height() / 2);

    bullet->particle_spawner->height_dir = 0;
    bullet->particle_spawner->dir = V2_ZERO;
    
    bullet->particle_spawner->height_accel = -PARTICLE_GRAVITY / 5;
    

    bullet->particle_spawner->target = bullet;
    bullet->particle_spawner->active = true;

    add_game_object(bullet->particle_spawner, PARTICLE_SPAWNER);

    return bullet;
}

Bullet *createTestBullet(v2 pos) { return createDefaultBullet(pos, (v2){1, 0}); }

bool intersectCircles(CircleCollider c1, CircleCollider c2) {
    return v2_distance_squared(c1.pos, c2.pos) < (c1.radius + c2.radius) * (c1.radius + c2.radius);  // dist^2 < (r1 + r2)^2
}

void bulletTick(Bullet *bullet, double delta) {

    bullet->entity.pos = v2_add(bullet->entity.pos, v2_mul(bullet->dir, to_vec(bullet->speed * delta)));
    bullet->collider->pos = bullet->entity.pos;

    bullet->lifeTimer -= delta;



    if (bullet->lifeTimer <= 0) {
        enemy_bullet_destroy(bullet);
        return;
    }

    CollisionData bullet_coldata = getCircleTileMapCollision(*bullet->collider);
    if (bullet_coldata.didCollide) {
        enemy_bullet_destroy(bullet);
        return;
    }

    if (bullet->hit_player && intersectCircles(*bullet->collider, *(player->collider))) {
        player_take_dmg(bullet->dmg);
        enemy_bullet_destroy(bullet);
        return;
    }

    if (bullet->dirSprite != NULL) {
        dSpriteTick(bullet->dirSprite, bullet->entity.pos, delta);
    } else {
        spriteTick(bullet->entity.sprite, delta);
    }
}

bool pos_in_tile(v2 pos) {
    int row = pos.y / tileSize;
    int col = pos.x / tileSize;

    if (!in_range(row, 0, TILEMAP_HEIGHT - 1) || !in_range(col, 0, TILEMAP_WIDTH - 1)) return false;

    return levelTileMap[row][col] == P_WALL;
}

CollisionData getCircleTileCollision(CircleCollider circle, v2 tilePos) {
    CollisionData result;
    result.didCollide = false;
    result.offset = (v2){0, 0};

    v2 clampedPos;
    clampedPos.x = clamp(circle.pos.x, tilePos.x, tilePos.x + tileSize);
    clampedPos.y = clamp(circle.pos.y, tilePos.y, tilePos.y + tileSize);

    v2 toClamped = v2_sub(clampedPos, circle.pos);
    double dist = v2_length(toClamped);
    if (dist == 0) {
        return result;
    }
    double overlap = circle.radius - dist;
    if (overlap == 0) {
        return result;
    }

    v2 dirToClamped = v2_div(toClamped, to_vec(dist));
    if (overlap > 0) {
        result.didCollide = true;
        result.offset = v2_mul(dirToClamped, to_vec(-overlap));
    }

    return result;
}

CollisionData getCircleTileMapCollision(CircleCollider circle) {
    CollisionData result;
    result.didCollide = false;
    result.offset = (v2){0, 0};

    v2 gridCheckStart;
    v2 gridCheckEnd;

    gridCheckStart.x = (int)((circle.pos.x - circle.radius) / tileSize) - 1;
    gridCheckStart.y = (int)((circle.pos.y - circle.radius) / tileSize) - 1;

    gridCheckEnd.x = (int)((circle.pos.x + circle.radius) / tileSize) + 2;  //+ 2 to account for rounding up
    gridCheckEnd.y = (int)((circle.pos.y + circle.radius) / tileSize) + 2;

    v2 gridCheckSize = v2_sub(gridCheckEnd, gridCheckStart);

    for (int row = gridCheckStart.y; row < gridCheckEnd.y; row++) {
        for (int col = gridCheckStart.x; col < gridCheckEnd.x; col++) {
            if (!in_range(row, 0, TILEMAP_HEIGHT - 1) || !in_range(col, 0, TILEMAP_WIDTH - 1)) continue;
            if (levelTileMap[row][col] == -1) continue;
            CollisionData data = getCircleTileCollision(circle, (v2){col * tileSize, row * tileSize});
            if (data.didCollide) {
                result.didCollide = true;
                result.offset = v2_add(result.offset, data.offset);
            }
        }
    }

    return result;
}

void update_entity_collisions(void *val, int type) {
    switch (type) {
        case (int)PROJECTILE:;
            Projectile *projectile = val;
            CollisionData coll_data = getCircleTileMapCollision(projectile->collider);
            if (coll_data.didCollide) {
                projectile->entity.pos = v2_add(projectile->entity.pos, coll_data.offset);
                projectile->vel = v2_reflect(projectile->vel, v2_normalize(coll_data.offset));
                projectile->vel = v2_mul(to_vec(projectile->bounciness), projectile->vel);
            }
    }
}

bool isValidLevel(char *file) {
    if (file == NULL) return false;

    char *fileExtension = ".hclevel";

    int fileLen = strlen(file);
    int extLen = strlen(fileExtension);
    if (strcmp(&file[fileLen - extLen], fileExtension) == 0) {
        return true;
    }

    return false;
}

// Takes a file name with no extension and assumes it's a png
void getTextureFiles(char *fileName, int fileCount, GPU_Image ***textures) {
    int charCount = get_num_digits(fileCount);

    for (int i = 0; i < fileCount; i++) {
        char num[charCount + 10];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(fileName, num);
        char *fileWithExtension = concat(fileWithNum, ".png");

        GPU_Image *tex = load_texture(fileWithExtension);
        (*textures)[i] = tex;

        free(fileWithNum);
        free(fileWithExtension);
    }
}

void update_fullscreen() { // iffy solution but whatever
    GPU_SetFullscreen(fullscreen, true);
    // UI_set_fullscreen(fullscreen);
}

BakedLightColor _lerp_baked_light_color(BakedLightColor a, BakedLightColor b, double w) {
    return (BakedLightColor) {
        lerp(a.r, b.r, w),
        lerp(a.g, b.g, w),
        lerp(a.b, b.b, w)
    };
}

void bake_lights() {
   
    bool has_lights = false;

    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);

        if (object->type == LIGHT_POINT) {
            has_lights = true;
            break;
        }
    }

    if (!has_lights) {
        return;
    }



    init_loading_screen();

    const int CALC_RES = BAKED_LIGHT_CALC_RESOLUTION; // directly affects performance!

    
    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {

            if (r % 100 == 0 && c == 0) {
                double max_progress = 0.1;
                double progress = (double)r / (TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION);
                update_loading_progress(0.8 + progress * max_progress);
            }

            baked_light_grid[r][c] = (BakedLightColor){ambient_light, ambient_light, ambient_light};

            
            int tilemap_row = r / BAKED_LIGHT_RESOLUTION;
            int tilemap_col = c / BAKED_LIGHT_RESOLUTION;

            bool is_in_wall = in_range(tilemap_row, 0, TILEMAP_HEIGHT - 1)
            && in_range(tilemap_col, 0, TILEMAP_WIDTH - 1)
            && levelTileMap[tilemap_row][tilemap_col] == P_WALL;

            if (is_in_wall) continue;

            int calc_row = ((int)(r * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;
            int calc_col = ((int)(c * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;

            bool should_calculate = r == calc_row && c == calc_col;

            if (!should_calculate) {
                baked_light_grid[r][c] = baked_light_grid[calc_row][calc_col];
                continue;
            }

            const int calc_tile_size = BAKED_LIGHT_RESOLUTION / CALC_RES;

            

            for (int i = 0; i < gameobjects->length; i++) {
                obj *current = arraylist_get(gameobjects, i);
                if (current->type != LIGHT_POINT) continue;
                
                LightPoint *point = current->val;

                v2 current_pos = v2_mul(v2_div((v2){c, r}, to_vec(BAKED_LIGHT_RESOLUTION)), to_vec(tileSize));
                if (abs(current_pos.x - point->pos.x) > point->radius || abs(current_pos.y - point->pos.y) > point->radius) continue;

                double dist_to_point = v2_distance(point->pos, current_pos);

                if (dist_to_point > point->radius) continue;

                v2 dir = v2_div(v2_sub(point->pos, current_pos), to_vec(dist_to_point));

				RayCollisionData data = castRay(current_pos, dir);

                BakedLightColor col = {0, 0, 0};

				if (!data.hit) {
					double s = clamp(lerp(1, 0, dist_to_point / point->radius), 0, 1);
                    s *= s * s; // cubic
                    double helper = s * point->strength;
					col.r += helper * (double)point->color.r / 255;
					col.g += helper * (double)point->color.g / 255;
					col.b += helper * (double)point->color.b / 255;
				} else {
                    double dist_squared = v2_distance_squared(data.collpos, current_pos);

                    if (dist_squared <= dist_to_point * dist_to_point) continue;

                    double s = clamp(lerp(1, 0, dist_to_point / point->radius), 0, 1);
                    s *= s * s; // cubic
                    double helper = s * point->strength;
                    col.r += helper * (double)point->color.r / 255;
					col.g += helper * (double)point->color.g / 255;
					col.b += helper * (double)point->color.b / 255;
                }

                baked_light_grid[r][c].r = SDL_clamp(baked_light_grid[r][c].r + col.r, 0, MAX_LIGHT);
                baked_light_grid[r][c].g = SDL_clamp(baked_light_grid[r][c].g + col.g, 0, MAX_LIGHT);
                baked_light_grid[r][c].b = SDL_clamp(baked_light_grid[r][c].b + col.b, 0, MAX_LIGHT);

            }
        }
    }  
    

    int box_size_x = 20; // doesn't affect performance anymore! go crazy
    int box_size_y = 20;

    // first apply horizontal blur without filling calc pixels, then vertical blur with filling calc pixels and we're golden

    // horizontal

    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        
        BakedLightColor current_sum = {-1, -1, -1};
        
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {

            int calc_row = ((int)(r * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;
            int calc_col = ((int)(c * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;

            if (c == calc_col && r == calc_row) continue;

            int left = max(c - box_size_x / 2, 0);
            int right = min(c + box_size_x / 2, TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION - 1);

            int count = right - left;

            if (current_sum.r == -1) {

                current_sum = (BakedLightColor){0, 0, 0};

                for (int bc = left; bc < right; bc++) {
                    current_sum = (BakedLightColor) {
                        current_sum.r + baked_light_grid[r][bc].r,
                        current_sum.g + baked_light_grid[r][bc].g,
                        current_sum.b + baked_light_grid[r][bc].b
                    };
                }   

            } else {
                current_sum = (BakedLightColor) {
                    current_sum.r - baked_light_grid[r][left - 1].r + baked_light_grid[r][right].r,
                    current_sum.g - baked_light_grid[r][left - 1].g + baked_light_grid[r][right].g,
                    current_sum.b - baked_light_grid[r][left - 1].b + baked_light_grid[r][right].b
                };
            }

            baked_light_grid[r][c] = (BakedLightColor){current_sum.r / count, current_sum.g / count, current_sum.b / count};

        }
    }
    update_loading_progress(0.95);


    // vertical

    for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {
        
        BakedLightColor current_sum = {-1, -1, -1};
        
        for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {

            int top = max(0, r - box_size_y / 2);
            int bottom = min(TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION - 1, r + box_size_y / 2);

            int count = bottom - top;

            if (current_sum.r == -1) {

                current_sum = (BakedLightColor){0, 0, 0};

                for (int br = top; br < bottom; br++) {
                    current_sum = (BakedLightColor) {
                        current_sum.r + baked_light_grid[br][c].r,
                        current_sum.g + baked_light_grid[br][c].g,
                        current_sum.b + baked_light_grid[br][c].b
                    };
                }
            } else {
                current_sum = (BakedLightColor) {
                    current_sum.r - baked_light_grid[top][c].r + baked_light_grid[bottom][c].r,
                    current_sum.g - baked_light_grid[top][c].g + baked_light_grid[bottom][c].g,
                    current_sum.b - baked_light_grid[top][c].b + baked_light_grid[bottom][c].b
                };
            }

            baked_light_grid[r][c] = (BakedLightColor) {current_sum.r / count, current_sum.g / count, current_sum.b / count};

        }
    }

    lightmap_image = GPU_CreateImage(BAKED_LIGHT_RESOLUTION * TILEMAP_WIDTH, BAKED_LIGHT_RESOLUTION * TILEMAP_HEIGHT, GPU_FORMAT_RGBA);
    GPU_Target *image_target = GPU_LoadTarget(lightmap_image);

    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {
            BakedLightColor color = baked_light_grid[r][c];

            double multiplier = 255.0 / 5;

            SDL_Color tex_color = {
                SDL_clamp(color.r * multiplier, 0, 255),
                SDL_clamp(color.g * multiplier, 0, 255),
                SDL_clamp(color.b * multiplier, 0, 255),
                255
            };
            GPU_Pixel(image_target, c, r, tex_color);
        }
    }

    GPU_FreeTarget(image_target);

    update_loading_progress(1);

    remove_loading_screen();
}

double get_max_height() {
    double fov_factor = tanHalfStartFOV / tanHalfFOV;
    return (WALL_HEIGHT * WALL_HEIGHT_MULTIPLIER * WINDOW_HEIGHT) * fov_factor;
}

void reset_level() {

    init_loading_screen();
    generate_dungeon();
    update_loading_progress(0.1);
    load_dungeon();
}

void player_take_dmg(double dmg) {

    player->health -= dmg;

    // play some effect or animation
    play_sound(player_default_hurt, 0.5);
    shakeCamera(20, 10, true, 2);

    player_hit_particles->pos = player->pos;
    player_hit_particles->height = player->height;
    player_hit_particles->target = NULL;
    particle_spawner_explode(player_hit_particles);

    double progress_to_death = (double)(player->maxHealth - player->health) / player->maxHealth; // when it reaches 1, youre cooked
    vignette_color = (SDL_Color){255 * progress_to_death, 0, 0};

    if (player->health <= 0) {
        player_die();
    }
}

void player_die() {
    // play some dramatic ahh animation
    queued_player_death = true;
}

void _player_die() {
    
    player->pos = spawn_point;
    player->health = player->maxHealth;

}

void init_loading_screen() {
    
    is_loading = true;
    loading_progress = 0;

    GPU_Clear(actual_screen);

    const v2 bar_container_size = {
        200,
        50
    };

    GPU_Rect bar_container = {
        WINDOW_WIDTH / 2 - bar_container_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_container_size.y / 2,
        bar_container_size.x,
        bar_container_size.y
    };

    const v2 bar_size = {
        190,
        40
    };

    GPU_Rect bar_background = {
        WINDOW_WIDTH / 2 - bar_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_size.y / 2,
        bar_size.x,
        bar_size.y
    };

    GPU_RectangleFilled2(actual_screen, bar_container, GPU_MakeColor(255, 255, 255, 255));

    GPU_RectangleFilled2(actual_screen, bar_background, GPU_MakeColor(0, 0, 0, 255));

    GPU_Flip(actual_screen);

}

void update_loading_progress(double progress) {

    GPU_Clear(actual_screen);

    const v2 bar_container_size = {
        200,
        50
    };

    GPU_Rect bar_container = {
        WINDOW_WIDTH / 2 - bar_container_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_container_size.y / 2,
        bar_container_size.x,
        bar_container_size.y
    };

    v2 bar_bg_size = {
        190,
        40
    };

    GPU_Rect bar_background = {
        WINDOW_WIDTH / 2 - bar_bg_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_bg_size.y / 2,
        bar_bg_size.x,
        bar_bg_size.y
    };

    v2 bar_size = v2_sub(bar_bg_size, to_vec(10));

    GPU_Rect bar = {
        WINDOW_WIDTH / 2 - bar_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_size.y / 2,
        bar_size.x * progress,
        bar_size.y
    };
    bar.w = bar_size.x * progress;

    GPU_RectangleFilled2(actual_screen, bar_container, GPU_MakeColor(255, 255, 255, 255));

    GPU_RectangleFilled2(actual_screen, bar_background, GPU_MakeColor(0, 0, 0, 255));

    GPU_RectangleFilled2(actual_screen, bar, GPU_MakeColor(255, 255, 255, 255));

    GPU_Flip(actual_screen);

}

void remove_loading_screen() {
    is_loading = false;
}


SDL_Color lerp_color(SDL_Color col1, SDL_Color col2, double w) {
    SDL_Color res = {
        lerp(col1.r, col2.r, w),
        lerp(col1.g, col2.g, w),
        lerp(col1.b, col2.b, w)
    };

    return res;
}

void shooter_bullet_effect(Bullet *bullet) {
    Sprite *sprite = createSprite(true, 1);
    sprite->animations[0] = create_animation(4, 0, shooter_bullet_explode_frames);
    sprite->animations[0].playing = true;
    sprite->animations[0].fps = 12;
    sprite->animations[0].loop = false;

    v2 pos = v2_sub(bullet->entity.pos, v2_mul(bullet->dir, to_vec(5)));

    Effect *effect = createEffect(pos, bullet->entity.size, sprite, 0.35);
    effect->entity.height = bullet->entity.height;
    effect->entity.affected_by_light = false;

    add_game_object(effect, EFFECT);
}

void enemy_bullet_destroy(Bullet *bullet) {
   
   if (bullet->on_hit != NULL) {
        bullet->on_hit(bullet);
   }

    remove_game_object(bullet, BULLET); 
}

int charge_time_to_shots(double charge_time) {
    return (int)(charge_time * 3);
}

void dir_sprite_play_anim(DirectionalSprite *dir_sprite, int anim) {
    if (dir_sprite->current_anim == anim) return;
    
    for (int i = 0; i < dir_sprite->dirCount; i++) {
        dir_sprite->sprites[i]->animations[anim].frame = 0;
        dir_sprite->sprites[i]->currentAnimationIdx = anim;
    }

    dir_sprite->current_anim = anim;
    dir_sprite->playing = true;
}


Ability ability_primary_shoot_create() {

    if (shoot_icon == NULL) {
        printf("Shoot icon is null at primary shoot create. \n");
    } else {
        printf("It's not null. \n");
    }

    return (Ability) {
        .activate = ability_shoot_activate,
        .tick = default_ability_tick,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = 0.2,
        .timer = 0.2,
        .type = A_PRIMARY,
        .texture = shoot_icon,
        .delay = 0,
        .delay_timer = -1000
    };
}

void default_ability_tick(Ability *ability, double delta) {
    if (!ability->can_use) {
        ability->timer -= delta;
        if (ability->timer <= 0) {
            ability->timer = ability->cooldown;
            ability->can_use = true;
        }
    }

    bool waiting_delay = ability->delay_timer > -999;
    if (waiting_delay) {
        ability->delay_timer -= delta;
        if (ability->delay_timer <= 0) {
            ability->delay_timer = -1000;
            ability->activate(ability);
        }
    }
}

Ability ability_secondary_shoot_create() {
    return (Ability) {
        .activate = ability_secondary_shoot_activate,
        .tick = default_ability_tick,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = 5,
        .delay = 0,
        .delay_timer = -1000,
        .timer = 0,
        .type = A_SECONDARY,
        .texture = shotgun_icon,
        // .shot_amount = 6,
        // .shot_timer = 0.06,
        // .shots_left = 0
    };
}

void ability_secondary_shoot_activate(Ability *ability) {

    shakeCamera(35, 15, true, 10);
    play_sound(rapidfire_sound, 0.5);

    Ability *rapid_fire = (Ability *)ability;
    int shots = 6;


    while (shots-- > 0) _shoot(0.05);
    

    player->height_vel = -1;
    player->vel = v2_mul(playerForward, to_vec(-5));
}

void _shoot(double spread) { // the sound isnt attached bc shotgun makes eargasm

    double pitch = player->pitch + randf_range(1000 * -spread, 1000 * spread);

    
    shakeCamera(10, 4, true, 1);
    spritePlayAnim(leftHandSprite, 1);

    v2 shoot_dir = v2_rotate(playerForward, randf_range(-PI * spread, PI * spread));

    v2 effect_size = to_vec(8000);

    RayCollisionData ray_data = castRayForAll(player->pos, shoot_dir);

    v2 hit_pos = screenToFloor((v2){RESOLUTION_X / 2, WINDOW_HEIGHT / 2 + pitch});
    double distance_to_hit_pos = v2_distance(hit_pos, player->pos);
    double distance_to_coll_pos = ray_data.hit? v2_distance(ray_data.collpos, player->pos) : 999999999999;

    v2 final_pos;
    bool is_ceiling = pitch < 0;
    double final_height = is_ceiling? get_max_height() * 1.5 : get_max_height() * 0.5;


    if (distance_to_coll_pos < distance_to_hit_pos) {
        final_pos = ray_data.collpos;
        final_height = lerp( get_max_height() * 0.5 + get_player_height(), final_height, inverse_lerp(0, distance_to_hit_pos, distance_to_coll_pos));
    } else {
        final_pos = hit_pos;
    }

    double size = 8000;
    Effect *hitEffect = createEffect(final_pos, to_vec(size), createSprite(true, 1), 1);
    hitEffect->entity.height = final_height - size / 2;

    hitEffect->entity.sprite->animations[0] = create_animation(5, 0, shootHitEffectFrames);
    
    hitEffect->entity.sprite->animations[0].fps = 12;
    hitEffect->entity.sprite->animations[0].loop = false;
    hitEffect->entity.affected_by_light = false;

    spritePlayAnim(hitEffect->entity.sprite, 0);

    add_game_object(hitEffect, EFFECT);

    
    struct ability_shoot_packet packet_data = {
        .hit_pos = final_pos,
        .hit_height = final_height,
        .shooter_id = client_self_id,
        .hit_id = -1
    };
    if (ray_data.hit) {
        if (ray_data.colliderType == PLAYER_ENTITY) {

            PlayerEntity *player_entity = ray_data.collider;

            
            packet_data.hit_id = player_entity->id;
            
        }
    }

    MPPacket packet = {.is_broadcast = true, .len = sizeof(packet_data), .type = PACKET_ABILITY_SHOOT};

    MPClient_send(packet, &packet_data);








}

void draw_3d_line(v2 pos1, double h1, v2 pos2, double h2) {
    v2 screen_pos_1 = worldToScreen(pos1, h1, true);
    v2 screen_pos_2 = worldToScreen(pos2, h2, true);

    // SDL_RenderDrawLine(renderer, screen_pos_1.x, screen_pos_1.y, screen_pos_2.x, screen_pos_2.y);
}

ParticleSpawner create_particle_spawner(v2 pos, double height) {
    ParticleSpawner spawner = {
        .dir = (v2){1, 0},
        .height_dir = 0,
        .accel = (v2){0, 0},
        .height_accel = PARTICLE_GRAVITY,
        .pos = pos,
        .height = height,
        .spawn_rate = 20,
        .spread = 0.1,
        .min_speed = 10,
        .max_speed = 10,
        .min_size = to_vec(500),
        .max_size = to_vec(1500),
        .floor_drag = 0.01,
        .bounciness = 0.5,
        .particle_lifetime = 1,
        .active = true,
        .target = NULL,
        .fade = true,
        .fade_scale = true,
        .affected_by_light = true,
        .explode_particle_amount = 20
    };

    Sprite sprite;
    sprite.animCount = 0;
    sprite.animations = NULL;
    sprite.currentAnimationIdx = 0;
    sprite.isAnimated = false;
    sprite.texture = default_particle_texture;

    spawner.sprite = sprite;

    spawner.target = NULL;

    spawner.spawn_timer = 1.0 / spawner.spawn_rate;

    return spawner;
}

void particle_spawner_tick(ParticleSpawner *spawner, double delta) {

    if (!spawner->active) return;
    spawner->spawn_timer -= delta;
    if (spawner->spawn_timer <= 0) {
        spawner->spawn_timer = 1.0 / spawner->spawn_rate;
        particle_spawner_spawn(spawner);
    }
}

void particle_spawner_spawn(ParticleSpawner *spawner) {

    Particle particle;
    
    v2 pos = spawner->pos;
    double height = spawner->height;
    if (spawner->target != NULL) {
        pos = v2_add(pos, spawner->target->pos);
        height += spawner->target->height;
    }

    double speed = randf_range(spawner->min_speed, spawner->max_speed);
    v2 dir = v2_rotate(spawner->dir, randf_range(-PI * spawner->spread, PI * spawner->spread));
    double height_dir = spawner->height_dir * randf_range(-1, 1);
    
    double vel_x = dir.x * speed;
    double vel_y = dir.y * speed;
    double vel_z = height_dir * speed * 3000; // bc height is different like that

    particle.vel = (v2){vel_x, vel_y};
    particle.h_vel = vel_z;

    particle.effect.entity.affected_by_light = spawner->affected_by_light;

    particle.accel = spawner->accel;
    particle.h_accel = spawner->height_accel;

    particle.effect.life_time = spawner->particle_lifetime;
    particle.effect.life_timer = particle.effect.life_time;

    particle.start_color = spawner->start_color;
    particle.end_color = spawner->end_color;

    particle.bounciness = spawner->bounciness;
    particle.floor_drag = spawner->floor_drag;

    particle.effect.entity.pos = pos; // change later with emission
    particle.effect.entity.height = spawner->height;
    
    particle.fade = spawner->fade;
    particle.fade_scale = spawner->fade_scale;

    double size_rand = randf_range(0, 1);
    v2 size = v2_lerp(spawner->min_size, spawner->max_size, size_rand);
    
    particle.initial_size = size;

    particle.effect.entity.sprite = malloc(sizeof(Sprite));
    *particle.effect.entity.sprite = spawner->sprite;


    Particle *particle_object = malloc(sizeof(Particle));
    *particle_object = particle;

    if (particle.effect.entity.sprite->texture == NULL) {
        particle.effect.entity.sprite->texture = default_particle_texture;
        particle.effect.entity.sprite->isAnimated = false;
        printf("Error! switching to default texture! \n");
    }

    add_game_object(particle_object, PARTICLE);

}


void particle_tick(Particle *particle, double delta) {

    SDL_Color current_color;

    double prog = inverse_lerp(particle->effect.life_time, 0, particle->effect.life_timer);

    int r = lerp(particle->start_color.r, particle->end_color.r, prog);
    int g = lerp(particle->start_color.g, particle->end_color.g, prog);
    int b = lerp(particle->start_color.b, particle->end_color.b, prog);
    int a = lerp(particle->start_color.a, particle->end_color.a, prog);

    if (particle->end_color.a == 0) {
        current_color = particle->start_color;
        current_color.a = a;
    } else {
        current_color = (SDL_Color){r, g, b, a};
    }

    particle->effect.entity.color = current_color;

    particle->vel = v2_add(particle->vel, v2_mul(particle->accel, to_vec(delta)));
    particle->h_vel += particle->h_accel * delta;

    particle->effect.entity.size = v2_lerp(particle->initial_size, V2_ZERO, inverse_lerp(particle->effect.life_time, 0, particle->effect.life_timer));
    particle->effect.entity.pos = v2_add(particle->effect.entity.pos, v2_mul(particle->vel, to_vec(delta)));
    particle->effect.entity.height += particle->h_vel * delta;
    double floor_bound = get_max_height() / 2 + particle->effect.entity.size.y / 2;
    double ceil_bound = get_max_height() / 2 + get_max_height() - particle->effect.entity.size.y / 2;
    if (particle->effect.entity.height < floor_bound) {
        particle->effect.entity.height = floor_bound;
        particle->h_vel *= -particle->bounciness;
        particle->vel = v2_mul(particle->vel, to_vec(1 - particle->floor_drag));
    }
    if (particle->effect.entity.height > ceil_bound) {
        particle->effect.entity.height = ceil_bound;
        particle->h_vel *= -particle->bounciness;
    }

    effectTick((Effect *)particle, delta);
}

Ability ability_dash_create() {
    return (Ability) {
        .activate = ability_dash_activate,
        .tick = default_ability_tick,
        .before_activate = ability_dash_before_activate,
        .can_use = true,
        .cooldown = 3,
        .timer = 3,
        .type = A_UTILITY,
        .texture = dash_icon,
        .delay = 0.1,
        .delay_timer = -1000
    };
}

// void ability_dash_tick(Ability *ability, double delta) {

// }

void ability_dash_activate(Ability *ability) {

    const double MAX_DASH_STR = 100;

    v2 pos = player->pos;
    v2 dir = playerForward;
    double dash_distance = MAX_DASH_STR;


    // player->vel = v2_mul(dir, to_vec(dash_strength));
    shakeCamera(15, 20, true, 10);


    vignette_color = (SDL_Color){50, 150, 255, 165};
    //fov = startFov * 1.3;


    RayCollisionData ray = castRay(pos, dir);

    

    if (ray.hit) {
        double dist_sqr = v2_distance_squared(ray.startpos, ray.collpos);
        if (dist_sqr < dash_distance * dash_distance) {
            dash_distance = sqrt(dist_sqr) - 1;
        }
    }

    player->pos = v2_add(player->pos, v2_mul(dir, to_vec(dash_distance)));
    player->height_vel = -2;
    player->vel = v2_mul(dir, to_vec(3));

}

void activate_ability(Ability *ability) {
    
    if (ability == NULL || !ability->can_use || paused) return;


    ability->can_use = false;

    if (ability->before_activate != NULL) {
        ability->before_activate(ability);
    }

    ability->delay_timer = ability->delay;
}

void ability_dash_before_activate(Ability *ability) {
    spritePlayAnim(dash_anim_sprite, 0);
}



void _continue_button_pressed(UIComponent *comp, bool pressed) {
    toggle_pause();
}


void make_ui() {

    pause_menu = UI_alloc(UIComponent);

    UI_set_size(pause_menu, (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2});
    UI_set_pos(pause_menu, (v2){WINDOW_WIDTH / 4, WINDOW_HEIGHT / 4});

    pause_menu->default_style = (UIStyle){.bg_color = Color(0, 0, 0, 125), .fg_color = Color(255, 255, 255, 255)};
    
    UILabel *paused_label = UI_alloc(UILabel);
    UI_add_child(pause_menu, paused_label);
    
    UILabel_set_text(paused_label, String("Paused!"));
    UILabel_set_alignment(paused_label, ALIGNMENT_CENTER, ALIGNMENT_CENTER);
    UI_set_pos(paused_label, (v2){0, 50});
    UI_set_size(paused_label, (v2){WINDOW_WIDTH / 2, WINDOW_WIDTH / 16});

    UIButton *continue_button = UI_alloc(UIButton);
    UI_add_child(pause_menu, continue_button);
    
    
    UI_set_size(continue_button, v2_div(UI_get_size(paused_label), (v2){2, 1}));
    UI_center_around_pos(continue_button, (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 50});
    UILabel_set_alignment(continue_button, ALIGNMENT_CENTER, ALIGNMENT_CENTER);
    UILabel_set_text(continue_button, String("Continue"));

    continue_button->on_click = _continue_button_pressed;

    UIStyle default_style = (UIStyle){.bg_color = Color(0, 0, 0, 175), .fg_color = Color(255, 255, 255, 255)};

    UI_set(UIComponent, continue_button, default_style, default_style);




    
    pause_menu->visible = false;
    UI_add_child(UI_get_root(), pause_menu);


    debug_label = UI_alloc(UILabel);

    UILabel_set_text(debug_label, StringRef("debug label: stuff"));
    
    debug_label->font_size = 15;

    UI_set_pos(debug_label, V2_ZERO);
    UI_set_size(debug_label, (v2){WINDOW_WIDTH, WINDOW_HEIGHT / 5});
    UI_add_child(UI_get_root(), debug_label);
}


void toggle_pause() {
    paused = !paused;
    lock_and_hide_mouse = !paused;
    pause_menu->visible = paused;
}

void particle_spawner_explode(ParticleSpawner *spawner) {
    for (int i = 0; i < spawner->explode_particle_amount; i++) {
        particle_spawner_spawn(spawner);
    }
}

Room Room_new(v2 pos) {
    Room room;
    room.room_idx = pos;
    room.left = NULL;
    room.right = NULL;
    room.down = NULL;
    room.up = NULL;
    room.is_start = false;
    room.is_boss = false;
    room.room_file_name = String("test_room.hcroom");
    room.initialized = true;


    return room;
}

void generate_room_recursive(Room *room_ptr, bool visited[DUNGEON_SIZE][DUNGEON_SIZE]) {
    visited[(int)room_ptr->room_idx.y][(int)room_ptr->room_idx.x] = true;
    // generated_count++;

    v2 current = room_ptr->room_idx;

    v2 dirs[4] = {V2_LEFT, V2_RIGHT, V2_UP, V2_DOWN};
    const int LEFT = 0;
    const int RIGHT = 1;
    const int UP = 2;
    const int DOWN = 3;
    int dir_indicies[4] = {LEFT, RIGHT, UP, DOWN};

    shuffle_array(dir_indicies, 4);


    for (int i = 0; i < 4; i++) {
        int dir_idx = dir_indicies[i];
        v2 dir = dirs[dir_idx];


        v2 pos = v2_add(current, dir);
        if (!in_range(pos.x, 0, DUNGEON_SIZE - 1) || !in_range(pos.y, 0, DUNGEON_SIZE - 1)) continue;
        if (visited[(int)pos.y][(int)pos.x]) continue;

        rooms[(int)pos.y][(int)pos.x] = Room_new(pos);
        visited[(int)pos.y][(int)pos.x] = true;

        Room *new_ref = &rooms[(int)pos.y][(int)pos.x];
        
        switch (dir_idx) {
            case 0:
                room_ptr->left = new_ref;
                new_ref->right = room_ptr;
                break;
            case 1:
                room_ptr->right = new_ref;
                new_ref->left = room_ptr;
                break;
            case 2:
                room_ptr->up = new_ref;
                new_ref->down = room_ptr;
                break;
            case 3:
                room_ptr->down = new_ref;
                new_ref->up = room_ptr;
                break;
            default:
                printf("Uh oh \n");
                break;
        }

        generate_room_recursive(new_ref, visited);
    }
}

void generate_dungeon() {
    
    srand(client_dungeon_seed);

    bool visited[DUNGEON_SIZE][DUNGEON_SIZE] = {0};


    // for multiplayer testing
    Room start_room = Room_new(V2_ZERO);//Room_new((v2){randi_range(0, DUNGEON_SIZE - 1), randi_range(0, DUNGEON_SIZE - 1)});

    start_room.is_start = true;

    rooms[(int)start_room.room_idx.y][(int)start_room.room_idx.x] = start_room;

    
    generate_room_recursive(&rooms[(int)start_room.room_idx.y][(int)start_room.room_idx.x], visited);

}

void load_room(Room *room_ptr) {

    room_ptr->left_entrance_pos = (v2){(int)ROOM_WIDTH / 2, (int)ROOM_HEIGHT / 2};
    room_ptr->right_entrance_pos = (v2){(int)ROOM_WIDTH / 2, (int)ROOM_HEIGHT / 2};
    room_ptr->top_entrance_pos = (v2){(int)ROOM_WIDTH / 2, (int)ROOM_HEIGHT / 2};
    room_ptr->bottom_entrance_pos = (v2){(int)ROOM_WIDTH / 2, (int)ROOM_HEIGHT / 2};

    Room room = *room_ptr;

    int ROOM_COUNT = 3; // replace with folder counting implementation thing

    int room_idx = randi_range(1, ROOM_COUNT);


    String room_num_str = String_from_int(room_idx);
    String path = String_concat(StringRef("Levels/DungeonRooms/Stage1/"), room_num_str);
    String final = String_concat(path, StringRef(".hcroom"));

    room_ptr->room_file_name = final;

    String_delete(&path);
    String_delete(&room_num_str);

    FILE *fh = fopen(room_ptr->room_file_name.data, "r");
    if (fh == NULL) {
        printf("File doesnt exist. File: '%s' \n", room.room_file_name.data);
        return;
    }


    int data_count = ROOM_HEIGHT * ROOM_WIDTH * 4 + 1; // grr

    int data[data_count];

    fread(data, sizeof(int), data_count, fh);
    int data_ptr = 0;

    int offset_r = room.room_idx.y * ROOM_HEIGHT;
    int offset_c = room.room_idx.x * ROOM_WIDTH;
    

    SaveType save_type = (SaveType)data[data_ptr++];

    if (save_type != ST_ROOM) {
        printf("File is not a room! Not loading that! \n");
        fclose(fh);
        return;
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            floorTileMap[offset_r + r][offset_c + c ] = data[data_ptr++];
            if (floorTileMap[offset_r + r][offset_c + c ] == P_FLOOR_LIGHT) {
                v2 tile_mid = (v2){(offset_c + c + 0.5) * tileSize, (offset_r + r + 0.5) * tileSize};
                spawn_floor_light(tile_mid);
            }
        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {

            int tile = data[data_ptr++];

            if (tile == P_DOOR) {
                v2 entrance_pos = (v2){c, r};
                if (entrance_pos.x > room_ptr->right_entrance_pos.x) {
                    room_ptr->right_entrance_pos = entrance_pos;
                }
                if (entrance_pos.x < room_ptr->left_entrance_pos.x) {
                    room_ptr->left_entrance_pos = entrance_pos;
                }
                if (entrance_pos.y > room_ptr->bottom_entrance_pos.y) {
                    room_ptr->bottom_entrance_pos = entrance_pos;
                }
                if (entrance_pos.y < room_ptr->top_entrance_pos.y) {
                    room_ptr->top_entrance_pos = entrance_pos;
                }

                levelTileMap[offset_r + r][offset_c + c] = P_WALL;
            } else {
                levelTileMap[offset_r + r][offset_c + c] = tile;
            }



        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            ceilingTileMap[offset_r + r][offset_c + c ] = data[data_ptr++];


            if (ceilingTileMap[offset_r + r][offset_c + c ] == P_CEILING_LIGHT) {
                v2 tile_mid = (v2){(offset_c + c + 0.5) * tileSize, (offset_r + r + 0.5) * tileSize};
                spawn_ceiling_light(tile_mid);
            }
        }
    }
    
    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            int etype = data[data_ptr++];

            v2 tile_mid = (v2){(offset_c + c + 0.5) * tileSize, (offset_r + r + 0.5) * tileSize};

            if (room.is_start && etype == P_PLAYER) {
                place_entity(tile_mid, P_PLAYER);
            } else if (etype != P_PLAYER && !room.is_start) {
                place_entity(tile_mid, etype);
            }
        }
    }

    fclose(fh);

}

void _carve_path(v2 pos1, v2 pos2, bool vertical) {


    int start_row = pos1.y;
    int start_col = pos1.x;

    int col_dist = abs(pos1.x - pos2.x);
    int row_dist = abs(pos1.y - pos2.y);

    int current_col = start_col;
    int current_row = start_row;
    int dir_c = sign(pos2.x - pos1.x);
    int dir_r = sign(pos2.y - pos1.y);



    if (!vertical) {        

        for (int c = current_col; c != start_col + col_dist / 2; c += dir_c) {
            current_col = c;
            levelTileMap[current_row][current_col] = -1;
        }
        for (int r = current_row; r != start_row + row_dist * dir_r; r += dir_r) {
            current_row = r;
            levelTileMap[current_row][current_col] = -1;
            
        }
        for (int c = current_col; c != start_col + (col_dist + 1) * dir_c; c += dir_c) {
            current_col = c;
            levelTileMap[current_row][current_col] = -1;
        }

    } else {
        for (int r = current_row; r != start_row + row_dist / 2; r += dir_r) {
            current_row = r;
            levelTileMap[current_row][current_col] = -1;
        }
        for (int c = current_col; c != start_col + col_dist * dir_c; c += dir_c) {
            current_col = c;
            levelTileMap[current_row][current_col] = -1;
        }
        for (int r = current_row; r != start_row + (row_dist + 1) * dir_r; r += dir_r) {
            current_row = r;
            levelTileMap[current_row][current_col] = -1;
        }
    }

    levelTileMap[current_row][current_col] = -1;
}

void carve_room_paths() {
    bool visited[DUNGEON_SIZE][DUNGEON_SIZE] = {0};

    for (int r = 0; r < DUNGEON_SIZE; r++) {
        for (int c = 0; c < DUNGEON_SIZE; c++) {
            const Room room = rooms[r][c];
            visited[r][c] = true;
            v2 room_pos = v2_mul(room.room_idx, (v2){ROOM_WIDTH, ROOM_HEIGHT});

            v2 offset = (v2){0, 0};

            if (room.left != NULL && !visited[(int)room.left->room_idx.y][(int)room.left->room_idx.x]) {
                v2 left_room_pos = v2_mul(room.left->room_idx, (v2){ROOM_WIDTH, ROOM_HEIGHT});
                _carve_path(v2_add(room_pos, room.left_entrance_pos), v2_add(left_room_pos, v2_sub(room.left->right_entrance_pos, offset)), false);
            }
            
            if (room.right != NULL && !visited[(int)room.right->room_idx.y][(int)room.right->room_idx.x]) {
                v2 right_room_pos = v2_mul(room.right->room_idx, (v2){ROOM_WIDTH, ROOM_HEIGHT});
                _carve_path(v2_add(room_pos, room.right_entrance_pos), v2_add(right_room_pos, v2_sub(room.right->left_entrance_pos, offset)), false);
            }       

            if (room.up != NULL && !visited[(int)room.up->room_idx.y][(int)room.up->room_idx.x]) {
                v2 top_room_pos = v2_mul(room.up->room_idx, (v2){ROOM_WIDTH, ROOM_HEIGHT});
                _carve_path(v2_add(room_pos, room.top_entrance_pos), v2_add(top_room_pos, v2_sub(room.up->bottom_entrance_pos, offset)), true);
            }       

            if (room.down != NULL && !visited[(int)room.down->room_idx.y][(int)room.down->room_idx.x]) {
                v2 bottom_room_pos = v2_mul(room.down->room_idx, (v2){ROOM_WIDTH, ROOM_HEIGHT});
                _carve_path(v2_add(room_pos, room.bottom_entrance_pos), v2_add(bottom_room_pos, v2_sub(room.down->top_entrance_pos, offset)), true);
            }       

        }
    }
}

void load_dungeon() {

    static int times_called = 0;
    times_called += 1;
    printf("times called: %d \n", times_called);

    clearLevel();

    reset_tilemap(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    reset_tilemap(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    double prog = 0.1;

    for (int r = 0; r < DUNGEON_SIZE; r++) {
        for (int c = 0; c < DUNGEON_SIZE; c++) {

            prog = lerp(0.1, 0.8, (double)(r * DUNGEON_SIZE + c) / (DUNGEON_SIZE * DUNGEON_SIZE));
            update_loading_progress(prog);

            load_room(&rooms[r][c]);

        }    
    }

    carve_room_paths();
    prog += 0.05;
    update_loading_progress(prog);

    tilemap_image = GPU_CreateImage(TILEMAP_WIDTH, TILEMAP_HEIGHT, GPU_FORMAT_RGBA);

    SDL_Surface *surface = GPU_CopySurfaceFromImage(tilemap_image);

    for (int row = 0; row < TILEMAP_HEIGHT; row++) {
        for (int col = 0; col < TILEMAP_WIDTH; col++) {

            int floorTile = floorTileMap[row][col] == -1? 0 : floorTileMap[row][col];
            int ceilingTile = ceilingTileMap[row][col] == -1? 0 : ceilingTileMap[row][col];

            SDL_Color color = {
                SDL_clamp(ceilingTile * 10, 0, 255),
                SDL_clamp(0, 0, 255),
                SDL_clamp(floorTile * 10, 0, 255),
                SDL_clamp(255, 0, 255)
            };

            ((int *)surface->pixels)[row * TILEMAP_WIDTH + col] = color.a << 24 | color.b << 16 | color.g << 8 | color.r;
        }
    }

    GPU_UpdateImage(tilemap_image, NULL, surface, NULL);

    SDL_FreeSurface(surface);


    bake_lights();

    update_loading_progress(1);
    

}

double get_player_height() {
    return player->crouching? player->height - player->tallness / 2 : player->height;
}


double set_time_scale_for_duration(double ts, double duration) {
    game_speed = ts;
    game_speed_duration_timer = duration;
}

bool is_player_on_floor() {
    return player->height <= player->tallness * 0.8;
}

// takes ownership of 'string'
void write_to_debug_label(String string) {
    UILabel_set_text(debug_label, string);
    UILabel_update(debug_label);
}

bool is_entity_type(int type) {
    return in_range(type, ENTITY_START, ENTITY_END);
}

void on_player_connect(SOCKET player_socket) {
    write_to_debug_label(String_from_int(MP_clients_amount));
    int player_id = next_id++;

    server_client_id_list[MP_clients_amount - 1] = player_id;

    char buf[MP_DEFAULT_BUFFER_SIZE] = {0};

    MPPacket packet = {
        .is_broadcast = false,
        .len = sizeof(player_id),
        .type = PACKET_UPDATE_PLAYER_ID
    };

    memcpy(buf, &packet, sizeof(packet));
    memcpy(buf + sizeof(packet), (void *)&player_id, packet.len);

    send(player_socket, buf, sizeof(packet) + packet.len, 0);

    MPServer_send(
        (MPPacket){.type = PACKET_PLAYER_JOINED, .len = sizeof(struct player_joined_packet), .is_broadcast = true}, 
        &player_id
    );

    for (int i = 0; i < MP_clients_amount; i++) {
        printf("Sending to the dude. ID: %d \n", server_client_id_list[i]);

        struct player_joined_packet packet_data = {.id = server_client_id_list[i]};

        MPPacket packet = {.type = PACKET_PLAYER_JOINED, .len = sizeof(struct player_joined_packet), .is_broadcast = false};

        char bruh[MP_DEFAULT_BUFFER_SIZE] = {0};

        memcpy(bruh, &packet, sizeof(packet));
        memcpy(bruh + sizeof(packet), &packet_data, packet.len);

        send(player_socket, bruh, sizeof(packet) + packet.len, 0);
    }


    MPPacket seed_packet = {
        .is_broadcast = false,
        .len = sizeof(struct dungeon_seed_packet),
        .type = PACKET_DUNGEON_SEED
    };

    if (server_dungeon_seed == -1) {
        server_dungeon_seed = rand();
    }

    struct dungeon_seed_packet seed_packet_data = {
        .seed = server_dungeon_seed
    };

    MPServer_send_to(seed_packet, &seed_packet_data, player_socket);
    printf("Sent seed to client. \n");
}

void on_player_disconnect(SOCKET player_socket) {

}

void on_server_recv(SOCKET socket, MPPacket packet, void *data) {

    if (packet.type == PACKET_REQUEST_DUNGEON_SEED) {
        struct dungeon_seed_packet packet_data = {.seed = server_dungeon_seed};
        MPPacket seed_packet = {.type = PACKET_DUNGEON_SEED, .len = sizeof(packet_data), .is_broadcast = false};

        MPServer_send_to(seed_packet, &packet_data, socket);
        return;
    }

    MPServer_send(packet, data);
}

void client_add_player_entity(int id) {
    PlayerEntity *player_entity = malloc(sizeof(PlayerEntity));

    player_entity->id = id;
    player_entity->entity.affected_by_light = true;
    player_entity->entity.height = 0;
    player_entity->entity.pos = V2_ZERO;
    player_entity->entity.size = to_vec(10000);
    player_entity->entity.sprite = createSprite(false, 0);
    player_entity->entity.sprite->texture = entityTexture;
    player_entity->entity.color = (SDL_Color){255, 255, 255,  255};
    player_entity->desired_pos = V2_ZERO;
    player_entity->desired_height = 0;
    player_entity->collider = (CircleCollider){.radius = PLAYER_COLLIDER_RADIUS, .pos = V2_ZERO};

    player_entity->direction_indicator = malloc(sizeof(Entity));
    player_entity->direction_indicator->affected_by_light = true;
    player_entity->direction_indicator->color = GPU_MakeColor(255, 255, 255, 255);
    player_entity->direction_indicator->sprite = createSprite(false, 0);
    player_entity->direction_indicator->sprite->texture = default_particle_texture;
    player_entity->direction_indicator->height = get_max_height() / 2 + player->tallness / 2;
    player_entity->direction_indicator->pos = V2_ZERO;
    player_entity->direction_indicator->size = to_vec(1600);
    

    add_game_object(player_entity->direction_indicator, ENTITY);

    printf("Player joined! ID: %d \n", player_entity->id);
    write_to_debug_label(String_from_int(player_entity->id));

    add_game_object(player_entity, PLAYER_ENTITY);
}

// #CLIENT RECV
void on_client_recv(MPPacket packet, void *data) {

    if (packet.type == PACKET_UPDATE_PLAYER_ID) {

        client_self_id = *(int *)(data);

    } else if (packet.type == PACKET_PLAYER_POS) {
        
        struct player_pos_packet packet_data = *(struct player_pos_packet *)data;

        if (packet_data.id == client_self_id) {
            return;
        }

        PlayerEntity *player_entity = find_or_add_player_entity_by_id(packet_data.id);

        player_entity->desired_pos = packet_data.pos;
        player_entity->desired_height = packet_data.height;
        player_entity->dir = packet_data.dir;
        player_entity->entity.color = packet_data.color;
    
    } else if (packet.type == PACKET_DUNGEON_SEED) {

        if (client_dungeon_seed != -1) {
            return;
        }

        printf("Received dungeon seed. \n");

        struct dungeon_seed_packet *packet_data = data;

        client_dungeon_seed = packet_data->seed;

        loading_map = true;

    } else if (packet.type == PACKET_ABILITY_SHOOT) {
        struct ability_shoot_packet *packet_data = data;
        
        if (client_self_id == packet_data->shooter_id) {
            
            if (packet_data->hit_id != -1) {
                player_entity_take_dmg(find_or_add_player_entity_by_id(packet_data->hit_id), 1);
            }
            
            return;
        }
            
            

        // create the effect at the pos and height
        Effect *hitEffect = createEffect(packet_data->hit_pos, to_vec(8000), createSprite(true, 1), 1);

        if (hitEffect == NULL) {
            printf("hit effect is null \n");
            return;
        }

        hitEffect->entity.sprite->animations[0] = create_animation(5, 0, shootHitEffectFrames);
        
        hitEffect->entity.sprite->animations[0].fps = 12;
        hitEffect->entity.sprite->animations[0].loop = false;
        hitEffect->entity.height = packet_data->hit_height;
        hitEffect->entity.affected_by_light = false;

        spritePlayAnim(hitEffect->entity.sprite, 0);

        add_game_object(hitEffect, EFFECT);


        if (packet_data->hit_id != -1) {

            if (packet_data->hit_id == client_self_id) {
                player_take_dmg(1);
                return;
            }

            PlayerEntity *player_entity = find_or_add_player_entity_by_id(packet_data->hit_id);
            player_entity_take_dmg(player_entity, 1);
        }

    } else if (packet.type == PACKET_HOST_LEFT) {
        exit(1);

    } else if (packet.type == PACKET_PLAYER_LEFT) {
        struct player_left_packet *packet_data = data;


        if (packet_data->id == client_self_id) {
            if (MP_is_server) {
                shutdown(MPServer_socket, SD_BOTH);
                closesocket(MPServer_socket);
            }
            shutdown(MPClient_socket, SD_BOTH);
            closesocket(MPClient_socket);
            can_exit = true;
            return;
        }

        for (int i = 0; i < gameobjects->length; i++) {
            obj *object = arraylist_get(gameobjects, i);

            if (object->type != PLAYER_ENTITY) continue;

            PlayerEntity *player_entity = object->val;

            if (player_entity->id == packet_data->id) {
                remove_game_object(player_entity, PLAYER_ENTITY);
                return;
            }
        }
    } else if (packet.type == PACKET_ABILITY_BOMB) {
        struct ability_bomb_packet *packet_data = data;

        if (packet_data->sender_id == client_self_id) return;

        Projectile *bomb = malloc(sizeof(Projectile));
        *bomb = create_bomb_projectile(packet_data->pos, packet_data->vel);
        bomb->entity.height = packet_data->height;
        bomb->height_vel = packet_data->height_vel;

        add_game_object(bomb, PROJECTILE);
    }
}

void player_entity_tick(PlayerEntity *player_entity, double delta) {

    double real_desired_height = player_entity->desired_height + get_max_height() * 0.5 - player_entity->entity.size.y / 2;

    player_entity->entity.pos = v2_lerp(player_entity->entity.pos, player_entity->desired_pos, 0.1 * (delta * 144));
    player_entity->entity.height = lerp(player_entity->entity.height, real_desired_height, 0.1 * (delta * 144));
    player_entity->collider.pos = player_entity->entity.pos;
    player_entity->direction_indicator->pos = v2_add(player_entity->entity.pos, v2_mul(player_entity->dir, to_vec(20)));
}

void init_textures() {
    
    blood_particle = load_texture("Textures/BloodParticle.png");
    
    int bloom_frag = GPU_LoadShader(GPU_FRAGMENT_SHADER, "Shaders/bloom_frag.glsl");
    int bloom_vert = GPU_LoadShader(GPU_VERTEX_SHADER, "Shaders/bloom_vert.glsl");
    
    bloom_shader = GPU_LinkShaders(bloom_frag, bloom_vert);

    bloom_shader_block = GPU_LoadShaderBlock(bloom_shader, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");

    GPU_FreeShader(bloom_frag);
    GPU_FreeShader(bloom_vert);


    tilemap_image = GPU_CreateImage(TILEMAP_WIDTH, TILEMAP_HEIGHT, GPU_FORMAT_RGBA);

    GPU_Target *image_target = GPU_LoadTarget(tilemap_image);

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            SDL_Color color = (SDL_Color){255, 255, 255, 255};
            GPU_RectangleFilled2(image_target, (GPU_Rect){c, r, 1, 1}, color);
        }
    }

    GPU_FreeTarget(image_target);

    floor_and_ceiling_spritesheet = load_texture("Textures/floor_and_ceiling_spritesheet.png");

    int frag = GPU_LoadShader(GPU_FRAGMENT_SHADER, "Shaders/floor_frag.glsl");
    int vert = GPU_LoadShader(GPU_VERTEX_SHADER, "Shaders/floor_vert.glsl");

    floor_shader = GPU_LinkShaders(frag, vert);

    floor_shader_block = GPU_LoadShaderBlock(floor_shader, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");

    GPU_FreeShader(frag);
    GPU_FreeShader(vert);

    dash_screen_anim = malloc(sizeof(GPU_Image *) * 6);
    getTextureFiles("Textures/Abilities/Dash/screen_anim", 6, &dash_screen_anim);

    dash_anim_sprite = createSprite(true, 1);
    dash_anim_sprite->animations[0] = create_animation(6, 10, dash_screen_anim);
    dash_anim_sprite->animations[0].loop = false;
    dash_anim_sprite->animations[0].fps = 10;
    dash_anim_sprite->animations[0].frame = 5;


    dash_icon = load_texture("Textures/Abilities/Icons/dash_icon.png");

    ability_icon_frame = load_texture("Textures/Abilities/Icons/icon_frame.png");

    shotgun_icon = load_texture("Textures/Abilities/Icons/shotgun_icon.png");

    shoot_icon = load_texture("Textures/Abilities/Icons/shoot_icon.png");

    exploder_hit = load_texture("Textures/ExploderEnemyAnim/exploder_hit.png");

    exploder_explosion_texture = malloc(sizeof(GPU_Image *) * 12);
    getTextureFiles("Textures/ExploderEnemyAnim/Explosion/explosion", 12, &exploder_explosion_texture);

    shooter_dirs_textures = malloc(sizeof(GPU_Image *) * 16);

    for (int i = 0; i < 16; i++) {
        char *baseFileName = "Textures/ShooterEnemy/frame";
        char num[get_num_digits(i + 1)];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(baseFileName, num);
        char *fileWithExtension = concat(fileWithNum, ".png");

        shooter_dirs_textures[i] = load_texture(fileWithExtension);
    }

    default_particle_texture = load_texture("Textures/base_particle.png");

    shooter_bullet_default_frames = malloc(sizeof(GPU_Image *) * 4);
    shooter_bullet_explode_frames = malloc(sizeof(GPU_Image *) * 4);

    getTextureFiles("Textures/ShooterEnemy/Bullet/Default/Bullet", 4, &shooter_bullet_default_frames);
    getTextureFiles("Textures/ShooterEnemy/Bullet/Explode/Bullet", 4, &shooter_bullet_explode_frames);

    exploder_frames = malloc(sizeof(GPU_Image *) * 80);
    getTextureFiles("Textures/ExploderEnemyAnim/exploderEnemyAnim", 80, &exploder_frames);


    exclam_notice_anim = malloc(sizeof(GPU_Image *) * 6);
    getTextureFiles("Textures/ExclamNoticeAnim/noticeAnim", 6, &exclam_notice_anim);

    player_default_hurt = create_sound("Sounds/player_default_hurt.wav");
    player_default_shoot = create_sound("Sounds/player_default_shoot.wav");

    mimran_jumpscare = load_texture("Textures/scary_monster2.png");

    shooter_hit_texture = load_texture("Textures/ShooterEnemy/hit_frame1.png");

    healthbar_texture = load_texture("Textures/health_bar1.png");

    vignette_texture = load_texture("Textures/vignette.png");

    fenceTexture = load_texture("Textures/fence.png");
    SDL_SetTextureBlendMode(fenceTexture, SDL_BLENDMODE_BLEND);

    floorAndCeiling = GPU_CreateImage(RESOLUTION_X, RESOLUTION_Y, GPU_FORMAT_RGBA);
    GPU_SetImageFilter(floorAndCeiling, GPU_FILTER_NEAREST);

    floor_and_ceiling_target_image = GPU_CreateImage(RESOLUTION_X, RESOLUTION_Y, GPU_FORMAT_RGBA);
    GPU_SetImageFilter(floor_and_ceiling_target_image, GPU_FILTER_NEAREST);

    floorTexture = load_texture("Textures/floor.png");
    floorLightTexture = load_texture("Textures/floor_light.png");
    floorTexture2 = load_texture("Textures/floor2.png");
    ceilingTexture = load_texture("Textures/ceiling.png");
    ceilingLightTexture = load_texture("Textures/ceiling_light.png");
    wallTexture = load_texture("Textures/wall.png");
     GPU_SetImageFilter(wallTexture, GPU_FILTER_NEAREST);


    wallFrames = malloc(sizeof(GPU_Image *) * 17);
    getTextureFiles("Textures/WallAnim/wallAnim", 17, &wallFrames);

    crosshair = load_texture("Textures/crosshair.png");

    leftHandSprite = createSprite(true, 2);
    GPU_Image **default_hand = malloc(sizeof(GPU_Image *)); 
    default_hand[0] = load_texture("Textures/rightHandAnim/rightHandAnim6.png");
    leftHandSprite->animations[0] = create_animation(1, 0, default_hand);


    leftHandSprite->animations[1] = create_animation(6, 0, NULL);
    getTextureFiles("Textures/rightHandAnim/rightHandAnim", 6, &leftHandSprite->animations[1].frames);
    leftHandSprite->animations[1].fps = 12;

    leftHandSprite->animations[1].loop = false;
    spritePlayAnim(leftHandSprite, 0);

    shootHitEffectFrames = malloc(sizeof(GPU_Image *) * 5);

    getTextureFiles("Textures/ShootEffectAnim/shootHitEffect", 5, &shootHitEffectFrames);


    entityTexture = load_texture("Textures/scary_monster.png");

    skybox_texture = load_texture("Textures/skybox.png");

}

PlayerEntity *find_player_entity_by_id(int id) {
    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);

        if (object->type == PLAYER_ENTITY) {
            PlayerEntity *player_entity = object->val;
            if (player_entity->id == id) {
                return player_entity;
            }
        }
    }

    return NULL;
}

PlayerEntity *find_or_add_player_entity_by_id(int id) {

    if (id == client_self_id) return NULL;

    PlayerEntity *player_entity = find_player_entity_by_id(id);

    if (player_entity != NULL) return player_entity;

    client_add_player_entity(id);
    return find_player_entity_by_id(id);
}


void player_entity_take_dmg(PlayerEntity *player_entity, double dmg) {
    // actually damage the player

    player_hit_particles->pos = player_entity->entity.pos;
    player_hit_particles->height = player_entity->entity.height;
    player_hit_particles->target = NULL;
    particle_spawner_explode(player_hit_particles);

}

void ability_bomb_activate() {
    printf("Bomb! \n");
    Projectile *bomb = malloc(sizeof(Projectile));
    *bomb = create_bomb_projectile(player->pos, v2_mul(playerForward, to_vec(1.4)));
    bomb->entity.height = get_max_height() / 2 + get_player_height();

    add_game_object(bomb, PROJECTILE);

    MPPacket packet = {.type = PACKET_ABILITY_BOMB, .len = sizeof(struct ability_bomb_packet), .is_broadcast = true};

    struct ability_bomb_packet packet_data = {.pos = bomb->entity.pos, .height = bomb->entity.height, .vel = bomb->vel, .height_vel = bomb->height_vel, .sender_id = client_self_id};

    MPClient_send(packet, &packet_data);


}

Ability ability_bomb_create() {
    Ability ability = {
        .activate = ability_bomb_activate,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = 5,
        .delay = 0,
        .delay_timer = 0,
        .texture = entityTexture,
        .tick = default_ability_tick,
        .timer = 2,
        .type = A_SPECIAL
    };

    return ability;
}

bool is_projectile_type(int type) {
    return in_range(type, PROJECTILE_START, PROJECTILE_END);
}

void projectile_tick(Projectile *projectile, double delta) {

    cd_print(true, "bruh \n");

    if (!projectile->_created && projectile->on_create != NULL) {
        projectile->on_create(projectile);
        projectile->_created = true;
    }

    projectile->vel = v2_add(projectile->vel, v2_mul(projectile->accel, to_vec(delta * 144)));
    projectile->height_vel += projectile->height_accel * delta;

    projectile->entity.pos = v2_add(projectile->entity.pos, v2_mul(projectile->vel, to_vec(delta * 144)));
    projectile->entity.height += projectile->height_vel * delta * 144;

    if (projectile->entity.height <= get_max_height() / 2) {
        if (projectile->destroy_on_floor) {
            printf("Projectile destroyed on floor. \n");
            projectile_destroy(projectile);
            return;
        }

        projectile->height_vel *= -projectile->bounciness;
        projectile->entity.height = get_max_height() / 2;
    }

    projectile->life_timer -= delta;
    if (projectile->life_timer <= 0) {
        printf("Projectile time ran out. \n");
        projectile_destroy(projectile);
        return;
    }
    
    projectile->collider.pos = projectile->entity.pos;

    if (projectile->on_tick != NULL) {
        projectile->on_tick(projectile, delta);
    }
}

void projectile_destroy(Projectile *projectile) {
    if (projectile->on_destruction != NULL) {
        projectile->on_destruction(projectile);
    }

    remove_game_object(projectile, PROJECTILE);
}

Projectile create_default_projectile(double life_time) {
    Projectile projectile = {0};

    projectile.life_time = life_time;
    projectile.life_timer = life_time;
    projectile.entity.color = (SDL_Color){255, 255, 255, 255};
    projectile.collider.radius = 15;

    return projectile;
}

Projectile create_bomb_projectile(v2 pos, v2 start_vel) {
    Projectile projectile = create_default_projectile(1.6);

    projectile.height_accel = PROJECTILE_GRAVITY;
    projectile.bounciness = 0.5;
    projectile.destroy_on_floor = false;
    projectile.entity.pos = pos;
    projectile.destroy_on_floor = true;
    projectile.vel = v2_add(start_vel, v2_mul(player->vel, to_vec(0.2)));
    projectile.height_vel = player->pitch * -3;
    projectile.entity.sprite = createSprite(false, 0);
    projectile.entity.sprite->texture = entityTexture;
    projectile.entity.size = to_vec(8000);
    projectile.on_destruction = bomb_on_destroy;
    
    
    return projectile;
}

void bomb_on_destroy(Projectile *projectile) {
    shakeCamera(30, 15, true, 10);
    bomb_explode_particles->min_size = to_vec(15000);
    bomb_explode_particles->max_size = to_vec(28000);
    bomb_explode_particles->height_accel = -PARTICLE_GRAVITY * 0.1;
    bomb_explode_particles->fade_scale = true;
    bomb_explode_particles->affected_by_light = false;
    bomb_explode_particles->min_speed = 30;
    bomb_explode_particles->max_speed = 80;
    bomb_explode_particles->spread = 2;
    bomb_explode_particles->height_dir = 0.2;
    bomb_explode_particles->explode_particle_amount = 20;
    bomb_explode_particles->particle_lifetime = 1.3;
    bomb_explode_particles->floor_drag = 0.3;
    
    bomb_explode_particles->start_color = GPU_MakeColor(255, 230, 120, 255);
    bomb_explode_particles->end_color = GPU_MakeColor(255, 230, 120, 255);

    bomb_explode_particles->pos = projectile->entity.pos;
    bomb_explode_particles->height = get_max_height() / 2;
    particle_spawner_explode(bomb_explode_particles);

    bomb_explode_particles->particle_lifetime = 2.5;
    bomb_explode_particles->start_color = GPU_MakeColor(120, 120, 120, 255);
    bomb_explode_particles->end_color = GPU_MakeColor(0, 0, 0, 0);
    bomb_explode_particles->min_size = to_vec(15000);
    bomb_explode_particles->max_size = to_vec(19000);
    bomb_explode_particles->min_speed = 100;
    bomb_explode_particles->max_speed = 150;

    particle_spawner_explode(bomb_explode_particles);
}

Ability pick_random_ability_from_array(Ability arr[], int size) {
    return arr[randi_range(0, size - 1)];
}

void randomize_player_abilities() {

    randomize();

    Ability *default_primary = malloc(sizeof(Ability));
    Ability *default_secondary = malloc(sizeof(Ability));
    Ability *default_utility = malloc(sizeof(Ability));
    //Ability *default_special = malloc(sizeof(Ability));

    Ability primary_choices[] = {ability_primary_shoot_create()};
    Ability secondary_choices[] = {ability_secondary_shoot_create(), ability_bomb_create()};
    Ability utility_choices[] = {ability_dash_create()};
    //Ability speical_choices[] = {};

    *default_primary = pick_random_ability_from_array(primary_choices, sizeof(primary_choices) / sizeof(Ability));
    *default_secondary = pick_random_ability_from_array(secondary_choices, sizeof(secondary_choices) / sizeof(Ability));
    *default_utility = pick_random_ability_from_array(utility_choices, sizeof(utility_choices) / sizeof(Ability));
    //*default_special = ability_bomb_create();

    
    if (player->primary != NULL) {
        free(player->primary);
    }
    if (player->secondary != NULL) {
        free(player->secondary);
    }
    if (player->utility != NULL) {
        free(player->utility);
    }
    // if (player->special != NULL) {
    //     free(player->special);
    // }
    

    player->primary = default_primary;
    player->secondary = default_secondary;
    player->utility = default_utility;
    player->special = NULL;
}


// #END
#pragma GCC diagnostic pop