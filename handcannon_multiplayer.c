
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include "game_utils.c"
#include "globals.h"
#include "ui.c"
#include "mystring.c"
#include "multiplayer.c"



// #DEFINITIONS

#define DEBUG_FLAG true

#define SERVER_IP "84.95.64.135"
#define SERVER_PORT 1155
#define TPS 300
#define FPS 300
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 580

// ^^^ The two most OG lines of the project ^^^

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
#define SERVER_TICK_RATE 40
#define PLAYER_COLLIDER_RADIUS 8
#define NODE_MAX_SIZE 512
#define MAX_PACKET_SIZE 1024

#define PARTICLE_GRAVITY -50000
#define PROJECTILE_GRAVITY -2000

#define OUT_OF_SCREEN_POS \
    (v2) { WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100 }


#define iter_over_all_nodes(varname, ...) \
    do { \
        Node **node_arr = get_all_nodes_array(); \
        for (int name_i_wont_use = 0; name_i_wont_use < array_length(node_arr); name_i_wont_use++) { \
            Node *varname = node_arr[name_i_wont_use]; \
            __VA_ARGS__ \
        } \
        array_free(node_arr); \
    } while(0)


#define get_window() SDL_GetWindowFromID(actual_screen->context->windowID)

#define DEF_STRUCT(name, typename, ...) enum {typename = __COUNTER__}; typedef struct name __VA_ARGS__ name;
#define END_STRUCT(typename) enum {typename##_END = __COUNTER__};
#define instanceof(type, parent_type) (type >= parent_type && type <= parent_type##_END)
#define node(thing) ((Node *)thing)


// #PACKETS
enum PacketTypes {
    PACKET_UPDATE_PLAYER_ID,
    PACKET_PLAYER_POS,
    PACKET_PLAYER_JOINED,
    PACKET_DUNGEON_SEED,
    PACKET_REQUEST_DUNGEON_SEED,
    PACKET_ABILITY_SHOOT,
    PACKET_HOST_LEFT,
    PACKET_PLAYER_LEFT,
    PACKET_PLAYER_TOOK_DAMAGE,
    PACKET_REQUEST_CREATE_NODE,
    PACKET_SEND_SYNC_ID,
    PACKET_SYNC_PROJECTILE,

    PACKETS_TO_SYNC,

        PACKET_ABILITY_BOMB,
        PACKET_ABILITY_FORCEFIELD,
        PACKET_ABILITY_SWITCHSHOT,

    PACKETS_TO_SYNC_END,
    PACKET_SWITCH_POSITIONS,
    PACKETS_END
};

struct switch_positions_packet {
    int id1;
    v2 pos1;
    double h1;
    int id2;
    v2 pos2;
    double h2;
};

struct ability_switchshot_packet {
    int sync_id;
    v2 pos;
    double height;
    v2 vel;
    double h_vel;
    int sender_id;
};

struct ability_forcefield_packet {
    int sync_id;
    v2 pos;
    double height;
    v2 vel;
    double h_vel;
    int sender_id;
};

struct sync_projectile_packet {
    v2 pos, vel;
    double height, h_vel;
    int sync_id;
};

struct send_sync_id_packet {
    int sync_id;
};

struct ability_bomb_packet {
    int sync_id;
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

enum Tiles { WALL1 = 1, WALL2 = 2 };


typedef enum AbilityType {
    A_PRIMARY,
    A_SECONDARY,
    A_UTILITY,
    A_SPECIAL
} AbilityType;

// #STRUCTS

typedef struct CollisionData {
    v2 offset;  // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;


DEF_STRUCT(Ability, ABILITY, {
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
});

DEF_STRUCT(Animation, ANIMATION, {
    GPU_Image **frames;
    int frameCount;
    int frame;
    double fps;
    double timeToNextFrame;
    bool loop;
    bool playing;
    int priority;
});

DEF_STRUCT(WallStripe, WALL_STRIPE, {
    v2 pos, normal; // 16, 16
    double size; // 8
    double brightness; // 8  // a bunch of rendering bullshit:
    double collIdx; // 8
    double wallWidth; // 8
    GPU_Image *texture; // 8
    int i; // 4
});

DEF_STRUCT(Node, NODE, {
    void (*on_render)(struct Node *);
    void (*on_tick)(struct Node *, double);
    void (*on_ready)(struct Node *);
    void (*on_delete)(struct Node *);

    bool _called_ready;

    int type;

    int sync_id;
    int size;

    bool freed, queued_for_deletion;

    struct Node *parent;
    struct Node **children;
});
    DEF_STRUCT(Line, LINE, {
        Node node;
        GPU_Image *texture;
        v2 p1, p2;
        double h1, h2;
        double width;
        bool fade;
    });

    END_STRUCT(LINE);

    DEF_STRUCT(Sprite, SPRITE, {
        Node node;
        GPU_Image *texture;  // not used if animated
        bool isAnimated;
        int current_anim_idx;
        Animation *animations;
        SDL_Color color;
        bool inherit_color;
        v2 scale;
        int autoplay_anim;
    });

    END_STRUCT(SPRITE);

    DEF_STRUCT(ColorRect, COLOR_RECT, {
        Node node;
        SDL_Color color;
        int align_x, align_y;
    });

    DEF_STRUCT(DirSprite, DIR_SPRITE, {
        Node node;
        Sprite *sprites;
        v2 dir;  // global direction
        int dirCount;
        int current_anim;
        int playing;
        int fps;
    });

    END_STRUCT(DIR_SPRITE);

    DEF_STRUCT(WorldNode, WORLD_NODE, {
        Node node;
        v2 pos;
        v2 size;
        double height;
    });

        DEF_STRUCT(Entity, ENTITY, {
            WorldNode world_node;
            // v2 pos, size;
            // double height;
            bool affected_by_light;
            SDL_Color color;
        });

            DEF_STRUCT(Effect, EFFECT, {
                Entity entity;
                double life_time;
                double life_timer;
            });

                DEF_STRUCT(Particle, PARTICLE, {
                    Effect effect;
                    v2 vel;
                    double h_vel;
                    v2 accel;
                    double h_accel;
                    double bounciness;
                    double floor_drag;
                    v2 initial_pos;
                    double initial_height;
                    v2 initial_size;
                    bool fade;
                    bool fade_scale;

                    v2 radial_accel;
                    double height_radial_accel;
                    double damp;

                    SDL_Color start_color, end_color;

                });

            END_STRUCT(EFFECT);

            DEF_STRUCT(PlayerEntity, PLAYER_ENTITY, {
                Entity entity;
                v2 desired_pos;
                double desired_height;
                v2 dir;
                int id;
            });

            DEF_STRUCT(Projectile, PROJECTILE, {
                Entity entity;
                int type;
                v2 vel;
                double height_vel;
                v2 accel;
                double height_accel;
                double life_time, life_timer;
                double bounciness;
                bool destroy_on_floor, destroy_on_ceiling;
                bool _created;
                void (*on_create)(struct Projectile *);
                void (*on_tick)(struct Projectile *, double);
                void (*on_destruction)(struct Projectile *);

                int shooter_id;
            });

            END_STRUCT(PROJECTILE);

        END_STRUCT(ENTITY);

        DEF_STRUCT(Player, PLAYER, {            
            WorldNode world_node;
            v2 vel;
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
            double tallness;

            Ability *primary, *secondary, *utility, *special;

        });


        DEF_STRUCT(ParticleSpawner, PARTICLE_SPAWNER, {
            WorldNode world_node;

            v2 dir;
            double height_dir;

            double spread;

            v2 accel;
            double height_accel;
            
            v2 radial_accel;
            double height_radial_accel;
            double damp;

            double min_speed, max_speed;

            v2 min_size, max_size;

            int spawn_rate; // per second

            double spawn_timer;

            double bounciness;

            double floor_drag;

            int sprite_idx; // if -1, pick randomly

            double particle_lifetime;

            bool active;

            Entity *target;

            bool fade;

            bool fade_scale;

            bool affected_by_light;

            int explode_particle_amount;

            SDL_Color start_color, end_color;

        });

    DEF_STRUCT(CircleCollider, CIRCLE_COLLIDER, {
        WorldNode world_node;
        double radius;
        void (*on_collide)(struct CircleCollider *, CollisionData);
        void (*custom_on_collide)(struct CircleCollider *, CollisionData);
    });

    END_STRUCT(WORLD_NODE);


    DEF_STRUCT(Tilemap, TILEMAP, {
        Node node;
        int level_tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH];
        int floor_tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH];
        int ceiling_tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH];
    });

    DEF_STRUCT(Renderer, RENDERER, {
        Node node;
    });

    DEF_STRUCT(LightPoint, LIGHT_POINT, {
        Node node;
        v2 pos;
        double strength;
        double radius;
        SDL_Color color;
    });



    DEF_STRUCT(CanvasNode, CANVAS_NODE, {
        Node node;
        v2 pos, size;
        int z_index;
    });

    END_STRUCT(CANVAS_NODE);



END_STRUCT(NODE);


#define new(var_type, var_type_name, ...)  ({var_type object = var_type##_new(__VA_ARGS__); node(&object)->size = sizeof(var_type);((Node *)&object)->type = instanceof(var_type_name, NODE)? var_type_name : ((Node *)&object)->type; object;})
#define alloc(var_type, var_type_name, ...) ({var_type *ptr; ptr = malloc(sizeof(var_type)); (*ptr) = new(var_type, var_type_name, __VA_ARGS__); ptr;})


// #STRUCTS

typedef struct Raycast {
    v2 pos, dir;
} Raycast;

typedef struct RayCollisionData {
    v2 startpos, collpos, normal;
    Node *collider;
    int colliderType;
    GPU_Image *colliderTexture;
    bool hit;
    double collIdx, wallWidth;
} RayCollisionData;

enum ProjectileTypes {
    PROJ_BOMB,
    PROJ_FORCEFIELD
};

typedef struct RenderObject {
    void *val;
    double dist_squared;
    int type;
    bool isnull;
} RenderObject;

typedef struct BakedLightColor {
    float r, g, b;
} BakedLightColor;




typedef struct Room {
    v2 room_idx; // 0, 0 -> 3, 3
    String room_file_name;
    bool is_start, is_boss;

    struct Room *left, *right, *up, *down;
    v2 left_entrance_pos, right_entrance_pos, top_entrance_pos, bottom_entrance_pos;

    bool initialized;
} Room;

// #FUNC

bool is_sync_id_valid(int sync_id);

void Node_ready(Node *node);

Sprite *ParticleSpawner_get_sprite(ParticleSpawner *spawner);

Sprite *copy_sprite(Sprite *sprite, bool copy_tree);

void Sprite_ready(Node *node);

Node *copy_node(Node *node, bool copy_tree);

void switchshot_projectile_switch(Projectile *proj, int player_id);

void switchshot_projectile_tick(Projectile *proj, double delta);

Projectile *create_switchshot_projectile(v2 pos, double height, v2 vel, double h_vel);

void switchshot_activate(Ability *ability);

Ability create_switchshot_ability();

Node *find_node_by_sync_id(int sync_id);

void add_to_sync_queue(Node *node);

Node *use_sync_id(int sync_id);

void Line_tick(Node *node, double delta);

void Line_render(Node *node);

Line Line_new();

void render_textured_quad(GPU_Image *texture, v2 top_left, v2 top_right, v2 bot_left, v2 bot_right);

void Node_queue_add_to_game_node(Node *node);

void DirSprite_on_delete(Node *node);

void DirSprite_add_animation(DirSprite *dir_sprite, int frame_count, GPU_Image **textures);

void DirSprite_tick(Node *node, double delta);

void DirSprite_render(Node *node);

DirSprite DirSprite_new(int dir_count);

void Projectile_on_collide(CircleCollider *collider, CollisionData data);

void init_ability_hud();

void init_crosshair();

void CircleCollider_default_on_collide(CircleCollider *collider, CollisionData data);

void health_node_tick(Node *node, double delta);

v2 world_to_screen_size(v2 size, v2 pos, double height);

void ColorRect_render(Node *node);

ColorRect ColorRect_new(SDL_Color color, int align_x, int align_y);

void init_health_bar();

void Node_queue_deletion(Node *node);

Node *get_child_by_type(Node *parent, int child_type);

CircleCollider CircleCollider_new(int radius);

CanvasNode CanvasNode_new();

void hand_sprite_tick(Node *node, double delta);

void init_hand_sprite();

void ability_tick(Ability *ability, double delta);

Effect Effect_new(double life_time);

Particle Particle_new(double life_time);

WorldNode WorldNode_new();

void Sprite_delete(Node *node);

void Sprite_render(Node *node);

void Sprite_tick(Node *node, double delta);

Sprite Sprite_new(bool is_animated);

int get_node_count();

PlayerEntity PlayerEntity_new();

Entity Entity_new();

Node **get_all_nodes_array();

LightPoint LightPoint_new();

void Renderer_render(Node *node);

Renderer Renderer_new();

Tilemap Tilemap_new();


void Node_render(Node *node);

void Node_tick(Node *node, double delta);

void Node_add_child(Node *parent, Node *child);

void Node_remove_child(Node *parent, Node *child);

void Node_delete(Node *node);

Node Node_new();

void projectile_forcefield_on_tick(Projectile *projectile, double delta);

Projectile *create_forcefield_projectile();

void ability_forcefield_activate();

CollisionData get_circle_collision(CircleCollider *col1, CircleCollider *col2);

void bomb_on_tick(Projectile *projectile, double delta);

void Entity_tick(Entity *entity, double delta);

void randomize_player_abilities();

void bomb_on_destroy(Projectile *projectile);

Projectile *create_bomb_projectile(v2 pos, v2 start_vel);

Projectile Projectile_new(double life_time);

void projectile_destroy(Projectile *projectile);

void Projectile_tick(Node *node, double delta);

void ability_bomb_activate();

Ability ability_bomb_create();
Ability ability_forcefield_create();

void send_dmg_packet(int id, double dmg);

void player_entity_take_dmg(PlayerEntity *player_entity, double dmg);

PlayerEntity *find_or_add_player_entity_by_id(int id);

PlayerEntity *find_player_entity_by_id(int id);

void init_textures();

void PlayerEntity_tick(PlayerEntity *player_entity, double delta);

void init_player(v2 pos);

void on_client_recv(MPPacket packet, void *data);

void on_server_recv(SOCKET socket, MPPacket packet, void *data);

void on_player_connect(SOCKET player_socket);

void on_player_disconnect(SOCKET player_socket);

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

void place_entity(v2 pos, int type);

void Particle_tick(Node *node, double delta);

void particle_spawner_spawn(ParticleSpawner *spawner);

void particle_spawner_tick(Node *node, double delta);

ParticleSpawner ParticleSpawner_new(bool active);

void draw_3d_line(v2 pos1, double h1, v2 pos2, double h2);

void _shoot(double spread);

void ability_secondary_shoot_activate(Ability *ability);

Ability ability_dash_create();

Ability ability_secondary_shoot_create();

Ability ability_primary_shoot_create();

void default_ability_tick(Ability *ability, double delta);


void dir_sprite_play_anim(DirSprite *dir_sprite, int anim);

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

void reset_tilemap(int tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH]);

void load_level(char *file);

void init();

void render(double delta);

RayCollisionData ray_circle(Raycast ray, CircleCollider *collider);

RayCollisionData ray_node(Raycast ray, Node *node);

RayCollisionData castRayForEntities(v2 pos, v2 dir);

RayCollisionData castRay(v2 pos, v2 dir);

RayCollisionData castRayForAll(v2 pos, v2 dir);

CollisionData getCircleTileCollision(CircleCollider *circle, v2 tilePos);

CollisionData getCircleTileMapCollision(CircleCollider *circle);

void key_pressed(SDL_Keycode key);

void key_released(SDL_Keycode key);

void tick(double delta);

void handle_input(SDL_Event event);

double mili_to_sec(u64 mili);

v2 get_player_forward();

Animation create_animation(int frameCount, int priority, GPU_Image **frames);

void freeAnimation(Animation *anim);

GPU_Image *get_sprite_current_texture(Sprite *sprite);

Sprite *dir_sprite_current_sprite(DirSprite *dSprite, v2 spritePos);

void player_tick(Node *node, double delta);

void dSpriteTick(DirSprite *dSprite, v2 spritePos, double delta);

void spriteTick(Sprite *sprite, double delta);

void animation_tick(Animation *anim, double delta);

void Effect_tick(Node *node, double delta);

DirSprite *createDirSprite(int dirCount);

void spritePlayAnim(Sprite *sprite, int idx);

void ability_shoot_activate(Ability *ability);

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light, SDL_Color custom_color);

void renderDirSprite(DirSprite *dSprite, v2 pos, v2 size, double height);

double angleDist(double a1, double a2);

void shakeCamera(double strength, int ticks, bool fade, int priority);



bool isValidLevel(char *file);

GPU_Image **get_texture_files(char *file_name, int file_count);

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
GPU_Image **switchshot_anim;
GPU_Image *switchshot_icon;
GPU_Image *shoot_ray;
GPU_Image **forcefield_field;
GPU_Image *hand_default;
GPU_Image *shoot_ray;
GPU_Image **hand_shoot;
GPU_Image **forcefield_anim;
GPU_Image **player_textures;
GPU_Image *bomb_icon;
GPU_Image **bomb_anim;
GPU_Image *blood_particle;
GPU_Image **dash_screen_anim;
GPU_Image *lightmap_image = NULL;
GPU_Image *tilemap_image = NULL;
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
GPU_Image **bomb_explosion_particles;
GPU_Image **bomb_smoke_particles;

// #SOUNDS
Sound *bomb_explosion;
Sound *rapidfire_sound;
Sound *exploder_explosion;
Sound *player_default_shoot;
Sound *player_default_hurt;

// #SHADERS
int floor_shader;
GPU_ShaderBlock floor_shader_block;

int bloom_shader;
GPU_ShaderBlock bloom_shader_block;

// #ROOMGEN
Room rooms[DUNGEON_SIZE][DUNGEON_SIZE] = {0};


// #VAR

Node *hand_node;

bool ready_to_render = false;

Node *root_node;

Node *game_node;

Node **sync_id_queue;
Node **deletion_queue;
Node **add_queue;

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

int server_next_sync_id = 42;
int client_last_seen_sync_id = -1;

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
BakedLightColor baked_light_grid[TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION][TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION] = {0};
bool fullscreen = false;
bool running = true;

Player *player = NULL;

Tilemap *tilemap = NULL;

Renderer *renderer = NULL;

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
double ambient_light = 0.6;
int floorRenderStart;
const int tileSize = WINDOW_WIDTH / 30;
double HEIGHT_TO_XY;
double XY_TO_HEIGHT;
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

    deletion_queue = array(Node *, 100);
    add_queue = array(Node *, 10);
    sync_id_queue = array(Node *, 10);

    

    

    root_node = alloc(Node, NODE);

    init_textures();

    init_player(to_vec(500));

    Node_add_child(root_node, player);

    tilemap = alloc(Tilemap, TILEMAP);

    Node_add_child(root_node, tilemap);

    renderer = alloc(Renderer, RENDERER);

    Node_add_child(root_node, renderer);

    game_node = alloc(Node, NODE);

    Node_add_child(root_node, game_node);


    init();
    
    printf("xy to height: %.2f \n", XY_TO_HEIGHT);
    printf("height to xy: %.2f \n", HEIGHT_TO_XY);

    reset_tilemap(tilemap->level_tilemap);
    reset_tilemap(tilemap->floor_tilemap);
    reset_tilemap(tilemap->ceiling_tilemap);
    bake_lights();

    bool is_server = argc == 2 && !strcmp(argv[1], "server");
    if (DEBUG_FLAG) {
        is_server = !is_server;
    }

    if (is_server) {
        printf("creating server \n");
        MPServer();
    }
    MPClient(SERVER_IP);


    bool ran_first_tick = false;
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
        render_timer += delta;
        if (tick_timer >= 1000 / TPS) {
            tick(mili_to_sec(tick_timer) * game_speed);
            ran_first_tick = true;
            tick_timer = 0;
        }

        if (render_timer >= 1000 / FPS && ran_first_tick) {
            realFps = 1000.0 / render_timer;
            render(mili_to_sec(render_timer) * game_speed);
            render_timer = 0;
        }
    }

    struct player_left_packet packet_data = {.id = client_self_id};

    MPPacket packet = {.type = PACKET_PLAYER_LEFT, .is_broadcast = true, .len = sizeof(packet_data)};

    MPClient_send(packet, &packet_data);


    if (MP_is_server) {
        MPPacket hl_packet = {.type = PACKET_HOST_LEFT, .len = 0, .is_broadcast = true};
        MPServer_send(hl_packet, NULL);
    }

    GPU_FreeImage(screen_image);

    GPU_Quit();

    SDL_Quit();
}

void init() {  // #INIT

    //test1();

    bomb_explosion = create_sound("Sounds/explosion.wav");

    randomize();
    client_self_color.r = randi_range(25, 255);
    client_self_color.g = randi_range(25, 255);
    client_self_color.b = randi_range(25, 255);
    client_self_color.a = 255;

    GPU_SetBlendMode(screen_image, GPU_BLEND_NORMAL);


    //exit(1);
    

    rapidfire_sound = create_sound("Sounds/shotgun_ability.wav");

    exploder_explosion = create_sound("Sounds/exploder_explosion.wav");

    

    init_cd_print();

    tanHalfFOV = tan(deg_to_rad(fov / 2));
    tanHalfStartFOV = tan(deg_to_rad(startFov / 2));
    
    HEIGHT_TO_XY = (double)(tileSize * 2) / get_max_height();
    XY_TO_HEIGHT = get_max_height() / (tileSize * 2);

    for (int i = 0; i < 26; i++) keyPressArr[i] = false;


    init_hand_sprite();
    init_health_bar();
    init_crosshair();
    init_ability_hud();



    // Line *line = alloc(Line, LINE);

    // line->p1 = V2(500, 800);
    // line->h1 = player->world_node.height;
    // line->p2 = v2_add(line->p1, to_vec(100));
    // line->h2 = line->h1 + 100 * XY_TO_HEIGHT;
    // line->texture = entityTexture;

    // Node_add_child(root_node, line);

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
    player->world_node.height = clamp(player->world_node.height, 0, get_max_height());

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



void player_tick(Node *node, double delta) {

    CircleCollider *collider = get_child_by_type(player, CIRCLE_COLLIDER);

    playerForward = get_player_forward();

    // why use the node when the player is global :p

    static double update_pos_timer = 1.0 / CLIENT_UPDATE_RATE;

    //String height_str = String_from_double(player->world_node.height, 2);

    //write_to_debug_label(height_str);
    //printf("%.2f \n", player->world_node.height);

    double speed_multiplier = 1;



    player->height_vel -= 750 * delta;
    player->world_node.height += player->height_vel * (delta * 144);
    if (player->world_node.height > get_max_height() - player->tallness * 0.2) {
        player->height_vel = 0;
    }
    player->world_node.height = clamp(player->world_node.height, player->tallness * 0.8, get_max_height());

    // player->collider->world_node.pos = player->world_node.pos;

    ability_tick(player->primary, delta);
    ability_tick(player->secondary, delta);
    ability_tick(player->utility, delta);
    ability_tick(player->special, delta);

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

    // RayCollisionData ray = castRay(player->world_node.pos, movement_dir);
    // if (ray.hit) {
    //     double dist = v2_distance(ray.startpos, ray.collpos);
    //     if (movement > dist) {
    //         move_without_ray = false;
    //         player->world_node.pos = v2_add(player->world_node.pos, v2_mul(movement_dir, to_vec(dist - 10)));
    //     }
    // }

   
    player->world_node.pos = v2_add(player->world_node.pos, v2_mul(finalVel, to_vec(delta)));
    

    update_pos_timer -= delta;
    if (update_pos_timer <= 0) {
        update_pos_timer = 1.0 / CLIENT_UPDATE_RATE;
        struct player_pos_packet packet_data = {
            .pos = player->world_node.pos,
            .height = player->world_node.height,
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
    if (sprite->current_anim_idx == -1) return;
    animation_tick(&sprite->animations[sprite->current_anim_idx], delta);
}

// #TICK
void tick(double delta) {
    

    if (loading_map) {
        init_loading_screen();
        generate_dungeon();
        update_loading_progress(0.1);
        load_dungeon();
        loading_map = false;
        ready_to_render = true;
    }

    SDL_SetRelativeMouseMode(lock_and_hide_mouse);

    Node_tick(root_node, delta);

    // sync nodes
    
    if (MP_is_server) {
        
        static double server_tick_timer = 1 / SERVER_TICK_RATE;

        server_tick_timer -= delta;
        if (server_tick_timer <= 0) {
            server_tick_timer = 1 / SERVER_TICK_RATE;
            iter_over_all_nodes(node, {
                if (node->sync_id == -1 || !is_sync_id_valid(node->sync_id)) continue;

                if (instanceof(node->type, PROJECTILE)) {

                    Projectile *proj = node;

                    MPPacket packet = {.type = PACKET_SYNC_PROJECTILE, .len = sizeof(struct sync_projectile_packet), .is_broadcast = true};

                    struct sync_projectile_packet packet_data = {
                        .sync_id = node->sync_id,
                        .pos = proj->entity.world_node.pos,
                        .height = proj->entity.world_node.height,
                        .vel = proj->vel,
                        .h_vel = proj->height_vel
                    };

                    printf("Sending packet data: \n\tsync id: %d\n\tpacketlen: %d \n", packet_data.sync_id, packet.len);

                    MPClient_send(packet, &packet_data);
                }
                
            });
        }

        
    }


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

    if (client_dungeon_seed == -1) {
        client_dungeon_seed_request_timer -= delta;
        if (client_dungeon_seed_request_timer <= 0) {
            MPClient_send((MPPacket){.type = PACKET_REQUEST_DUNGEON_SEED, .len = 0, .is_broadcast = false}, NULL);
            client_dungeon_seed_request_timer = 0.5;
        }
    }
    // printf("Deletion queue length: %d \n", array_length(deletion_queue));

    while (array_length(deletion_queue) > 0) {
        Node_delete(deletion_queue[0]);
    }

    for (int i = array_length(add_queue) - 1; i >= 0; i--) {
        Node_add_child(game_node, add_queue[i]);
        array_remove(add_queue, i);
    }

    //printf("tick start");

    



    // if (game_speed_duration_timer <= 0) {
    //     game_speed = 1;
    // } else {
    //     game_speed_duration_timer -= delta / game_speed; // so it counts by real time instead of scaled time
    // }

    // vignette_color = lerp_color(vignette_color, (SDL_Color){0, 0, 0}, delta);

    // playerForward = get_player_forward();

    

    // tanHalfFOV = tan(deg_to_rad(fov / 2));

    // SDL_SetRelativeMouseMode(lock_and_hide_mouse); // bro this function was made for me

    

    


    // if (queued_player_death) {
    //     queued_player_death = false;
    //     _player_die();
    // }
}

// for slower decay, make 'a' smaller.
double distance_to_color(double distance, double a) {
    return exp(-a * distance);
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
    
    if (v2_equal(pos, player->world_node.pos)) {
        return (v2){WINDOW_WIDTH / 2, WINDOW_HEIGHT};
    }

    double signed_angle_to_forward = v2_signed_angle_between(playerForward, v2_sub(pos, player->world_node.pos));

    double signed_angle_degrees = rad_to_deg(signed_angle_to_forward);

    if (!allow_out_of_screen && !in_range(signed_angle_degrees, -0.5 * fov, 0.5 * fov)) {
        return OUT_OF_SCREEN_POS;
    } else {
        if (!in_range(signed_angle_degrees, -90, 90)) {
            double dist_to_viewplane = abs(v2_distance(pos, player->world_node.pos) * v2_cos_angle_between(playerForward, v2_sub(pos, player->world_node.pos)));

            pos = v2_add(pos, v2_mul(playerForward, to_vec(dist_to_viewplane + 2)));

            // we need to recalculate prev variables
            signed_angle_to_forward = v2_signed_angle_between(playerForward, v2_sub(pos, player->world_node.pos));

            signed_angle_degrees = deg_to_rad(signed_angle_to_forward);
        }
    }


    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->world_node.pos));

    double dist_to_player = v2_distance(pos, player->world_node.pos);


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
    double y_pos = WINDOW_HEIGHT / 2 + wallSize / 2 - ((height - get_player_height() + get_max_height() / 2) + WINDOW_HEIGHT / 2) / dist_to_viewplane;

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

    return v2_add(player->world_node.pos, v2_mul(rayDir, to_vec(dist)));
}

struct floor_and_ceiling_thread_data {
    int start_row;
    int end_row; // exclusive
    void **pixels;
};

void render_floor_and_ceiling() {
    // Use the shader for everything.

    if (tilemap_image == NULL || lightmap_image == NULL || player == NULL) {
        return;
    }

    static int times_called = 0;
    times_called++;

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

    //drawSkybox();

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

    GPU_Blit(floorAndCeiling, NULL, image_target, cameraOffset.x, cameraOffset.y);
    


    GPU_FreeTarget(image_target);
    
    GPU_BlitRect(floor_and_ceiling_target_image, NULL, screen, NULL);


    GPU_DeactivateShaderProgram();
}

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light, SDL_Color custom_color) {
    

    v2 screen_pos = worldToScreen(pos, height, false);

    if (v2_equal(screen_pos, OUT_OF_SCREEN_POS)) {
        return;
    }

    double dist_to_player = v2_distance(pos, player->world_node.pos);

    v2 final_size = world_to_screen_size(size, pos, height);

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

void renderDirSprite(DirSprite *dSprite, v2 pos, v2 size, double height) {
    GPU_Image *texture = get_sprite_current_texture(dir_sprite_current_sprite(dSprite, pos));
    renderTexture(texture, pos, size, height, true, (SDL_Color){255, 255, 255, 255});
}

RenderObject getWallStripe(int i) {
    v2 ray_dir = getRayDirByIdx(i);

    RayCollisionData data = castRay(player->world_node.pos, ray_dir);

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

RenderObject *get_render_list() {
    RenderObject *renderList = array(RenderObject, RESOLUTION_X + get_node_count() - 1);

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

    iter_over_all_nodes(node, {

        RenderObject render_object = (RenderObject){.isnull = false};
        v2 pos = V2_ZERO;

        bool custom_dist = false;
        
        if (instanceof(node->type, WORLD_NODE)) {

            pos = ((WorldNode *)node)->pos;

        }
        if (instanceof(node->type, SPRITE) || instanceof(node->type, DIR_SPRITE)) {
            if (node->parent != NULL) {
                
                if (instanceof(node->parent->type, WORLD_NODE)) {
                    pos = ((WorldNode *)node->parent)->pos;
                } else if (instanceof(node->parent->type, CANVAS_NODE)) {
                    custom_dist = true;
                    render_object.dist_squared = -((CanvasNode *)node->parent)->z_index; 
                }
                
            }
        }
        if (instanceof(node->type, LINE)) {// this is the last edge case i swear
            Line *line = node;
            double d1 = v2_distance_squared(line->p1, player->world_node.pos);
            double d2 = v2_distance_squared(line->p2, player->world_node.pos);

            if (d1 < d2) pos = line->p1;
            else pos = line->p2;
        }

        if (instanceof(node->type, CANVAS_NODE)) {
            custom_dist = true;
            render_object.dist_squared = -((CanvasNode *)node->parent)->z_index;
        }

        if (!custom_dist) {
            render_object.dist_squared = v2_distance_squared(pos, player->world_node.pos);
        }
        render_object.val = node;
        render_object.type = NODE;
        
        array_append(renderList, render_object);
        
    });

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

    for (int i = 0; i < 3; i++) {
        rgb[i] = SDL_clamp(rgb[i], 0, 255);
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
    
    // because it looks kinda ass
    double normal_modifier = 0;//v2_get_angle(stripe->normal) * 10;

    int rgb[3] = {
        125 * baked_light_color.r * brightness + normal_modifier,
        125 * baked_light_color.g * brightness + normal_modifier,
        125 * baked_light_color.b * brightness + normal_modifier
    };
    
    clampColors(rgb);


    

    GPU_SetRGB(texture, rgb[0], rgb[1], rgb[2]);
    GPU_BlitRect(texture, &srcRect, screen, &dstRect);

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

void drawSkybox() {
    GPU_Image *tex = skybox_texture;

    double x = loop_clamp(player->angle / startFov * WINDOW_WIDTH, 0, WINDOW_WIDTH);

    double yOffsets = -player->pitch;

    GPU_Rect skybox_rect = {-x, yOffsets, WINDOW_WIDTH * 2, WINDOW_HEIGHT / 2};

    GPU_BlitRect(tex, NULL, screen, &skybox_rect);
}

void render(double delta) {  // #RENDER

    if (loading_map || !ready_to_render) {
        return;
    }

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

    Node_render(renderer);

    GPU_ActivateShaderProgram(bloom_shader, &bloom_shader_block);

    GPU_SetShaderImage(screen_image, GPU_GetUniformLocation(bloom_shader, "u_screenTexture"), 1);
    
    float res[2] = {WINDOW_WIDTH, WINDOW_HEIGHT};

    GPU_SetUniformfv(GPU_GetUniformLocation(bloom_shader, "u_resolution"), 2, 1, res);


    GPU_Blit(screen_image, NULL, actual_screen, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);


    GPU_DeactivateShaderProgram();

    GPU_Blit(hud_image, NULL, actual_screen, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

    GPU_Flip(actual_screen);
} // #RENDER END

// #PLAYER INIT
void init_player(v2 pos) {
    if (player == NULL) player = malloc(sizeof(Player));

    player->world_node = new(WorldNode, WORLD_NODE);
    player->world_node.node.on_tick = player_tick;
    player->world_node.node.type = PLAYER;

    Node_add_child(player, alloc(CircleCollider, CIRCLE_COLLIDER, PLAYER_COLLIDER_RADIUS));



    player->angle = 0;
    player->world_node.pos = pos;
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

    player->maxHealth = 10;
    player->health = player->maxHealth;
    player->tallness = 11000;
    player->world_node.height = player->tallness;
    player->height_vel = 0;
    player->crouching = false;

    player->handOffset = V2_ZERO;
    player->pitch = 0;

    player->primary = NULL;
    player->secondary = NULL;
    player->utility = NULL;
    player->special = NULL;

    randomize_player_abilities();
}

// CLEAR
RayCollisionData ray_node(Raycast ray, Node *node) {

    RayCollisionData ray_data = {0};

    switch (node->type) {
        case (int)CIRCLE_COLLIDER:
            ray_data = ray_circle(ray, node);
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
            int t = tilemap->level_tilemap[(int)currentCell.y][(int)currentCell.x];

            if (t != -1) {
                found = true;
                tile = t;
            }
        }
    }

    RayCollisionData data = {0};
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

RayCollisionData ray_circle(Raycast ray, CircleCollider *collider) {
    
    RayCollisionData data = {0};

    if (v2_distance(ray.pos, collider->world_node.pos) <= collider->radius || v2_dot(ray.dir, v2_dir(ray.pos, collider->world_node.pos)) < 0) {
        data.hit = false;
        return data;
    }

    v2 rayToCircle = v2_sub(collider->world_node.pos, ray.pos);
    double a = v2_dot(rayToCircle, ray.dir);
    double cSquared = v2_length_squared(rayToCircle);
    double bSquared = cSquared - a * a;  // pythagoras
    if (collider->radius * collider->radius - bSquared < 0) {
        data.hit = false;
        return data;
    }                                                           // no imaginary numbers pls
    double d = sqrt(collider->radius * collider->radius - bSquared);  // more pythagoras

    // raypos + raydir * (a - d)
    v2 collision_pos = v2_add(ray.pos, v2_mul(ray.dir, to_vec(a - d)));  // woohoo!

    v2 normal = v2_dir(collider->world_node.pos, collision_pos);
    double collIdx = v2_get_angle(v2_dir(collider->world_node.pos, data.collpos)) / (2 * PI);

    data.hit = true;
    data.normal = normal;
    data.startpos = ray.pos;
    data.collpos = collision_pos;
    data.wallWidth = collider->radius * PI;
    data.collider = collider;
    
    data.collIdx = collIdx;

    return data;
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

void reset_tilemap(int t[TILEMAP_HEIGHT][TILEMAP_WIDTH]) { 

    // printf("Reseting tilemap \n");

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            t[r][c] = -1;
        }
    }
}

void clear_level() {
    for (int i = array_length(game_node->children) - 1; i >= 0; i--) {
        Node_delete(game_node->children[i]);
    }
}

void spawn_floor_light(v2 pos) {
    LightPoint *light = alloc(LightPoint, LIGHT_POINT);

    light->color = (SDL_Color){255, 50, 50};
    light->strength = 4;
    light->radius = 140;
    light->pos = pos;
    Node_add_child(game_node, light);
}

void spawn_ceiling_light(v2 pos) {
    LightPoint *light = alloc(LightPoint, LIGHT_POINT);
    
    randomize();
    int chance = randi_range(0, 2);

    if (chance == 0) {
        light->color = (SDL_Color){randf_range(200, 255), randf_range(130, 160), 70};//{255, 200, 100};
    } else if (chance == 1) {
        light->color = (SDL_Color){160, 160, 255};
    } else {
        light->color = (SDL_Color){120, 255, 120};
    }
    
    light->strength = 5;
    light->radius = 400;
    light->pos = pos;
    Node_add_child(game_node, light);
}

void place_entity(v2 pos, int type) {
    switch (type) {
        case (int)P_PLAYER:
            if (player == NULL) printf("player = null \n");
            player->world_node.pos = pos;
            spawn_point = pos;
            break;
    }
}

void load_level(char *file) {
    clear_level();

    reset_tilemap(tilemap->level_tilemap);
    reset_tilemap(tilemap->floor_tilemap);
    reset_tilemap(tilemap->ceiling_tilemap);

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
            tilemap->floor_tilemap[r][c] = data[data_ptr++];
            if (tilemap->floor_tilemap[r][c] == P_FLOOR_LIGHT) {
                v2 tile_mid = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                spawn_floor_light(tile_mid);
            }
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            tilemap->level_tilemap[r][c] = data[data_ptr++];
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            tilemap->ceiling_tilemap[r][c] = data[data_ptr++];


            if (tilemap->ceiling_tilemap[r][c] == P_CEILING_LIGHT) {
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

            int floorTile = tilemap->floor_tilemap[row][col] == -1? 0 : tilemap->floor_tilemap[row][col];
            int ceilingTile = tilemap->ceiling_tilemap[row][col] == -1? 0 : tilemap->ceiling_tilemap[row][col];

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

void animation_tick(Animation *anim, double delta) {
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

void spritePlayAnim(Sprite *sprite, int idx) {
    bool less_priority = sprite->current_anim_idx != -1 
    && sprite->animations[sprite->current_anim_idx].priority < sprite->animations[idx].priority
    && sprite->animations[sprite->current_anim_idx].playing;
    if (less_priority) {
        // return;
    }
    sprite->current_anim_idx = idx;
    for (int i = 0; i < array_length(sprite->animations); i++) {
        sprite->animations[i].playing = false;
    }
    sprite->animations[idx].frame = 0;
    sprite->animations[idx].timeToNextFrame = 1 / sprite->animations[idx].fps;
    sprite->animations[idx].playing = true;
}

GPU_Image *get_sprite_current_texture(Sprite *sprite) {
    if (!sprite->isAnimated) {
        return sprite->texture;
    } else {
        if (sprite->current_anim_idx == -1) return NULL;
        Animation current = sprite->animations[sprite->current_anim_idx];
        return current.frames[current.frame];
    }
}

// Todo: add shoot cooldown for base shooting
void ability_shoot_activate(Ability *ability) {
    play_sound(player_default_shoot, 0.4);
    _shoot(0);
}

void Effect_tick(Node *node, double delta) {

    Effect *effect = node;

    effect->life_timer -= delta;
    if (effect->life_timer <= 0) {
        Node_queue_deletion(node);
        return;
    }
    // spriteTick(effect->entity.sprite, delta);
}

RayCollisionData castRayForEntities(v2 pos, v2 dir) {
    Raycast ray = {pos, dir};

    RayCollisionData data = {0};
    double min_sqr_dist = 99999999999;

    iter_over_all_nodes(node, {

        if (node->type != CIRCLE_COLLIDER) continue;



        RayCollisionData coll_data = ray_circle(ray, node);
        
        if (!coll_data.hit) continue;

        double sqr_dist = v2_distance_squared(pos, coll_data.collpos);

        if (sqr_dist < min_sqr_dist) {
            min_sqr_dist = sqr_dist;
            data = coll_data;
        }
        
        data = coll_data;

    }); 

    return data;
}

RayCollisionData castRayForAll(v2 pos, v2 dir) {
    RayCollisionData entity_ray_data = castRayForEntities(pos, dir);
    RayCollisionData wall_ray_data = castRay(pos, dir);

    if (!wall_ray_data.hit && !entity_ray_data.hit) return (RayCollisionData){0};

    if (!entity_ray_data.hit) return wall_ray_data;
    if (!wall_ray_data.hit) return entity_ray_data;

    double entity_squared_dist = v2_distance_squared(pos, entity_ray_data.collpos);
    double wall_squared_dist = v2_distance_squared(pos, wall_ray_data.collpos);

    if (entity_squared_dist < wall_squared_dist) return entity_ray_data;
    return wall_ray_data;
}

Sprite *dir_sprite_current_sprite(DirSprite *dSprite, v2 spritePos) {
    double player_angle_to_sprite = v2_get_angle(v2_dir(player->world_node.pos, spritePos));

    if (dSprite->dirCount < 2) {
        return &dSprite->sprites[0];
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

        return &dSprite->sprites[min_idx];
    }

    return NULL;
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

bool pos_in_tile(v2 pos) {
    int row = pos.y / tileSize;
    int col = pos.x / tileSize;

    if (!in_range(row, 0, TILEMAP_HEIGHT - 1) || !in_range(col, 0, TILEMAP_WIDTH - 1)) return false;

    return tilemap->level_tilemap[row][col] == P_WALL;
}

CollisionData getCircleTileCollision(CircleCollider *circle, v2 tilePos) {
    CollisionData result = {0};

    v2 clampedPos;
    clampedPos.x = clamp(circle->world_node.pos.x, tilePos.x, tilePos.x + tileSize);
    clampedPos.y = clamp(circle->world_node.pos.y, tilePos.y, tilePos.y + tileSize);

    v2 toClamped = v2_sub(clampedPos, circle->world_node.pos);
    double dist = v2_length(toClamped);
    if (dist == 0) {
        return result;
    }
    double overlap = circle->radius - dist;
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

CollisionData getCircleTileMapCollision(CircleCollider *circle) {
    CollisionData result;
    result.didCollide = false;
    result.offset = (v2){0, 0};

    v2 gridCheckStart;
    v2 gridCheckEnd;

    gridCheckStart.x = (int)((circle->world_node.pos.x - circle->radius) / tileSize) - 1;
    gridCheckStart.y = (int)((circle->world_node.pos.y - circle->radius) / tileSize) - 1;

    gridCheckEnd.x = (int)((circle->world_node.pos.x + circle->radius) / tileSize) + 2;  //+ 2 to account for rounding up
    gridCheckEnd.y = (int)((circle->world_node.pos.y + circle->radius) / tileSize) + 2;

    v2 gridCheckSize = v2_sub(gridCheckEnd, gridCheckStart);

    for (int row = gridCheckStart.y; row < gridCheckEnd.y; row++) {
        for (int col = gridCheckStart.x; col < gridCheckEnd.x; col++) {
            if (!in_range(row, 0, TILEMAP_HEIGHT - 1) || !in_range(col, 0, TILEMAP_WIDTH - 1)) continue;
            if (tilemap->level_tilemap[row][col] == -1) continue;
            CollisionData data = getCircleTileCollision(circle, (v2){col * tileSize, row * tileSize});
            if (data.didCollide) {
                result.didCollide = true;
                result.offset = v2_add(result.offset, data.offset);
            }
        }
    }

    return result;
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
GPU_Image **get_texture_files(char *file_name, int file_count) {

    GPU_Image **textures = malloc(sizeof(GPU_Image *) * file_count);

    int charCount = get_num_digits(file_count);

    for (int i = 0; i < file_count; i++) {
        char num[charCount + 10];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(file_name, num);
        char *fileWithExtension = concat(fileWithNum, ".png");

        GPU_Image *tex = load_texture(fileWithExtension);
        textures[i] = tex;

        free(fileWithNum);
        free(fileWithExtension);
    }

    return textures;
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

    iter_over_all_nodes(node, {
        if (node->type == LIGHT_POINT) {
            has_lights = true;
            break;
        }
    });

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
            && tilemap->level_tilemap[tilemap_row][tilemap_col] == P_WALL;

            if (is_in_wall) continue;

            int calc_row = ((int)(r * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;
            int calc_col = ((int)(c * CALC_RES / BAKED_LIGHT_RESOLUTION)) * BAKED_LIGHT_RESOLUTION / CALC_RES;

            bool should_calculate = r == calc_row && c == calc_col;

            if (!should_calculate) {
                baked_light_grid[r][c] = baked_light_grid[calc_row][calc_col];
                continue;
            }

            const int calc_tile_size = BAKED_LIGHT_RESOLUTION / CALC_RES;

            int k = array_length(game_node->children);
            
            int light_count = 0;
            int total_nodes_count = 0;

            iter_over_all_nodes(node, {

                total_nodes_count++;

                if (node->type != LIGHT_POINT) {
                    continue;
                }
                LightPoint *point = node;
                light_count += 1;

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

                baked_light_grid[r][c].r = SDL_clamp(baked_light_grid[r][c].r + col.r, ambient_light, MAX_LIGHT);
                baked_light_grid[r][c].g = SDL_clamp(baked_light_grid[r][c].g + col.g, ambient_light, MAX_LIGHT);
                baked_light_grid[r][c].b = SDL_clamp(baked_light_grid[r][c].b + col.b, ambient_light, MAX_LIGHT);
            });



            
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
    play_sound(player_default_hurt, 0.1);
    shakeCamera(20, 10, true, 2);

    // player_hit_particles->world_node.pos = player->world_node.pos;
    // player_hit_particles->world_node.height = player->world_node.height;
    // player_hit_particles->target = NULL;
    // particle_spawner_explode(player_hit_particles);

    double progress_to_death = (double)(player->maxHealth - player->health) / player->maxHealth; // when it reaches 1, youre cooked
    vignette_color = (SDL_Color){255 * progress_to_death, 0, 0, 255};

    if (player->health <= 0) {
        player_die();
    }
}

void player_die() {
    // play some dramatic ahh animation
    player->world_node.pos = spawn_point;
    player->vel = V2_ZERO;
    player->height_vel = 0;
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

int charge_time_to_shots(double charge_time) {
    return (int)(charge_time * 3);
}

void dir_sprite_play_anim(DirSprite *dir_sprite, int anim) {
    if (dir_sprite->current_anim == anim) return;
    
    for (int i = 0; i < dir_sprite->dirCount; i++) {
        dir_sprite->sprites[i].animations[anim].frame = 0;
        dir_sprite->sprites[i].current_anim_idx = anim;
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
        .tick = NULL,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = 0.5,
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

    bool waiting_delay = ability->delay_timer > 0;
    if (waiting_delay) {
        ability->delay_timer -= delta;
        if (ability->delay_timer <= 0) {
            ability->delay_timer = -1;
            ability->activate(ability);
        }
    }
}

Ability ability_secondary_shoot_create() {
    return (Ability) {
        .activate = ability_secondary_shoot_activate,
        .tick = NULL,
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
    play_sound(rapidfire_sound, 0.1);

    Ability *rapid_fire = (Ability *)ability;
    int shots = 6;


    while (shots-- > 0) _shoot(0.05);
    

    player->height_vel = -1;
    player->vel = v2_mul(playerForward, to_vec(-5));
}

void _shoot(double spread) { // the sound isnt attached bc shotgun makes eargasm

    double pitch = player->pitch + randf_range(1000 * -spread, 1000 * spread);

    
    shakeCamera(20, 4, true, 1);
    spritePlayAnim(get_child_by_type(hand_node, SPRITE), 1);

    v2 shoot_dir = v2_rotate(playerForward, randf_range(-PI * spread, PI * spread));

    v2 effect_size = to_vec(8000);

    RayCollisionData ray_data = castRayForAll(player->world_node.pos, shoot_dir);

    if (ray_data.collider != NULL && node(ray_data.collider)->parent->type == PROJECTILE) {
        projectile_destroy(node(ray_data.collider)->parent);
    }


    v2 hit_pos = screenToFloor((v2){RESOLUTION_X / 2, WINDOW_HEIGHT / 2 + pitch});
    double distance_to_hit_pos = v2_distance(hit_pos, player->world_node.pos);
    double distance_to_coll_pos = ray_data.hit? v2_distance(ray_data.collpos, player->world_node.pos) : 999999999999;

    v2 final_pos;
    bool is_ceiling = pitch < 0;
    double final_height = is_ceiling? get_max_height() : 0;


    if (distance_to_coll_pos < distance_to_hit_pos) {
        final_pos = ray_data.collpos;
        final_height = lerp( get_player_height(), final_height, inverse_lerp(0, distance_to_hit_pos, distance_to_coll_pos));
    } else {
        final_pos = hit_pos;
    }

    double size = 8000;
    Effect *hit_effect = alloc(Effect, EFFECT, 1);
    hit_effect->entity.world_node.pos = final_pos;
    hit_effect->entity.world_node.height = final_height;
    hit_effect->entity.world_node.size = to_vec(size);

    Sprite *sprite = alloc(Sprite, SPRITE, true);

    Animation anim = create_animation(5, 1, shootHitEffectFrames);

    anim.fps = 12;
    anim.loop = false;

    array_append(sprite->animations, anim);

    spritePlayAnim(sprite, 0);

    Node_add_child(hit_effect, sprite);

    Node_add_child(game_node, hit_effect);

    // ray effect
    Effect *ray_effect = alloc(Effect, EFFECT, 0.3);
    Line *line = alloc(Line, LINE);
    v2 gun_offset = V2(5, 2);
    double gun_offset_h = -700 + -gun_offset.x * player->pitch;
    v2 final_offset = v2_rotate(gun_offset, v2_get_angle(playerForward));
    line->p1 = player->world_node.pos;
    line->p1 = v2_add(line->p1, final_offset);
    line->p2 = final_pos;
    line->h1 = get_player_height() + gun_offset_h;
    line->h2 = final_height;
    line->texture = shoot_ray;
    line->fade = true;
    Node_add_child(ray_effect, line);

    Node_add_child(game_node, ray_effect);


    
    struct ability_shoot_packet packet_data = {
        .hit_pos = final_pos,
        .hit_height = final_height,
        .shooter_id = client_self_id,
        .hit_id = -1
    };
    if (ray_data.hit) {
        if (ray_data.collider != NULL && ray_data.collider->parent->type == PLAYER_ENTITY) {

            PlayerEntity *player_entity = ray_data.collider->parent;

            
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

ParticleSpawner ParticleSpawner_new(bool active) {
    ParticleSpawner spawner = {
        .dir = (v2){1, 0},
        .height_dir = 0,
        .accel = (v2){0, 0},
        .height_accel = PARTICLE_GRAVITY,
        .spawn_rate = 20,
        .spread = 0.1,
        .min_speed = 10,
        .max_speed = 10,
        .min_size = to_vec(500),
        .max_size = to_vec(1500),
        .floor_drag = 0.01,
        .bounciness = 0.5,
        .particle_lifetime = 1,
        .target = NULL,
        .fade = true,
        .fade_scale = true,
        .affected_by_light = true,
        .explode_particle_amount = 20,
        .damp = 1,
        .radial_accel = V2_ZERO,
        .height_radial_accel = 0
    };

    spawner.world_node = new(WorldNode, WORLD_NODE);
    spawner.world_node.pos = V2_ZERO;
    spawner.world_node.height = 0;

    spawner.sprite_idx = -1;

    spawner.target = NULL;

    spawner.spawn_timer = 1.0 / spawner.spawn_rate;

    node(&spawner)->on_tick = particle_spawner_tick;

    spawner.active = active;

    return spawner;
}

void particle_spawner_tick(Node *node, double delta) {

    ParticleSpawner *spawner = node;

    if (!spawner->active) return;
    spawner->spawn_timer += delta;
    double thresh = 1.0 / spawner->spawn_rate;
    if (spawner->spawn_timer >= thresh) {
        int amount = spawner->spawn_timer / thresh;
        spawner->spawn_timer = 0;
        for (int i = 0; i < amount; i++) { // i do it this way so low fps still outputs more than 30 particles/frame
            particle_spawner_spawn(spawner);
        }
    }
}

void particle_spawner_spawn(ParticleSpawner *spawner) {

    Particle *particle = alloc(Particle, PARTICLE, spawner->particle_lifetime);
    
    v2 pos = spawner->world_node.pos;
    double height = spawner->world_node.height;

    if (node(spawner)->parent != NULL && instanceof(node(spawner)->parent->type, WORLD_NODE)) {
        pos = v2_add(pos, ((WorldNode *)node(spawner)->parent)->pos);
        height += ((WorldNode *)node(spawner)->parent)->height;
    }

    double speed = randf_range(spawner->min_speed, spawner->max_speed);

    double dir3d_length = sqrt(spawner->dir.x * spawner->dir.x + spawner->dir.y * spawner->dir.y + spawner->height_dir * spawner->height_dir);

    double dir3d_x = spawner->dir.x / dir3d_length;
    double dir3d_y = spawner->dir.y / dir3d_length;
    double dir3d_z = spawner->height_dir / dir3d_length;

    double yaw = v2_get_angle(V2(dir3d_x, dir3d_y));
    double pitch = v2_get_angle(V2(dir3d_x, dir3d_z));

    yaw += (spawner->spread / 180 * PI) * randf_range(-1, 1);
    pitch += (spawner->spread / 180 * PI) * randf_range(-1, 1);

    double new_x = cos(yaw) * cos(pitch);
    double new_y = sin(yaw) * cos(pitch);
    double new_z = sin(pitch);
    
    double vel_x = new_x * speed;
    double vel_y = new_y * speed;
    double vel_z = new_z * speed * XY_TO_HEIGHT; // bc height is different like that

    particle->vel = (v2){vel_x, vel_y};
    particle->h_vel = vel_z;

    particle->effect.entity.affected_by_light = spawner->affected_by_light;

    particle->accel = spawner->accel;
    particle->h_accel = spawner->height_accel;

    particle->effect.life_time = spawner->particle_lifetime;
    particle->effect.life_timer = particle->effect.life_time;

    particle->start_color = spawner->start_color;
    particle->end_color = spawner->end_color;

    particle->bounciness = spawner->bounciness;
    particle->floor_drag = spawner->floor_drag;

    particle->damp = spawner->damp;
    particle->radial_accel = spawner->radial_accel;
    particle->height_radial_accel = spawner->height_radial_accel;

    particle->effect.entity.world_node.pos = pos; // change later with emission
    particle->effect.entity.world_node.height = height;
    
    particle->fade = spawner->fade;
    particle->fade_scale = spawner->fade_scale;

    particle->initial_pos = pos;
    particle->initial_height = height;

    double size_rand = randf_range(0, 1);
    v2 size = v2_lerp(spawner->min_size, spawner->max_size, size_rand);
    
    particle->initial_size = size;


    Sprite *spawner_sprite = ParticleSpawner_get_sprite(spawner);
    if (spawner_sprite == NULL) {
        Sprite *particle_sprite = alloc(Sprite, SPRITE, false);
        particle_sprite->texture = default_particle_texture;
        Node_add_child(particle, particle_sprite);
    } else {
        Node_add_child(particle, copy_sprite(spawner_sprite, false));
    }

    Node_add_child(game_node, particle);

}


void Particle_tick(Node *node, double delta) { // #PT

    Particle *particle = node;

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
    particle->vel = v2_add(particle->vel, v2_mul(v2_rotate(particle->radial_accel, v2_get_angle(v2_dir(particle->effect.entity.world_node.pos, particle->initial_pos))), to_vec(delta)));
   
    particle->h_vel += particle->height_radial_accel * XY_TO_HEIGHT * sign(particle->initial_height - particle->effect.entity.world_node.height);
   
    particle->vel = v2_mul(particle->vel, to_vec((1.0 / particle->damp)));
    particle->h_vel *= (1.0 / particle->damp);

    if (particle->fade_scale) {
        particle->effect.entity.world_node.size = v2_lerp(particle->initial_size, V2_ZERO, inverse_lerp(particle->effect.life_time, 0, particle->effect.life_timer));
    } else {
        particle->effect.entity.world_node.size = particle->initial_size;
    }
    particle->effect.entity.world_node.pos = v2_add(particle->effect.entity.world_node.pos, v2_mul(particle->vel, to_vec(delta)));
    particle->effect.entity.world_node.height += particle->h_vel * delta;


    double floor_bound = particle->effect.entity.world_node.size.y / 2;
    double ceil_bound = get_max_height() - particle->effect.entity.world_node.size.y / 2;
    if (particle->effect.entity.world_node.height < floor_bound) {
        particle->effect.entity.world_node.height = floor_bound;
        particle->h_vel *= -particle->bounciness;
        particle->vel = v2_mul(particle->vel, to_vec(1 - particle->floor_drag));
    }
    if (particle->effect.entity.world_node.height > ceil_bound) {
        particle->effect.entity.world_node.height = ceil_bound;
        particle->h_vel *= -particle->bounciness;
    }


   

    Effect_tick((Effect *)particle, delta);
}

Ability ability_dash_create() {
    return (Ability) {
        .activate = ability_dash_activate,
        .tick = NULL,
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

    v2 pos = player->world_node.pos;
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

    player->world_node.pos = v2_add(player->world_node.pos, v2_mul(dir, to_vec(dash_distance)));
    player->height_vel = -2;
    player->vel = v2_mul(dir, to_vec(3));

}

void activate_ability(Ability *ability) {
    
    if (ability == NULL || !ability->can_use || paused) {
        if (ability == NULL) {
            commit_sudoku();
        }
        return;
    }


    ability->can_use = false;

    if (ability->before_activate != NULL) {
        ability->before_activate(ability);
        ability->delay_timer = ability->delay;
    } else {
        ability->activate(ability);
    }

}

void ability_dash_before_activate(Ability *ability) {
    CanvasNode *cnode = alloc(CanvasNode, CANVAS_NODE);

    cnode->size = V2(WINDOW_WIDTH, WINDOW_HEIGHT);
    cnode->z_index = -5;

    Sprite *sprite = alloc(Sprite, SPRITE, true);

    Animation anim = create_animation(6, 1, dash_screen_anim);
    anim.loop = false;
    anim.fps = 12;
    array_append(sprite->animations, anim);
    spritePlayAnim(sprite, 0);

    Node_add_child(cnode, sprite);

    Node_add_child(game_node, cnode);
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

    continue_button->custom_on_click = _continue_button_pressed;

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

    int num_lights = 0;
    
    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            tilemap->floor_tilemap[offset_r + r][offset_c + c] = data[data_ptr++];
            if (tilemap->floor_tilemap[offset_r + r][offset_c + c] == P_FLOOR_LIGHT) {
                v2 tile_mid = (v2){(offset_c + c + 0.5) * tileSize, (offset_r + r + 0.5) * tileSize};
                spawn_floor_light(tile_mid);
                num_lights++;
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

                tilemap->level_tilemap[offset_r + r][offset_c + c] = P_WALL;
            } else {
                tilemap->level_tilemap[offset_r + r][offset_c + c] = tile;
            }



        }
    }

    for (int r = 0; r < ROOM_HEIGHT; r++) {
        for (int c = 0; c < ROOM_WIDTH; c++) {
            tilemap->ceiling_tilemap[offset_r + r][offset_c + c ] = data[data_ptr++];


            if (tilemap->ceiling_tilemap[offset_r + r][offset_c + c ] == P_CEILING_LIGHT) {
                v2 tile_mid = (v2){(offset_c + c + 0.5) * tileSize, (offset_r + r + 0.5) * tileSize};
                spawn_ceiling_light(tile_mid);
                num_lights++;
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

    // printf("Loaded room with %d lights \n", num_lights);

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
            tilemap->level_tilemap[current_row][current_col] = -1;
        }
        for (int r = current_row; r != start_row + row_dist * dir_r; r += dir_r) {
            current_row = r;
            tilemap->level_tilemap[current_row][current_col] = -1;
            
        }
        for (int c = current_col; c != start_col + (col_dist + 1) * dir_c; c += dir_c) {
            current_col = c;
            tilemap->level_tilemap[current_row][current_col] = -1;
        }

    } else {
        for (int r = current_row; r != start_row + row_dist / 2; r += dir_r) {
            current_row = r;
            tilemap->level_tilemap[current_row][current_col] = -1;
        }
        for (int c = current_col; c != start_col + col_dist * dir_c; c += dir_c) {
            current_col = c;
            tilemap->level_tilemap[current_row][current_col] = -1;
        }
        for (int r = current_row; r != start_row + (row_dist + 1) * dir_r; r += dir_r) {
            current_row = r;
            tilemap->level_tilemap[current_row][current_col] = -1;
        }
    }

    tilemap->level_tilemap[current_row][current_col] = -1;
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

    clear_level();

    reset_tilemap(tilemap->level_tilemap);
    reset_tilemap(tilemap->floor_tilemap);
    reset_tilemap(tilemap->ceiling_tilemap);

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

            int floorTile = tilemap->floor_tilemap[row][col] == -1? 0 : tilemap->floor_tilemap[row][col];
            int ceilingTile = tilemap->ceiling_tilemap[row][col] == -1? 0 : tilemap->ceiling_tilemap[row][col];

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
    return player->crouching? player->world_node.height - player->tallness / 2 : player->world_node.height;
}


double set_time_scale_for_duration(double ts, double duration) {
    game_speed = ts;
    game_speed_duration_timer = duration;
}

bool is_player_on_floor() {
    return player->world_node.height <= player->tallness * 0.8;
}

// takes ownership of 'string'
void write_to_debug_label(String string) {
    UILabel_set_text(debug_label, string);
    UILabel_update(debug_label);
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
        // printf("Sending to the dude. ID: %d \n", server_client_id_list[i]);

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

// #SERVER RECV
void on_server_recv(SOCKET socket, MPPacket packet, void *data) {

    if (packet.len > MP_DEFAULT_BUFFER_SIZE) {
        printf("corrupted on_server_recv \n");
        commit_sudoku();
    }

    if (packet.type == PACKET_REQUEST_DUNGEON_SEED) {
        struct dungeon_seed_packet packet_data = {.seed = server_dungeon_seed};
        MPPacket seed_packet = {.type = PACKET_DUNGEON_SEED, .len = sizeof(packet_data), .is_broadcast = false};

        MPServer_send_to(seed_packet, &packet_data, socket);
        return;
    } 
    if (instanceof(packet.type, PACKETS_TO_SYNC)) {
        int sync_id = server_next_sync_id++;

        MPPacket packet = {.is_broadcast = false, .len = sizeof(struct send_sync_id_packet), .type = PACKET_SEND_SYNC_ID};
        struct send_sync_id_packet pdata = {.sync_id = sync_id};

        MPServer_send_to(packet, &pdata, socket);

        // sync id must always be first then
        *(int *)data = sync_id;

    }

    MPServer_send(packet, data);
}

void client_add_player_entity(int id) {

    PlayerEntity *player_entity = alloc(PlayerEntity, PLAYER_ENTITY);

    

    player_entity->id = id;
    player_entity->entity.affected_by_light = true;
    player_entity->entity.world_node.height = 0;
    player_entity->entity.world_node.pos = V2_ZERO;
    player_entity->entity.world_node.size = to_vec(10000);
    // player_entity->entity.sprite = createSprite(false, 0);
    // player_entity->entity.sprite->texture = entityTexture;
    player_entity->entity.color = (SDL_Color){255, 255, 255,  255};
    player_entity->desired_pos = V2_ZERO;
    player_entity->desired_height = 0;


    // Sprite *temp_sprite = alloc(Sprite, SPRITE, false);
    // temp_sprite->texture = entityTexture;

    // Node_add_child(player_entity, temp_sprite);

    Node_add_child(player_entity, alloc(CircleCollider, CIRCLE_COLLIDER, PLAYER_COLLIDER_RADIUS));

    DirSprite *dir_sprite = alloc(DirSprite, DIR_SPRITE, 16);

    DirSprite_add_animation(dir_sprite, 1, player_textures);

    dir_sprite_play_anim(dir_sprite, 0);

    Node_add_child(player_entity, dir_sprite);


    printf("Player joined! ID: %d \n", player_entity->id);
    write_to_debug_label(String_from_int(player_entity->id));

    Node_add_child(game_node, player_entity);


}

// #CLIENT RECV
void on_client_recv(MPPacket packet, void *data) {

    if (packet.len > MP_DEFAULT_BUFFER_SIZE) {
        printf("Packet too big! \n");
        return;
    }

    if (packet.type < 0 || packet.type > PACKETS_END) {
        printf("Invalid packet type! \n");
        return;
    }

    if (packet.type == PACKET_UPDATE_PLAYER_ID) {

        client_self_id = *(int *)(data);

    } else if (packet.type == PACKET_PLAYER_POS) {
        
        struct player_pos_packet packet_data = *(struct player_pos_packet *)data;

        if (packet_data.id == client_self_id) {
            return;
        }

        PlayerEntity *player_entity = find_or_add_player_entity_by_id(packet_data.id);

        if (player_entity == NULL) return;

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
            
        PlayerEntity *shooter = find_or_add_player_entity_by_id(packet_data->shooter_id);            

        if (shooter == NULL) return;

        // create the effect at the pos and height
        Effect *hit_effect = alloc(Effect, EFFECT, 1);

        hit_effect->entity.world_node.pos = packet_data->hit_pos;
        hit_effect->entity.world_node.height = packet_data->hit_height;
        hit_effect->entity.world_node.size = to_vec(8000);

        Sprite *sprite = alloc(Sprite, SPRITE, true);

        Animation anim = create_animation(5, 0, shootHitEffectFrames);

        anim.fps = 12;
        anim.loop = false;

        array_append(sprite->animations, anim);

        spritePlayAnim(sprite, 0);

        Node_add_child(hit_effect, sprite);


        Effect *ray_effect = alloc(Effect, EFFECT, 0.3);
        Line *line = alloc(Line, LINE);
        line->p1 = shooter->entity.world_node.pos;
        line->p2 = packet_data->hit_pos;
        line->h1 = shooter->entity.world_node.height;
        line->h2 = packet_data->hit_height;
        line->texture = shoot_ray;
        line->fade = true;
        line->width = 200;
        Node_add_child(ray_effect, line);

        Node_add_child(game_node, ray_effect);

        Node_add_child(game_node, hit_effect);


        if (packet_data->hit_id != -1) {

            if (packet_data->hit_id == client_self_id) {
                player_take_dmg(1);
                return;
            }

            PlayerEntity *player_entity = find_or_add_player_entity_by_id(packet_data->hit_id);
            if (player_entity == NULL) return;
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

        iter_over_all_nodes(node, {
            if (node->type != PLAYER_ENTITY) continue;

            PlayerEntity *player_entity = node;

            if (player_entity->id == packet_data->id) {
                Node_queue_deletion(node);
                return;
            }
        });
    } else if (packet.type == PACKET_ABILITY_BOMB) {
        struct ability_bomb_packet *packet_data = data;

        if (packet_data->sender_id == client_self_id) return;

        Projectile *bomb = create_bomb_projectile(packet_data->pos, packet_data->vel);
        bomb->entity.world_node.height = packet_data->height;
        bomb->height_vel = packet_data->height_vel;
        bomb->shooter_id = packet_data->sender_id;
        node(bomb)->sync_id = packet_data->sync_id;

        printf("Adding bomb with sync id %d ! \n\n\n\n\n\n\n\n\n\n\n", packet_data->sync_id);

        Node_add_child(game_node, bomb);

    } else if (packet.type == PACKET_ABILITY_SWITCHSHOT) {
        struct ability_switchshot_packet *packet_data = data;

        if (packet_data->sender_id == client_self_id) return;

        Projectile *switchshot = create_switchshot_projectile(packet_data->pos, packet_data->height, packet_data->vel, packet_data->h_vel);
        node(switchshot)->sync_id = packet_data->sync_id;
        switchshot->shooter_id = packet_data->sender_id;

        Node_add_child(game_node, switchshot);
     
    } else if (packet.type == PACKET_SEND_SYNC_ID) {
        printf("Received sync id %d! \n", ((struct send_sync_id_packet *)data)->sync_id);
        use_sync_id(((struct send_sync_id_packet *)data)->sync_id);

    } else if (packet.type == PACKET_SYNC_PROJECTILE) {

        if (MP_is_server) return;

        struct sync_projectile_packet *packet_data = data;

        // cd_print(true, "syncing projectile with sync id %d ! \n", packet_data->sync_id);

        Projectile *sync_projectile = find_node_by_sync_id(packet_data->sync_id);

        if (sync_projectile == NULL) {
            printf("projectile with sync id %d doesnt exist. \n", packet_data->sync_id);
            return;
        }

        sync_projectile->entity.world_node.pos = packet_data->pos;
        sync_projectile->entity.world_node.height = packet_data->height;
        sync_projectile->vel = packet_data->vel;
        sync_projectile->height_vel = packet_data->h_vel;

    } else if (packet.type == PACKET_ABILITY_FORCEFIELD) {
        struct ability_forcefield_packet *packet_data = data;

        if (packet_data->sender_id == client_self_id) return;

        Projectile *ff = create_forcefield_projectile();
        ff->entity.world_node.pos = packet_data->pos;
        ff->entity.world_node.height = packet_data->height;
        ff->vel = packet_data->vel;
        ff->height_vel = packet_data->h_vel;
        node(ff)->sync_id = packet_data->sync_id;
        ff->shooter_id = packet_data->sender_id;

        Node_add_child(game_node, ff);
    } else if (packet.type == PACKET_SWITCH_POSITIONS) {
        struct switch_positions_packet *packet_data = data;

        if (packet_data->id1 == client_self_id) {
            player->world_node.pos = packet_data->pos2;
            player->world_node.height = packet_data->h2;
        } else if (packet_data->id2 == client_self_id) {
            player->world_node.pos = packet_data->pos1;
            player->world_node.height = packet_data->h1;
        }
    }
}

void PlayerEntity_tick(PlayerEntity *player_entity, double delta) {

    double real_desired_height = player_entity->desired_height - player_entity->entity.world_node.size.y / 2;

    player_entity->entity.world_node.pos = v2_lerp(player_entity->entity.world_node.pos, player_entity->desired_pos, 0.1 * (delta * 144));
    player_entity->entity.world_node.height = lerp(player_entity->entity.world_node.height, real_desired_height, 0.1 * (delta * 144));

    DirSprite *dir_sprite = get_child_by_type(player_entity, DIR_SPRITE);

    dir_sprite->dir = player_entity->dir;

}


// #TEXTURES INIT
void init_textures() {

    switchshot_icon = load_texture("Textures/Abilities/Icons/switchshot_icon.png");

    switchshot_anim = get_texture_files("Textures/Abilities/Switchshot/switchshot", 4);

    bomb_smoke_particles = get_texture_files("Textures/ExplosionSmokeParticles/smoke", 8);

    bomb_explosion_particles = get_texture_files("Textures/ExplosionParticles/explosion_particle", 11);

    shoot_ray = load_texture("Textures/shoot_ray.png");

    forcefield_field = get_texture_files("Textures/Abilities/Forcefield/pulsate", 5);

    hand_shoot = get_texture_files("Textures/rightHandAnim/rightHandAnim", 6);

    forcefield_anim = get_texture_files("Textures/Abilities/Forcefield/forcefield", 3);
    

    player_textures = get_texture_files("Textures/Player/player", 16);
    

    bomb_icon = load_texture("Textures/Abilities/Icons/bomb_icon.png");

    bomb_anim = get_texture_files("Textures/Abilities/Bomb/bomb", 4);
   

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

    dash_screen_anim = get_texture_files("Textures/Abilities/Dash/screen_anim", 6);
    

    // dash_anim_sprite = createSprite(true, 1);
    // dash_anim_sprite->animations[0] = create_animation(6, 10, dash_screen_anim);
    // dash_anim_sprite->animations[0].loop = false;
    // dash_anim_sprite->animations[0].fps = 10;
    // dash_anim_sprite->animations[0].frame = 5;


    dash_icon = load_texture("Textures/Abilities/Icons/dash_icon.png");

    ability_icon_frame = load_texture("Textures/Abilities/Icons/icon_frame.png");

    shotgun_icon = load_texture("Textures/Abilities/Icons/shotgun_icon.png");

    shoot_icon = load_texture("Textures/Abilities/Icons/shoot_icon.png");

    default_particle_texture = load_texture("Textures/base_particle.png");

    player_default_hurt = create_sound("Sounds/player_default_hurt.wav");

    player_default_shoot = create_sound("Sounds/player_default_shoot.wav");

    mimran_jumpscare = load_texture("Textures/scary_monster2.png");

    healthbar_texture = load_texture("Textures/health_bar1.png");

    vignette_texture = load_texture("Textures/vignette.png");

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

    crosshair = load_texture("Textures/crosshair.png");

    // leftHandSprite = createSprite(true, 2);
    // GPU_Image **default_hand = malloc(sizeof(GPU_Image *)); 
    // default_hand[0] = load_texture("Textures/rightHandAnim/rightHandAnim6.png");
    // leftHandSprite->animations[0] = create_animation(1, 0, default_hand);


    // leftHandSprite->animations[1] = create_animation(6, 0, NULL);
    // get_texture_files("Textures/rightHandAnim/rightHandAnim", 6, &leftHandSprite->animations[1].frames);
    // leftHandSprite->animations[1].fps = 12;

    // leftHandSprite->animations[1].loop = false;
    // spritePlayAnim(leftHandSprite, 0);

    shootHitEffectFrames = get_texture_files("Textures/ShootEffectAnim/shootHitEffect", 5);

    entityTexture = load_texture("Textures/scary_monster.png");

    skybox_texture = load_texture("Textures/skybox.png");

    printf("Initialized textures! \n");

}

PlayerEntity *find_player_entity_by_id(int id) {
    iter_over_all_nodes(node, {
        if (node->type == PLAYER_ENTITY) {
            PlayerEntity *player_entity = node;
            if (player_entity->id == id) {
                return player_entity;
            }
        }
    });

    return NULL;
}

PlayerEntity *find_or_add_player_entity_by_id(int id) {

    if (!is_sync_id_valid(id)) return NULL;

    if (id == client_self_id) return NULL;

    PlayerEntity *player_entity = find_player_entity_by_id(id);

    if (player_entity != NULL) return player_entity;

    client_add_player_entity(id);

    client_last_seen_sync_id = max(client_last_seen_sync_id, id);

    return find_player_entity_by_id(id);
}


void player_entity_take_dmg(PlayerEntity *player_entity, double dmg) {

    // player_hit_particles->world_node.pos = player_entity->entity.world_node.pos;
    // player_hit_particles->world_node.height = player_entity->entity.world_node.height;
    // player_hit_particles->target = NULL;
    // particle_spawner_explode(player_hit_particles);

}

void ability_bomb_activate() {
    Projectile *bomb = create_bomb_projectile(v2_add(player->world_node.pos, v2_mul(playerForward, to_vec(15))), v2_mul(playerForward, to_vec(1.4)));
    bomb->entity.world_node.height = get_player_height();

    Node_add_child(game_node, bomb);
    add_to_sync_queue(bomb);


    MPPacket packet = {.type = PACKET_ABILITY_BOMB, .len = sizeof(struct ability_bomb_packet), .is_broadcast = true};

    struct ability_bomb_packet packet_data = {
        .pos = bomb->entity.world_node.pos, 
        .height = bomb->entity.world_node.height, 
        .vel = bomb->vel, 
        .height_vel = bomb->height_vel, 
        .sender_id = client_self_id,
        .sync_id = -1
    };

    MPClient_send(packet, &packet_data);


}

Ability ability_bomb_create() {
    Ability ability = {
        .activate = ability_bomb_activate,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = .2,
        .delay = 0,
        .delay_timer = 0,
        .texture = bomb_icon,
        .tick = NULL,
        .timer = .2,
        .type = A_SECONDARY
    };

    return ability;
}

void Projectile_tick(Node *node, double delta) {

    Projectile *projectile = node;

    if (!projectile->_created && projectile->on_create != NULL) {
        projectile->on_create(projectile);
        projectile->_created = true;
    }

    projectile->vel = v2_add(projectile->vel, v2_mul(projectile->accel, to_vec(delta * 144)));
    projectile->height_vel += projectile->height_accel * delta;

    projectile->entity.world_node.pos = v2_add(projectile->entity.world_node.pos, v2_mul(projectile->vel, to_vec(delta * 144)));
    projectile->entity.world_node.height += projectile->height_vel * delta * 144;

    if (projectile->entity.world_node.height - projectile->entity.world_node.size.y / 2 <= 0) {
        if (projectile->destroy_on_floor) {
            projectile_destroy(projectile);
            return;
        }

        projectile->height_vel *= -projectile->bounciness;
        projectile->entity.world_node.height = projectile->entity.world_node.size.y / 2;
    }
    if (projectile->entity.world_node.height + projectile->entity.world_node.size.y / 2 >= get_max_height()) {
        
        if (projectile->destroy_on_ceiling) {
            projectile_destroy(projectile);
            return;
        }
        
        projectile->height_vel *= -projectile->bounciness;
        projectile->entity.world_node.height = get_max_height() - projectile->entity.world_node.size.y / 2;
    }
    // projectile->collider.pos = projectile->entity.world_node.pos;

    // spriteTick(projectile->entity.sprite, delta);

    if (projectile->on_tick != NULL && !node->queued_for_deletion) {
        projectile->on_tick(projectile, delta);
    }

    projectile->life_timer -= delta;
    if (projectile->life_timer <= 0) {
        projectile_destroy(projectile);
        return;
    }
    

}

void projectile_destroy(Projectile *projectile) {

    if (node(projectile)->freed || node(projectile)->queued_for_deletion) return;

    if (projectile->on_destruction != NULL) {
        projectile->on_destruction(projectile);
    }

    Node_queue_deletion(projectile);
}

Projectile Projectile_new(double life_time) {
    Projectile projectile = {0};

    projectile.entity = new(Entity, ENTITY);

    node(&projectile)->on_tick = Projectile_tick;
    

    projectile.life_time = life_time;
    projectile.life_timer = life_time;
    projectile.entity.color = (SDL_Color){255, 255, 255, 255};

    projectile.entity.world_node.size = to_vec(8000);

    projectile.shooter_id = client_self_id;

    return projectile;
}

Projectile *create_bomb_projectile(v2 pos, v2 start_vel) {
    Projectile *projectile = alloc(Projectile, PROJECTILE, 1.6);
    
    CircleCollider *collider = alloc(CircleCollider, CIRCLE_COLLIDER, 5);
    collider->on_collide = Projectile_on_collide;

    Node_add_child(projectile, collider);

    projectile->type = PROJ_BOMB;

    projectile->height_accel = PROJECTILE_GRAVITY;
    projectile->bounciness = 1.0;
    projectile->destroy_on_floor = true;
    projectile->entity.world_node.pos = pos;
    projectile->vel = v2_add(start_vel, v2_mul(player->vel, to_vec(0.5)));
    projectile->height_vel = (is_player_on_floor()? 0 : player->height_vel * 0.5) + player->pitch * -3;
    projectile->entity.world_node.size = to_vec(8000);
    projectile->on_destruction = bomb_on_destroy;
    projectile->on_tick = bomb_on_tick;

    ParticleSpawner *p_spawner = alloc(ParticleSpawner, PARTICLE_SPAWNER, false);

    Sprite *p_sprite1 = alloc(Sprite, SPRITE, true);
    array_append(p_sprite1->animations, create_animation(11, 1, bomb_explosion_particles));
    p_sprite1->autoplay_anim = 0;
    p_sprite1->animations[0].loop = false;
    p_sprite1->animations[0].fps = 7;
    Node_add_child(p_spawner, p_sprite1);

    Sprite *p_sprite2 = alloc(Sprite, SPRITE, true);
    array_append(p_sprite2->animations, create_animation(8, 1, bomb_smoke_particles));
    p_sprite2->animations[0].loop = false;
    p_sprite2->animations[0].fps = 4;
    p_sprite2->autoplay_anim = 0;
    
    Node_add_child(p_spawner, p_sprite2);

    Node_add_child(projectile, p_spawner);

    Sprite *sprite = alloc(Sprite, SPRITE, true);
    array_append(sprite->animations, create_animation(4, 0, bomb_anim));
    sprite->animations[0].loop = true;
    sprite->animations[0].fps = 12;
    spritePlayAnim(sprite, 0);

    Node_add_child(projectile, sprite);
    
    return projectile;
}

void bomb_on_destroy(Projectile *projectile) {

    static const int MAX_DIST = 80;

    ParticleSpawner *p_spawner = get_child_by_type(projectile, PARTICLE_SPAWNER);
    if (p_spawner == NULL) {
        commit_sudoku();
    }
    p_spawner->world_node.pos = V2_ZERO;
    p_spawner->world_node.height = 0;

    shakeCamera(30, 15, true, 10);
    p_spawner->min_size = to_vec(12000);
    p_spawner->max_size = to_vec(12000);
    p_spawner->height_accel = -PARTICLE_GRAVITY * 0.3;
    p_spawner->fade_scale = false;
    p_spawner->affected_by_light = false;
    p_spawner->min_speed = 230;
    p_spawner->max_speed = 250;
    p_spawner->spread = 180;
    p_spawner->height_dir = 0;
    p_spawner->explode_particle_amount = 20;
    p_spawner->particle_lifetime = 3;
    p_spawner->floor_drag = 0.3;
    p_spawner->bounciness = 0.5;
    p_spawner->damp = 1.02;
    p_spawner->explode_particle_amount = 30;
    
    p_spawner->start_color = GPU_MakeColor(255, 230, 120, 255);
    p_spawner->end_color = GPU_MakeColor(255, 230, 120, 255);

    p_spawner->sprite_idx = 0;

    particle_spawner_explode(p_spawner);

    p_spawner->particle_lifetime = 3;
    p_spawner->start_color = GPU_MakeColor(120, 120, 120, 255);
    p_spawner->end_color = GPU_MakeColor(0, 0, 0, 0);
    p_spawner->min_size = to_vec(12000);
    p_spawner->max_size = to_vec(12000);
    p_spawner->min_speed = 240;
    p_spawner->max_speed = 260;
    p_spawner->height_dir = -0.2;

    p_spawner->sprite_idx = 1;

    particle_spawner_explode(p_spawner);

    play_spatial_sound(bomb_explosion, 0.5, player->world_node.pos, projectile->entity.world_node.pos, 500);


    double dist_sqr_to_player = v2_distance_squared(projectile->entity.world_node.pos, player->world_node.pos);

    if (dist_sqr_to_player < MAX_DIST * MAX_DIST) {

        double w = inverse_lerp(MAX_DIST * MAX_DIST, 0, dist_sqr_to_player);

        double dmg = w * 8;
        player_take_dmg(dmg);

        v2 full_kb = v2_mul(v2_dir(projectile->entity.world_node.pos, player->world_node.pos), to_vec(5));
        double height_full_kb = 500;

        v2 kb = v2_lerp(full_kb, V2_ZERO, 1 - w);
        double height_kb = lerp(height_full_kb, 0, 1 - w);

        player->vel = v2_add(player->vel, kb);
        player->height_vel = max(height_kb, player->height_vel + height_kb);
    }

    iter_over_all_nodes(node, {
        if (node->type != PLAYER_ENTITY) continue;

        PlayerEntity *player_entity = node;
        
        double dist_sqr = v2_distance_squared(projectile->entity.world_node.pos, player_entity->entity.world_node.pos);
        
        

        if (dist_sqr_to_player < MAX_DIST * MAX_DIST) {
            double dmg = inverse_lerp(MAX_DIST * MAX_DIST, 0, dist_sqr_to_player) * 8;
            player_entity_take_dmg(player_entity, dmg);
        }
    });
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

    Ability primary_choices[] = {ability_primary_shoot_create()}; //
    Ability secondary_choices[] = {create_switchshot_ability()}; //, ability_secondary_shoot_create(), ability_bomb_create()
    Ability utility_choices[] = {ability_forcefield_create()}; //
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

void Entity_tick(Entity *entity, double delta) {
    // spriteTick(entity->sprite, delta);
}

void bomb_on_tick(Projectile *projectile, double delta) {
    
    
    return;
    
    
    CircleCollider *player_collider = get_child_by_type(player, CIRCLE_COLLIDER);
    CircleCollider *proj_collider = get_child_by_type(projectile, CIRCLE_COLLIDER);

    if (get_circle_collision(player_collider, proj_collider).didCollide) {
        projectile_destroy(projectile);
        return;
    }

    iter_over_all_nodes(node, {
        if (node->type != PLAYER_ENTITY) continue;

        PlayerEntity *player_entity = node;
        CircleCollider *coll = get_child_by_type(player_entity, CIRCLE_COLLIDER);

        if (get_circle_collision(coll, proj_collider).didCollide) {
            projectile_destroy(projectile);
            return;
        }
    });
}

CollisionData get_circle_collision(CircleCollider *col1, CircleCollider *col2) {
    double dist_sqr = v2_distance_squared(col1->world_node.pos, col2->world_node.pos);

    CollisionData data = {0};

    if (dist_sqr < (col1->radius * col1->radius + col2->radius + col2->radius)) {
        data.didCollide = true;
    }


    return data;
}

Ability ability_forcefield_create() {
    Ability ability = {
        .activate = ability_forcefield_activate,
        .before_activate = NULL,
        .can_use = false,
        .cooldown = 10,
        .timer = 0,
        .delay_timer = 0,
        .delay = 0,
        .texture = entityTexture,
        .tick = NULL,
        .type = A_UTILITY
    };

    return ability;
}

void ability_forcefield_activate() {
    printf("Forcefield! \n");

    Projectile *proj = create_forcefield_projectile();

    // ((ParticleSpawner *)proj->extra_data)->target = proj;

    proj->entity.world_node.pos = player->world_node.pos;
    proj->entity.world_node.height = get_player_height();
    // ((ParticleSpawner *)proj->extra_data)->world_node.height = proj->entity.world_node.height;
    proj->vel = playerForward;
    Node_add_child(game_node, proj);
    add_to_sync_queue(proj);


    MPPacket packet = {.type = PACKET_ABILITY_FORCEFIELD, .len = sizeof(struct ability_forcefield_packet), .is_broadcast = true};

    struct ability_forcefield_packet packet_data = {
        .sender_id = client_self_id, 
        .sync_id = -1, 
        .vel = proj->vel, 
        .pos = proj->entity.world_node.pos,
        .height = proj->entity.world_node.height,
        .h_vel = proj->height_vel
    };

    MPClient_send(packet, &packet_data);


}

Projectile *create_forcefield_projectile() {
    Projectile *projectile = alloc(Projectile, PROJECTILE, 10);
    projectile->type = PROJ_FORCEFIELD;
    projectile->on_tick = projectile_forcefield_on_tick;

    Sprite *other_sprite = alloc(Sprite, SPRITE, true);
    Animation anim1 = create_animation(5, 1, forcefield_field);
    anim1.fps = 12;
    anim1.loop = true;
    array_append(other_sprite->animations, anim1);
    spritePlayAnim(other_sprite, 0);
    other_sprite->scale = V2(4, 4);
    Node_add_child(projectile, other_sprite);



    Sprite *sprite = alloc(Sprite, SPRITE, true);
    Animation anim = create_animation(2, 1, forcefield_anim);
    anim.fps = 5;
    anim.loop = true;
    array_append(sprite->animations, anim);
    Node_add_child(projectile, sprite);
    projectile->entity.world_node.size = to_vec(8000);

    ParticleSpawner *particle_spawner = alloc(ParticleSpawner, PARTICLE_SPAWNER, true);
    particle_spawner->spread = 2;
    particle_spawner->min_size = to_vec(2000);
    particle_spawner->max_size = to_vec(5000);
    particle_spawner->height_dir = 1;
    particle_spawner->start_color = Color(200, 255, 255, 255);
    particle_spawner->end_color = Color(255, 255, 255, 255);
    particle_spawner->active = true;
    particle_spawner->height_accel = 0;
    particle_spawner->particle_lifetime = .8;
    particle_spawner->bounciness = 1;
    particle_spawner->dir = (v2){10, 0};
    particle_spawner->affected_by_light = false;
    particle_spawner->min_speed = 2;
    particle_spawner->max_speed = 10;
    particle_spawner->spawn_rate = 32;
    particle_spawner->damp = 1;
    particle_spawner->radial_accel = V2(50, 0);
    particle_spawner->height_radial_accel = 7500 * XY_TO_HEIGHT;
    Node_add_child(projectile, particle_spawner);

    return projectile;
}

void projectile_forcefield_on_tick(Projectile *projectile, double delta) {

    const int REPEL_DIST = 50;

    projectile->vel = v2_lerp(projectile->vel, V2_ZERO, 0.01 * delta * 144);

    iter_over_all_nodes(node, {
        if (node->type != PROJECTILE) continue;

        Projectile *proj = node;
        if (proj == projectile) continue;

        double dist_sqr = v2_distance_squared(projectile->entity.world_node.pos, proj->entity.world_node.pos);
        if (dist_sqr < REPEL_DIST * REPEL_DIST) {
            proj->vel = v2_add(proj->vel, v2_dir(projectile->entity.world_node.pos, proj->entity.world_node.pos));
            proj->height_vel -= proj->height_accel * 1.2 * delta;
        }
    });
}

Node Node_new() {
    Node node = {0};
    node.parent = NULL;
    node.children = array(Node *, 3);

    node.sync_id = -1; // will only be used on things that will get updates (e.g. projectiles)
    node.type = -1;
    node.on_tick = NULL;
    node.on_render = NULL;
    node.on_ready = NULL;
    node.on_delete = NULL;

    return node;
}

void Node_delete(Node *node) {

    if (node == NULL) return;

    if (node->freed) return;

    if (node->on_delete != NULL) node->on_delete(node);
    
    for (int i = 0 ; i < array_length(deletion_queue); i++) {
        if (deletion_queue[i] == node) {
            array_remove(deletion_queue, i);
            break;
        }
    }
    
    while (array_length(node->children) > 0) { // ACCESS OF UNADDRESSABLE MEMORY
        Node *child = node->children[0]; // for debug
        Node_delete(child); // ACCESS OF UNADDRESSABLE MEMORY
    }

    if (node->parent != NULL) {
        Node_remove_child(node->parent, node);
        node->parent = NULL;
    }

    node->freed = true;

    node->on_delete = NULL;
    node->on_ready = NULL;
    node->on_render = NULL;
    node->on_tick = NULL;
    node->type = -1;
    

    array_free(node->children);
    node->children = NULL;
    free(node);
}

void Node_add_child(Node *parent, Node *child) {
    if (parent == NULL || child == NULL || parent->children == NULL) return;

    bool call_ready = child->parent == NULL && !child->_called_ready;

    child->parent = parent;
    array_append(parent->children, child);


    if (call_ready) {
        child->_called_ready = true;
        Node_ready(child);
    }
}


void Node_tick(Node *node, double delta) {

    if (node == NULL) return;

    if (node->on_tick != NULL) {
        node->on_tick(node, delta);
    }

    if (node->children == NULL) {
        printf("Node_tick: Node children are null! \n");
        commit_sudoku();
        return;
    }

    for (int i = 0; i < array_length(node->children); i++) {
        Node_tick(node->children[i], delta);
    }
}


void Node_render(Node *node) {
    if (node == NULL || node->freed || node->queued_for_deletion) return;
    
    
    if (node->on_render != NULL) {
        node->on_render(node);
    }
}

Tilemap Tilemap_new() {
    Tilemap tilemap = {0};
    tilemap.node.type = TILEMAP;
    tilemap.node = new(Node, NODE);
    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            tilemap.ceiling_tilemap[r][c] = -1;
            tilemap.floor_tilemap[r][c] = -1;
            tilemap.level_tilemap[r][c] = -1;
        }
    }

    return tilemap;
}

void Node_remove_child(Node *parent, Node *child) {

    ArrayHeader *header = array_header(parent->children);

    int l = header->length;

    for (int i = 0; i < l; i++) {
        if (parent->children[i] == child) {
            array_remove(parent->children, i);
            child->parent = NULL;
            return;
        }
    }
    // Child doesnt exist
    commit_sudoku();
}

Renderer Renderer_new() {
    Renderer renderer = {0};
    renderer.node.type = RENDERER;
    renderer.node = new(Node, NODE);
    renderer.node.on_render = Renderer_render;

    return renderer;
}

void Renderer_render(Node *node) {
    
    GPU_Clear(screen);
    GPU_Clear(actual_screen);
    GPU_Clear(hud);


    render_floor_and_ceiling();

    RenderObject *render_list = get_render_list();
    
    foreach(RenderObject render_obj, render_list, array_length(render_list), {
        
        if (render_obj.type == WALL_STRIPE) {
            renderWallStripe(render_obj.val);
        } else if (render_obj.type == NODE) {
            if (render_obj.val == renderer) continue;
            Node_render(render_obj.val);
        }
    });

    UI_render(hud, UI_get_root());

    array_free(render_list);

}


LightPoint LightPoint_new() {
    LightPoint l = {0};
    l.node = new(Node, NODE);
    l.node.type = LIGHT_POINT;
    l.color = Color(255, 255, 255, 255);
    l.pos = V2_ZERO;
    l.radius = 10;
    l.strength = 1;

    return l;
}

void _GANA_helper(Node *root, Node ***arr) {

    int i = 0;
    int len = array_length(root->children);

    //printf("Node has %d children \n", len);
    
    array_append((*arr), root);

    for (i = 0; i < len; i++) {
        _GANA_helper(root->children[i], arr);
    }
}

Node **get_all_nodes_array() {
    Node **arr = array(Node *, 200);

    _GANA_helper(root_node, &arr);

    return arr;
}

Entity Entity_new() {
    Entity entity = {0};
    entity.world_node = new(WorldNode, WORLD_NODE);
    entity.color = Color(255, 255, 255, 255);

    node(&entity)->on_tick = Entity_tick;

    return entity;
}

PlayerEntity PlayerEntity_new() {
    PlayerEntity player_entity = {0};
    
    
    player_entity.entity = new(Entity, ENTITY);
    
    node(&player_entity)->on_tick = PlayerEntity_tick;

    return player_entity;
}

int get_node_count() {
    Node **thing = get_all_nodes_array();
    int l = array_length(thing);
    array_free(thing);

    return l;
}

Sprite Sprite_new(bool is_animated) {
    Sprite sprite = {0};
    sprite.autoplay_anim = -1;
    sprite.node = new(Node, NODE);
    if (is_animated) {
        sprite.animations = array(Animation, 2);
    } else {
        sprite.animations = NULL;
    }
    sprite.texture = NULL;
    sprite.isAnimated = is_animated;
    sprite.current_anim_idx = 0;
    sprite.color = Color(255, 255, 255, 255);
    sprite.inherit_color = true;
    sprite.scale = V2_ONE;

    sprite.node.on_delete = Sprite_delete;
    sprite.node.on_tick = Sprite_tick;
    sprite.node.on_render = Sprite_render;
    sprite.node.on_ready = Sprite_ready;

    return sprite;
}

void Sprite_delete(Node *node) {
    Sprite *sprite = node;
    if (sprite->animations != NULL) {
        array_free(sprite->animations);
    }
}

void Sprite_render(Node *node) {

    if (node->parent == NULL) return;

    Sprite *sprite = node;
    SDL_Color color = sprite->color;

    if (sprite->inherit_color) {
        if (node->parent->type == ENTITY) {
            Entity *entity = node->parent;
            color = entity->color;
        }
    }

    GPU_SetRGBA(get_sprite_current_texture(sprite), color.r, color.g, color.b, color.a);

    if (instanceof(node->parent->type, WORLD_NODE)) {
        WorldNode *parent = node->parent;

        bool affected_by_light = false;
        SDL_Color custom_color = Color(255, 255, 255, 255);

        if (instanceof(node->parent->type, ENTITY)) {
            affected_by_light = ((Entity *)parent)->affected_by_light;
            custom_color = ((Entity *)parent)->color;
        }

        

        renderTexture(get_sprite_current_texture((Sprite *)node), parent->pos, v2_mul(parent->size, sprite->scale), parent->height, affected_by_light, custom_color);
    
    } else if (instanceof(node->parent->type, CANVAS_NODE)) {
        CanvasNode *parent = node->parent;

        GPU_Rect rect = {
            parent->pos.x,
            parent->pos.y,
            parent->size.x * sprite->scale.x,
            parent->size.y * sprite->scale.y
        };

        GPU_Image *current_texture = get_sprite_current_texture((Sprite *)node);

        GPU_BlitRect(current_texture, NULL, hud, &rect);
    }

}

void Sprite_tick(Node *node, double delta) { // NO NODE SHENANIGANS WITHOUT EDITING DIRSPRITE

    Sprite *sprite = node;

    if (sprite == NULL) return;
    if (!sprite->isAnimated) return;
    if (sprite->animations == NULL) return;
    if (sprite->current_anim_idx == -1) return;

    animation_tick(&sprite->animations[sprite->current_anim_idx], delta);
}

WorldNode WorldNode_new() {
    WorldNode world_node = {0};
    world_node.node = new(Node, NODE);

    world_node.pos = V2_ZERO;
    world_node.height = 0;
    world_node.size = to_vec(5000);


    return world_node;
}

Particle Particle_new(double life_time) {
    Particle particle = {0};

    particle.effect = new(Effect, EFFECT, life_time);
    particle.start_color = Color(255, 255, 255, 255);
    particle.end_color = Color(255, 255, 255, 255);
    particle.h_accel = -PARTICLE_GRAVITY;
    particle.initial_pos = V2(-10000, -10000);
    particle.damp = 1;

    node(&particle)->on_tick = Particle_tick;

    return particle;
}

Effect Effect_new(double life_time) {
    Effect effect = {0};
    effect.entity = new(Entity, ENTITY);

    node(&effect)->on_tick = Effect_tick;

    effect.life_time = life_time;
    effect.life_timer = life_time;

    return effect;
}

void ability_tick(Ability *ability, double delta) {
    
    if (ability == NULL) return;
    
    if (ability->tick != NULL) {
        ability->tick(ability, delta);
    }
    default_ability_tick(ability, delta);
}

void hand_sprite_tick(Node *node, double delta) {
    CanvasNode *self = node;

    Sprite *sprite = get_child_by_type(node, SPRITE);

    sprite->inherit_color = false;
    
    BakedLightColor bl_color;

    bool first_check = sprite->current_anim_idx == 0;
    bool second_check = sprite->current_anim_idx == 1 && sprite->animations[sprite->current_anim_idx].frame > 1;
    if (first_check || second_check) {
        bl_color = get_light_color_by_pos(player->world_node.pos, 0, 0);
        bl_color.r = 0.3 + bl_color.r * 0.7;
        bl_color.g = 0.3 + bl_color.g * 0.7;
        bl_color.b = 0.3 + bl_color.b * 0.7;
    } else {
        bl_color.r = 2;
        bl_color.g = 1.8;
        bl_color.b = 1.5;
    }

    sprite->color = Color(clamp(bl_color.r * 125, 0, 255), clamp(bl_color.g * 125, 0, 255), clamp(bl_color.b * 125, 0, 255), 255);

    static v2 start_pos = V2(-10000, -10000);
    if (v2_equal(start_pos, V2(-10000, -10000))) {
        start_pos = self->pos;
    }

    self->pos = v2_add(start_pos, player->handOffset);
}

void init_hand_sprite() {

    CanvasNode *canvas_node = alloc(CanvasNode, CANVAS_NODE);

    canvas_node->size = (v2){WINDOW_WIDTH, WINDOW_HEIGHT};

    Sprite *sprite = alloc(Sprite, SPRITE, true);

    Animation shoot_anim = create_animation(6, 1, hand_shoot);
    shoot_anim.loop = false;
    shoot_anim.fps = 12;

    Animation default_anim = create_animation(1, 0, hand_shoot + 5);
    array_append(sprite->animations, default_anim);
    array_append(sprite->animations, shoot_anim);

    spritePlayAnim(sprite, 0);

    canvas_node->node.on_tick = hand_sprite_tick;

    Node_add_child(canvas_node, sprite);

    Node_add_child(root_node, canvas_node);

    hand_node = canvas_node;
}

CanvasNode CanvasNode_new() {
    CanvasNode canvas_node = {0};

    canvas_node.node = new(Node, NODE);

    return canvas_node;
}

void CircleCollider_tick(Node *node, double delta) {
    CircleCollider *collider = node;

    if (node->parent == NULL || !instanceof(node->parent->type, WORLD_NODE)) return;

    collider->world_node.pos = ((WorldNode *)node->parent)->pos;
    collider->world_node.height = ((WorldNode *)node->parent)->height;

    WorldNode *parent = node->parent;

    CollisionData data = getCircleTileMapCollision(collider);

    if (data.didCollide) {
        if (collider->on_collide != NULL) {
            collider->on_collide(collider, data);
        }
        if (collider->custom_on_collide != NULL) {
            collider->custom_on_collide(collider, data);
        }
    }
}

CircleCollider CircleCollider_new(int radius) {
    CircleCollider collider = {.radius = radius};
    collider.world_node = new(WorldNode, WORLD_NODE);
    collider.on_collide = CircleCollider_default_on_collide;
    collider.custom_on_collide = NULL;
    node(&collider)->on_tick = CircleCollider_tick;

    return collider;
}

Node *get_child_by_type(Node *parent, int child_type) {


    for (int i = 0; i < array_length(parent->children); i++) {
        if (parent->children[i]->type == child_type) {
            return parent->children[i];
        }
    }

    return NULL;
}

void Node_queue_deletion(Node *node) {
    if (node->queued_for_deletion) return;
    node->queued_for_deletion = true;
    array_append(deletion_queue, node);
}


void init_health_bar() {
    CanvasNode *canvas_node = alloc(CanvasNode, CANVAS_NODE);

    canvas_node->pos = V2_ZERO;
    canvas_node->size = (v2){400, 150};

    CanvasNode *health_node = alloc(CanvasNode, CANVAS_NODE);
    health_node->pos = V2(60, 50);
    health_node->size = v2_div(canvas_node->size, V2(1.47, 3.5));
    health_node->z_index = -1;
    health_node->node.on_tick = health_node_tick;



    Node_add_child(health_node, alloc(ColorRect, COLOR_RECT, Color(255, 40, 40, 255), ALIGNMENT_LEFT, ALIGNMENT_TOP));

    Node_add_child(canvas_node, health_node);


    Sprite *sprite = alloc(Sprite, SPRITE, false);

    sprite->texture = healthbar_texture;

    Node_add_child(canvas_node, sprite);

    Node_add_child(root_node, canvas_node);
}

ColorRect ColorRect_new(SDL_Color color, int align_x, int align_y) {
    ColorRect color_rect = {0};

    color_rect.node = new(Node, NODE);

    color_rect.color = color;
    color_rect.align_x = align_x;
    color_rect.align_y = align_y;

    color_rect.node.on_render = ColorRect_render;

    return color_rect;
}

void ColorRect_render(Node *node) {
    
    if (node->parent == NULL) return;

    ColorRect *color_rect = node;
    
    if (instanceof(node->parent->type, WORLD_NODE)) {
        WorldNode *wnode = node->parent;
        v2 screen_pos = worldToScreen(wnode->pos, wnode->height, false);
        v2 screen_size = world_to_screen_size(wnode->size, wnode->pos, wnode->height);
        if (color_rect->align_x == ALIGNMENT_CENTER) {
            screen_pos.x -= screen_size.x / 2;
        } else if (color_rect->align_x == ALIGNMENT_RIGHT) {
            screen_pos.x -= screen_size.x;
        }
        if (color_rect->align_y == ALIGNMENT_CENTER) {
            screen_pos.y -= screen_size.y / 2;
        } else if (color_rect->align_y == ALIGNMENT_RIGHT) {
            screen_pos.y -= screen_size.y;
        }

        GPU_Rect rect = {
            screen_pos.x,
            screen_pos.y,
            screen_size.x,
            screen_size.y
        };

        GPU_RectangleFilled2(screen, rect, color_rect->color);
    } else if (instanceof(node->parent->type, CANVAS_NODE)) {
        CanvasNode *cnode = node->parent;

        GPU_Rect rect = {
            cnode->pos.x,
            cnode->pos.y,
            cnode->size.x,
            cnode->size.y
        };

        GPU_RectangleFilled2(hud, rect, color_rect->color);
    }
}

v2 world_to_screen_size(v2 size, v2 pos, double height) {

    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->world_node.pos));

    double dist = v2_distance(player->world_node.pos, pos);

    double dist_from_viewplane = dist * cos_angle_to_forward;

    double fov_factor = tanHalfFOV / tanHalfStartFOV;

    v2 final_size = v2_div(size, to_vec(dist_from_viewplane * fov_factor));

    return final_size;
}

void health_node_tick(Node *node, double delta) {
    CanvasNode *cnode = node;
    CanvasNode *parent = node->parent;

    double normalized_health = player->health / player->maxHealth;
    // div 1.47 3.5 == mul 0.68 0.285

    v2 scaling_factor = V2(0.68, 0.285);

    cnode->size = v2_mul(parent->size, V2(scaling_factor.x * normalized_health, scaling_factor.y));
}

void CircleCollider_default_on_collide(CircleCollider *collider, CollisionData data) {
    if (node(collider)->parent == NULL || !instanceof(node(collider)->parent->type, WORLD_NODE)) return;

    

    WorldNode *parent = node(collider)->parent;

    if (data.didCollide) {
        parent->pos = v2_add(parent->pos, data.offset);
    }
}



void init_crosshair() {
    CanvasNode *cnode = alloc(CanvasNode, CANVAS_NODE);

    double s = 16;
    cnode->size = to_vec(s);
    cnode->pos = V2(WINDOW_WIDTH / 2 - s / 2, WINDOW_HEIGHT / 2 - s / 2);

    Sprite *sprite = alloc(Sprite, SPRITE, false);
    sprite->texture = crosshair;

    Node_add_child(cnode, sprite);

    Node_add_child(root_node, cnode);
}

void _abilities_render(Node *node) {
    render_ability_helper((v2){WINDOW_WIDTH * 1/16      , WINDOW_HEIGHT * 5/6}, player->primary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 80 , WINDOW_HEIGHT * 5/6}, player->secondary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 160, WINDOW_HEIGHT * 5/6}, player->utility);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 240, WINDOW_HEIGHT * 5/6}, player->special);
}

void init_ability_hud() {
    CanvasNode *cnode = alloc(CanvasNode, CANVAS_NODE);
    cnode->node.on_render = _abilities_render;

    Node_add_child(root_node, cnode);
    
}

void Projectile_on_collide(CircleCollider *collider, CollisionData data) {
    Projectile *projectile = node(collider)->parent;

    projectile->entity.world_node.pos = v2_add(projectile->entity.world_node.pos, data.offset);
    projectile->vel = v2_mul(v2_reflect(projectile->vel, v2_normalize(data.offset)), to_vec(projectile->bounciness));
}

void DirSprite_tick(Node *node, double delta) {
    DirSprite *dir_sprite = node;

    for (int i = 0; i < dir_sprite->dirCount; i++) {
        Sprite_tick(&dir_sprite->sprites[i], delta);
    }
}

void DirSprite_render(Node *node) {
    DirSprite *dir_sprite = node;

    if (node->parent == NULL || !instanceof(node->parent->type, WORLD_NODE)) return;
    // no support for canvas node bc how tf would that work

    WorldNode *parent = node->parent;

    GPU_Image *current_texture = get_sprite_current_texture(dir_sprite_current_sprite(dir_sprite, parent->pos));

    bool affected_by_light = true;
    SDL_Color custom_color = Color(255, 255, 255, 255);
    if (instanceof(node->parent->type, ENTITY)) {
        Entity *parent_e = parent;
        affected_by_light = parent_e->affected_by_light;
        custom_color = parent_e->color;
    }

    renderTexture(current_texture, parent->pos, parent->size, parent->height, affected_by_light, custom_color);

}

DirSprite DirSprite_new(int dir_count) {
    DirSprite dir_sprite = {0};
    dir_sprite.dirCount = dir_count;
    dir_sprite.sprites = malloc(sizeof(Sprite) * dir_count);
    for (int i = 0; i < dir_count; i++) {
        dir_sprite.sprites[i] = new(Sprite, SPRITE, true);
    }

    dir_sprite.node = new(Node, NODE);

    node(&dir_sprite)->on_tick = DirSprite_tick;
    node(&dir_sprite)->on_delete = DirSprite_on_delete;
    node(&dir_sprite)->on_render = DirSprite_render;

    return dir_sprite;
}

void DirSprite_add_animation(DirSprite *dir_sprite, int frame_count, GPU_Image **textures) {
    for (int i = 0; i < dir_sprite->dirCount; i++) {
        
        Animation anim = create_animation(frame_count, 1, textures + i * frame_count);
        
        array_append(dir_sprite->sprites[i].animations, anim);
    }
}

void DirSprite_on_delete(Node *node) {
    DirSprite *dir_sprite = node;

    free(dir_sprite->sprites);
    dir_sprite->sprites = NULL;
}

void Node_queue_add_to_game_node(Node *node) {
    array_append(add_queue, node);
}

struct vertex {
    float x, y;
    float s, t;
    float r, g, b, a;
};

// this function is so gever thanks GPT
void render_textured_quad(GPU_Image *texture, v2 top_left, v2 top_right, v2 bot_left, v2 bot_right) {
 

    float vertices[] = {
        // Triangle 1 (Top-left, Top-right, Bottom-right)
        top_left.x, top_left.y,  0.0f, 0.0f,  1, 1, 1, 1,   // Top-left
        top_right.x, top_right.y,  1.0f, 0.0f,  1, 1, 1, 1,   // Top-right
        bot_right.x, bot_right.y,  1.0f, 1.0f,  1, 1, 1, 1,   // Bottom-right
        
        // Triangle 2 (Top-left, Bottom-right, Bottom-left)
        top_left.x, top_left.y,  0.0f, 0.0f,  1, 1, 1, 1,   // Top-left
        bot_right.x, bot_right.y,  1.0f, 1.0f,  1, 1, 1, 1,   // Bottom-right
        bot_left.x, bot_left.y,  0.0f, 1.0f,  1, 1, 1, 1    // Bottom-left
    };

    // Render the two triangles
    GPU_TriangleBatch(texture, screen, 6, vertices, 0, NULL, GPU_BATCH_XY_ST_RGBA);
}

void Line_render(Node *node) {

    Line *line = node;

    if (line->texture == NULL) return;

    double w = line->width;

    v2 top_left = worldToScreen(line->p1, line->h1 - w, true);
    v2 bot_left = worldToScreen(line->p1, line->h1 + w, true);
    v2 top_right = worldToScreen(line->p2, line->h2 - w, true);
    v2 bot_right = worldToScreen(line->p2, line->h2 + w, true);

    render_textured_quad(line->texture, top_left, top_right, bot_left, bot_right);

}

Line Line_new() {
    Line line = {0};
    line.node = new(Node, NODE);
    line.width = 100;

    line.node.on_render = Line_render;
    line.node.on_tick = Line_tick;

    return line;
}



void Line_tick(Node *node, double delta) {
    Line *line = node;
    static double start_width = -1;
    if (start_width == -1) {
        start_width = line->width;
    }

    if (line->fade && node->parent != NULL && instanceof(node->parent->type, EFFECT)) {
        Effect *parent = node->parent;
        line->width = lerp(start_width, 0, inverse_lerp(parent->life_time, 0, parent->life_timer));
    }
}

Node *use_sync_id(int sync_id) {
    if (array_length(sync_id_queue) <= 0) {
        
        printf("Sync id queue is empty!\n");
        
        return NULL;
    }

    if (!is_sync_id_valid(sync_id)) {
        return NULL;
    }

    printf("Popped from sync queue! \n");

    Node *res = sync_id_queue[0];

    array_remove(sync_id_queue, 0);

    res->sync_id = sync_id;

    client_last_seen_sync_id = sync_id;

    return res;
}

void add_to_sync_queue(Node *node) {
    array_append(sync_id_queue, node);
}

Node *find_node_by_sync_id(int sync_id) {
    Node *res = NULL;

    iter_over_all_nodes(node, {
        if (node->sync_id == sync_id) {
            res = node;
            break; // cant return because memory leak !!!
        }
    });

    return res;
}


Ability create_switchshot_ability() {
    Ability a = {
        .activate = switchshot_activate,
        .can_use = false,
        .cooldown = 0.2,
        .delay = 0,
        .delay_timer = 0,
        .texture = switchshot_icon,
        .tick = NULL,
        .timer = 0.2,
        .type = A_SECONDARY,
        .before_activate = NULL
    };

    return a;
}

void switchshot_activate(Ability *ability) {

    double h_dir = 0; // change later

    Projectile *switchshot = create_switchshot_projectile(player->world_node.pos, player->world_node.height, playerForward, h_dir); 

    add_to_sync_queue(switchshot);

    MPPacket packet = {.is_broadcast = true, .len = sizeof(struct ability_switchshot_packet), .type = PACKET_ABILITY_SWITCHSHOT};

    struct ability_switchshot_packet packet_data = {
        .sync_id = -1,
        .sender_id = client_self_id,
        .pos = switchshot->entity.world_node.pos,
        .height = switchshot->entity.world_node.height,
        .vel = switchshot->vel,
        .h_vel = switchshot->height_accel
    };

    MPClient_send(packet, &packet_data);

    printf("Supposed to add switchshot \n");
    Node_add_child(game_node, switchshot);
}

void switchshot_custom_on_collide(CircleCollider *collider, CollisionData colldata) {
    projectile_destroy(node(collider)->parent);
}

Projectile *create_switchshot_projectile(v2 pos, double height, v2 vel, double h_vel) {
    Projectile *proj = alloc(Projectile, PROJECTILE, 8);


    proj->height_accel = 0;
    proj->height_vel = h_vel;
    proj->vel = vel;

    proj->destroy_on_floor = true;
    proj->destroy_on_ceiling = true;
    proj->on_tick = switchshot_projectile_tick;
    proj->on_create = NULL;

    proj->entity.world_node.pos = pos;
    proj->entity.world_node.height = height;


    Sprite *sprite = alloc(Sprite, SPRITE, true);
    array_append(sprite->animations, create_animation(4, 0, switchshot_anim));
    sprite->autoplay_anim = 0;
    sprite->animations[0].fps = 12;
    Node_add_child(proj, sprite);

    CircleCollider *collider = alloc(CircleCollider, CIRCLE_COLLIDER, 5);

    collider->custom_on_collide = switchshot_custom_on_collide;

    Node_add_child(proj, collider);

    return proj;
}

void switchshot_projectile_tick(Projectile *proj, double delta) {

    if (MP_is_server) {
        iter_over_all_nodes(node, {
            if (node->type == PLAYER_ENTITY) {
                PlayerEntity *player_entity = node;

                if (proj->shooter_id == player_entity->id) continue;

                CircleCollider *collider = get_child_by_type(proj, CIRCLE_COLLIDER);
                CircleCollider *player_collider = get_child_by_type(node, CIRCLE_COLLIDER);

                CollisionData coll_data = get_circle_collision(collider, player_collider);

                if (coll_data.didCollide) {
                    switchshot_projectile_switch(proj, player_entity->id);
                    projectile_destroy(proj);
                    return;
                }
            } else if (node->type == PLAYER) {

                if (proj->shooter_id == client_self_id) continue;

                CircleCollider *player_collider = get_child_by_type(player, CIRCLE_COLLIDER);
                CircleCollider *collider = get_child_by_type(proj, CIRCLE_COLLIDER);

                CollisionData coll_data = get_circle_collision(collider, player_collider);

                if (coll_data.didCollide) {
                    switchshot_projectile_switch(proj, client_self_id);
                    projectile_destroy(proj);
                    return;
                }
            }

            
        });
    }

    
}

void switchshot_projectile_switch(Projectile *proj, int player_id) {


    v2 pos1, pos2;
    double h1, h2;

    if (proj->shooter_id == client_self_id) {
        pos1 = player->world_node.pos;
        h1 = player->world_node.height;
    } else {
        PlayerEntity *player1 = find_player_entity_by_id(proj->shooter_id);

        pos1 = player1->entity.world_node.pos;
        h1 = player1->entity.world_node.height;
    }

    if (player_id == client_self_id) {
        pos2 = player->world_node.pos;
        h2 = player->world_node.height;
    } else {
        PlayerEntity *player2 = find_player_entity_by_id(player_id);
        pos2 = player2->entity.world_node.pos;
        h2 = player2->entity.world_node.height;
    }

    MPPacket packet = {.is_broadcast = true, .len = sizeof(struct switch_positions_packet), .type = PACKET_SWITCH_POSITIONS};

    struct switch_positions_packet packet_data = {
        .id1 = proj->shooter_id,
        .pos1 = pos1,
        .h1 = h1,
        .id2 = player_id,
        .pos2 = pos2,
        .h2 = h2
    };

    MPClient_send(packet, &packet_data);
}

Node *copy_node(Node *node, bool copy_tree) {
    Node *copy = malloc(node->size);
    memcpy(copy, node, node->size);

    copy->children = array(Node *, array_length(node->children));
    if (copy_tree) {
        for (int i = 0; i < array_length(node->children); i++) {
            Node_add_child(copy, copy_node(node->children[i], true));
        }
    }

    copy->parent = NULL;

    return copy;
}

void Sprite_ready(Node *node) {
    Sprite *sprite = node;
    if (sprite->autoplay_anim != -1) {
        spritePlayAnim(sprite, sprite->autoplay_anim);
    }
}

Sprite *copy_sprite(Sprite *sprite, bool copy_tree) {
    Sprite *base_copy = copy_node(sprite, copy_tree);
    
    base_copy->animations = array(Animation, array_length(sprite->animations));
    for (int i = 0; i < array_length(sprite->animations); i++) {
        array_append(base_copy->animations, sprite->animations[i]);
    }

    return base_copy;
}

Sprite *ParticleSpawner_get_sprite(ParticleSpawner *spawner) {
    int sprite_count = 0;
    for (int i = 0; i < array_length(node(spawner)->children); i++) {
        if (node(spawner)->children[i]->type == SPRITE) sprite_count++;
    }

    if (spawner->sprite_idx > sprite_count) {
        printf("Tried accessing sprite idx %d when the actual count is %d \n", spawner->sprite_idx, sprite_count);
        return NULL;
    }

    if (sprite_count == 0) {
        return NULL;
    }

    int pick = spawner->sprite_idx == -1 ? randi_range(0, sprite_count - 1) : spawner->sprite_idx;

    int j = 0;
    for (int i = 0; i < array_length(node(spawner)->children); i++) {
        if (j == pick) return node(spawner)->children[i];
        if (node(spawner)->children[i]->type == SPRITE) j++;
    }

    commit_sudoku(); // should never happen

    return NULL;
}

void Node_ready(Node *node) {
    if (node->on_ready != NULL) {
        node->on_ready(node);
    }

    for (int i = 0; i < array_length(node->children); i++) {
        Node_ready(node->children[i]);
    }
}

bool is_sync_id_valid(int sync_id) {
    if (sync_id < 42) return false;
    // if (MP_is_server) {
    //     if (sync_id > server_next_sync_id) return false;
    // } else {
    //     if (sync_id > client_last_seen_sync_id + 100) return false; // 100 so it's only violated by 
    // }
    

    return true;
}


// #END
#pragma GCC diagnostic pop