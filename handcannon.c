
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include "game_utils.c"
#include "globals.h"
#include "ui.c"

// SDL_Renderer *renderer;
// SDL_Window *window;

// #DEFINITIONS

#define TPS 300
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 580
#define RESOLUTION_X 720
#define RESOLUTION_Y 360
#define X_SENSITIVITY .1
#define Y_SENSITIVITY 0.8
#define COLOR_BLACK \
    (SDL_Color) { 0, 0, 0 }
#define TRANSPARENT \
    (SDL_Color) { 0, 0, 0, 0 }
#define RENDER_DISTANCE 350
#define WALL_HEIGHT 30
#define NUM_WALL_THREADS 1
#define NUM_FLOOR_THREADS 2

#define BAKED_LIGHT_RESOLUTION 36
#define BAKED_LIGHT_CALC_RESOLUTION 8

#define PARTICLE_GRAVITY 1

#define OUT_OF_SCREEN_POS \
    (v2) { WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100 }


#define get_window() SDL_GetWindowFromID(actual_screen->context->windowID)

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

typedef struct Rapidfire {
    Ability ability;
    double shot_timer;
    double shot_amount;
    double shots_left;
} Rapidfire;

typedef struct Player {
    v2 pos, vel;
    double speed, angle, torque, collSize;
    double pitch_angle;
    double pitch;
    bool sprinting;
    bool canShoot;
    double shootCooldown;
    double shootChargeTimer;
    int pendingShots;
    int max_pending_shots;
    double ShootTickTimer;
    v2 handOffset;
    double health, maxHealth;
    CircleCollider *collider;

    Ability *primary, *secondary, *utility, *special;
} Player;

typedef struct Entity {
    v2 pos, size;
    Sprite *sprite;
    double height;
    bool affected_by_light;
} Entity;

typedef struct LightPoint {
    v2 pos;
    double strength;
    double radius;
    SDL_Color color;
} LightPoint;

typedef struct Effect {
    Entity entity;
    double lifeTime;
} Effect;

typedef struct Particle {
    Effect effect;
    v2 vel;
    double h_vel;
    v2 accel;
    double h_accel;
    double bounciness;
    double floor_drag;
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

} ParticleSpawner;


typedef struct RenderObject {
    void *val;
    int type;
    double dist_squared;
} RenderObject;

typedef struct Enemy {
    Entity entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
    GPU_Image *hit_texture;
    v2 dir;
    v2 vel;
    double speed, speed_multiplier;
    bool seeingPlayer;

    v2 last_seen_player_pos;
    float track_player_timer;
    
    double maxHealth, health;
    double sound_max_radius;
    v2 move_dir;

    v2 home_pos;
    v2 current_wander_pos;
    double wander_pause_timer;

    EnemyState state;    

    bool noticed_player;
    
    double time_to_pursue;
    double pursue_timer;

    double time_to_forget;
    double forget_timer;

    double pause_timer;

    double max_vision_distance;

    v2 dir_to_player;
    double dist_squared_to_player;

    bool collided_last_frame;

    void (*tick)(struct Enemy *, double);

    void (*on_take_dmg)(struct Enemy *, double);

} Enemy;

typedef struct Bullet {
    Entity entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
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



// ENEMY IDEAS:
// Enemy which reflects shots
// Big enemy that splits into multiple enemies

// #ENEMIES
typedef struct ShooterEnemy {
    Enemy enemy;
    double shootCooldown;
    double shootCooldownTimer;

    int mode;
    double mode_timer;
    v2 resting_move_dir;
    
    v2 attacking_move_dir;
    double attacking_shoot_timer;


} ShooterEnemy;

typedef struct ExploderEnemy {
    Enemy enemy;

    double explosion_kb;
    double explosion_radius;
} ExploderEnemy;

// #ENEMIES END

typedef struct CollisionData {
    v2 offset;  // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;

// #FUNC

void _player_die();

void ability_dash_before_activate(Ability *ability);

void activate_ability(Ability *ability);

void ability_dash_activate(Ability *ability);

void ability_dash_tick(Ability *ability, double delta);

void enemy_default_forget_behaviour(Enemy *enemy, double delta);

void shooter_bullet_effect(Bullet *bullet);

void place_entity(v2 pos, int type);

void exploder_on_take_dmg(Enemy *enemy, double dmg);

void particle_tick(Particle *particle, double delta);

void particle_spawner_spawn(ParticleSpawner *spawner);

void particle_spawner_tick(ParticleSpawner *spawner, double delta);

ParticleSpawner create_particle_spawner(v2 pos, double height);

void draw_3d_line(v2 pos1, double h1, v2 pos2, double h2);

void _shoot(double spread);

void ability_secondary_shoot_activate(Ability *ability);

Ability ability_dash_create();

Rapidfire ability_secondary_shoot_create();

Ability ability_primary_shoot_create();

void default_ability_tick(Ability *ability, double delta);

void exploder_explode(ExploderEnemy *exploder);

void dir_sprite_play_anim(DirectionalSprite *dir_sprite, int anim);

void exploder_tick(Enemy *enemy, double delta);

bool is_entity_type(int type);

ExploderEnemy *enemy_exploder_create(v2 pos);

void enemy_pause(Enemy *enemy, double sec);

void enemy_default_handle_state(Enemy *enemy, double delta);

void enemy_notice_player_effect(Enemy *enemy, double delta);

void enemy_idle_movement(Enemy *enemy, double delta);

void enemy_handle_collisions(Enemy *enemy);

void enemy_move(Enemy *enemy, double delta);

int charge_time_to_shots(double charge_time);

void enemy_bullet_destroy(Bullet *bullet);

SDL_Color lerp_color(SDL_Color col1, SDL_Color col2, double w);

void remove_loading_screen();

void update_loading_progress(double progress);

void init_loading_screen();

void player_die();

void player_take_dmg(double dmg);

void reset_level();

bool is_enemy_type(int type);

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

void init_player();

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

void enemyTick(Enemy *enemy, double delta);

void animationTick(Animation *anim, double delta);

void effectTick(Effect *effect, double delta);

void bulletTick(Bullet *bullet, double delta);

void shooterTick(Enemy *enemy, double delta);

Sprite *createSprite(bool isAnimated, int animCount);

DirectionalSprite *createDirSprite(int dirCount);

void spritePlayAnim(Sprite *sprite, int idx);

Sprite *getRandomWallSprite();

void ability_shoot_activate(Ability *ability);

void enemyTakeDmg(Enemy *enemy, int dmg);

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light);

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height);

double angleDist(double a1, double a2);

void shakeCamera(double strength, int ticks, bool fade, int priority);

ShooterEnemy *enemy_shooter_create(v2 pos);



bool isValidLevel(char *file);

void getTextureFiles(char *fileName, int fileCount, GPU_Image ***textures);

// #FUNC END

GPU_Target *screen;
GPU_Image *screen_image;
GPU_Target *actual_screen;

// #TEXTURES
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
GPU_Image *defualt_particle_texture;
GPU_Image *mimran_jumpscare;
GPU_Image *shooter_hit_texture;
GPU_Image *healthbar_texture;
GPU_Image *vignette_texture;
GPU_Image *enemy_bullet_texture;
GPU_Image *floorAndCeiling;
GPU_Image *floor_and_ceiling_target_image;
GPU_Image *wallTexture;
GPU_Image *entityTexture;
GPU_Image *crosshair;
GPU_Image *fenceTexture;
GPU_Image *skybox_texture;
GPU_Image **wallFrames;
GPU_Image **shootHitEffectFrames;
GPU_Image **enemy_bullet_destroy_anim;
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
Sprite *animatedWallSprite;
Sprite *leftHandSprite;

// #SOUNDS
Sound *rapidfire_sound;
Sound *exploder_explosion;
Sound *enemy_default_hit;
Sound *enemy_default_kill;
Sound *player_default_shoot;
Sound *player_default_hurt;

// #SHADERS
int floor_shader;
GPU_ShaderBlock floor_shader_block;

// #VAR

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
arraylist *gameobjects;
Player *player = NULL;
bool keyPressArr[26];
bool render_debug = false;
bool lockMouse = true;
bool isLMouseDown = false;
bool isRMouseDown = false;
double startFov = 90;
double fov = 90;
const char *font = "font.ttf";
const SDL_Color fogColor = {0, 0, 0, 255};
double tanHalfFOV;
double tanHalfStartFOV;
double ambient_light = 0.5;
int floorRenderStart;
int **levelTileMap;
int **floorTileMap;
int **ceilingTileMap;
const int tileSize = WINDOW_WIDTH / 30;
double realFps;
bool isCameraShaking = false;
int camerashake_current_priority = 0;
int cameraShakeTicks;
int cameraShakeTicksLeft = 0;
double cameraShakeTimeToNextTick = 0.02;
double cameraShakeCurrentStrength = 0;
bool cameraShakeFadeActive = false;
RenderObject *wallStripesToRender[RESOLUTION_X];
v2 cameraOffset = {0, 0};
v2 playerForward;
char *levelToLoad = NULL;
const double PLAYER_SHOOT_COOLDOWN = 0.5;
// #VAR END

// #DEBUG VAR


// #MAIN
int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) printf("Shit. \n");

    // window = SDL_CreateWindow("Doom style 3D!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);


    actual_screen = GPU_Init(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_INIT_DISABLE_VSYNC);
    SDL_SetWindowTitle(SDL_GetWindowFromID(actual_screen->context->windowID), "Goofy");

    screen_image = GPU_CreateImage(WINDOW_WIDTH, WINDOW_HEIGHT, GPU_FORMAT_RGBA);
    GPU_SetImageFilter(screen_image, GPU_FILTER_NEAREST);
    screen = GPU_LoadTarget(screen_image);


    // renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);

    if (argc >= 2) {
        levelToLoad = argv[1];
    }

    init_ui();

    init();

    u64 tick_timer = 0, render_timer = 0;
    u64 last_time = SDL_GetTicks64();
    while (running) {  // #GAME LOOP
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_input(event);
        }

        u64 now = SDL_GetTicks64();
        u64 delta = now - last_time;
        last_time = now;
        tick_timer += delta;
        // render_timer += delta;
        if (tick_timer >= 1000 / TPS) {
            realFps = 1000.0 / tick_timer;
            tick(mili_to_sec(tick_timer));
            render(mili_to_sec(tick_timer));
            tick_timer = 0;
        }
    }

    // SDL_DestroyRenderer(renderer);

    // SDL_DestroyWindow(window);

    GPU_FreeImage(screen_image);

    GPU_Quit();

    SDL_Quit();
}

Enemy createEnemy(v2 pos, DirectionalSprite *dir_sprite) {
    Enemy enemy;
    enemy.maxHealth = 5;
    enemy.health = enemy.maxHealth;
    enemy.dir = (v2){1, 0};
    enemy.vel = (v2){0, 0};
    enemy.move_dir = (v2){0, 0};
    enemy.speed = 50;
    enemy.speed_multiplier = 1;
    enemy.state = STATE_IDLE;
    
    enemy.tick = NULL;
    enemy.on_take_dmg = NULL;

    // idle movement
    enemy.home_pos = pos;
    enemy.current_wander_pos = pos;
    enemy.wander_pause_timer = 10;

    enemy.time_to_forget = 6;
    enemy.forget_timer = enemy.time_to_forget;

    enemy.noticed_player = false;
    enemy.time_to_pursue = 1;
    enemy.pursue_timer = enemy.time_to_pursue;


    enemy.pause_timer = 0;

    enemy.max_vision_distance = 300;

    enemy.dir_to_player = V2_ZERO;
    enemy.dist_squared_to_player = 0;

    enemy.collided_last_frame = false;

    enemy.entity.pos = pos;
    enemy.entity.size = to_vec(9500);
    enemy.entity.sprite = NULL;
    enemy.entity.affected_by_light = true;
    enemy.hit_texture = NULL;
    enemy.sound_max_radius = 400;

    if (dir_sprite == NULL) {
        enemy.dirSprite = createDirSprite(16);
        for (int i = 0; i < 16; i++) {

            enemy.dirSprite->sprites[i] = createSprite(false, 0);
            enemy.dirSprite->sprites[i]->texture = shooter_dirs_textures[i];
        }
    } else {
        enemy.dirSprite = dir_sprite;
    }

    enemy.dirSprite->dir = (v2){1, 0};

    enemy.entity.height = get_max_height() * 0.1;

    enemy.collider = malloc(sizeof(CircleCollider));
    enemy.collider->radius = 10;
    enemy.collider->pos = enemy.entity.pos;

    enemy.seeingPlayer = false;
    enemy.last_seen_player_pos = (v2){0, 0};
    enemy.track_player_timer = 0.25;

    return enemy;
}

void init() {  // #INIT

    tilemap_image = GPU_CreateImage(TILEMAP_WIDTH, TILEMAP_HEIGHT, GPU_FORMAT_RGBA);

    GPU_Target *image_target = GPU_LoadTarget(tilemap_image);

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            SDL_Color color = (SDL_Color){255, 255, 255, 255};
            GPU_RectangleFilled2(image_target, (GPU_Rect){c, r, 1000, 1000}, color);
        }
    }

    GPU_FreeTarget(image_target);

    GPU_SaveImage(tilemap_image, "test.png", GPU_FILE_PNG);

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

    rapidfire_sound = create_sound("Sounds/shotgun_ability.wav");

    exploder_explosion = create_sound("Sounds/exploder_explosion.wav");

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

    defualt_particle_texture = load_texture("Textures/base_particle.png");

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
    enemy_default_hit = create_sound("Sounds/enemy_default_hit.wav");
    enemy_default_kill = create_sound("Sounds/enemy_default_kill.wav");

    init_cd_print();

    mimran_jumpscare = load_texture("Textures/scary_monster2.png");

    shooter_hit_texture = load_texture("Textures/ShooterEnemy/hit_frame1.png");

    healthbar_texture = load_texture("Textures/health_bar.png");

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

    init_tilemap(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    init_tilemap(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    init_tilemap(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    tanHalfFOV = tan(deg_to_rad(fov / 2));
    tanHalfStartFOV = tan(deg_to_rad(startFov / 2));

    gameobjects = create_arraylist(10);

    wallTexture = load_texture("Textures/wall.png");
     GPU_SetImageFilter(wallTexture, GPU_FILTER_NEAREST);


    wallFrames = malloc(sizeof(GPU_Image *) * 17);
    getTextureFiles("Textures/WallAnim/wallAnim", 17, &wallFrames);

    crosshair = load_texture("Textures/crosshair.png");

    animatedWallSprite = createSprite(true, 1);
    animatedWallSprite->animations[0] = create_animation(17, 0, wallFrames);
    animatedWallSprite->animations[0].fps = 10;
    spritePlayAnim(animatedWallSprite, 0);

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

    for (int i = 0; i < 26; i++) keyPressArr[i] = false;

    entityTexture = load_texture("Textures/scary_monster.png");

    if (isValidLevel(levelToLoad)) {
        load_level(levelToLoad);

    } else {
        load_level("Levels/default_level.hclevel");
    }

    skybox_texture = load_texture("Textures/skybox.png");

    //SDL_RenderSetLogicalSize(SDL_GetRenderer(get_window()), WINDOW_WIDTH, WINDOW_HEIGHT);


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
            player->angle += event.motion.xrel * X_SENSITIVITY;
            player->handOffset.x = lerp(player->handOffset.x, -event.motion.xrel * 2, 0.06);
            player->pitch += event.motion.yrel * Y_SENSITIVITY;
            player->handOffset.y = lerp(player->handOffset.y, -event.motion.yrel * 2, 0.06);
            player->pitch = clamp(player->pitch, -280, 280);
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
    if (key == SDLK_p) {
        lockMouse = !lockMouse;
        return;
    }
    if (key == SDLK_o) {
        render_debug = !render_debug;
        return;
    }

    if (key == SDLK_r) {

        reset_level();
    }

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

    double speed_multiplier = 1;

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

    player->vel = v2_lerp(player->vel, v2_add(v2_mul(move_dir, to_vec(keyVec.x)), v2_mul(move_dir_rotated, to_vec(keyVec.y))), delta * 10);

    CollisionData player_coldata = getCircleTileMapCollision(*player->collider);
    if (player_coldata.didCollide) {
        player->pos = v2_add(player->pos, player_coldata.offset);
    }
    if (player->sprinting) {
        speed_multiplier *= 1.25;
        fov = lerp(fov, startFov * 1.2, delta * 7);
    } else {
        fov = lerp(fov, startFov, delta * 7);
    }

    v2 finalVel = v2_mul(player->vel, to_vec(player->speed * speed_multiplier));

    bool move_without_ray = true;

    double movement = v2_length(finalVel) * delta;

    if (movement > 2) {
        RayCollisionData ray = castRay(player->pos, playerForward);
        if (ray.hit) {
            double dist = v2_distance(ray.startpos, ray.collpos);
            if (dist < movement) {
                move_without_ray = false;
                player->pos = v2_add(player->pos, v2_mul(playerForward, to_vec(dist - 10)));
                cd_print(true, "Hello world \n");
            }
        }
    }
    
    if (move_without_ray) 
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(delta)));
    
    
    
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

    if (is_enemy_type(type)) {
        Enemy *enemy = obj;
        if (enemy->tick != NULL) {
            enemy->tick(enemy, delta);
            return;
        }
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
        
    }
}

// #TICK
void tick(double delta) {

    vignette_color = lerp_color(vignette_color, (SDL_Color){0, 0, 0}, delta);

    playerForward = get_player_forward();

    

    tanHalfFOV = tan(deg_to_rad(fov / 2));

    if (lockMouse) {
        SDL_WarpMouseInWindow(get_window(), WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
        
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

    SDL_ShowCursor(!lockMouse);

    animationTick(&animatedWallSprite->animations[animatedWallSprite->currentAnimationIdx], delta);
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

    GPU_BlitRect(tilemap_image, NULL, screen, NULL);
    
}

v2 getRayDirByIdx(int i) {
    double x = tanHalfFOV;
    double idx = lerp(-x, x, ((double)(i + 1)) / RESOLUTION_X);

    v2 temp = (v2){1, idx};

    temp = v2_normalize(temp);
    v2 rayDir = v2_dir((v2){0, 0}, v2_rotate(temp, deg_to_rad(player->angle)));
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
    double wallSize = WALL_HEIGHT * WINDOW_HEIGHT / dist_to_viewplane * fov_factor;
    double y_pos = WINDOW_HEIGHT / 2 + wallSize / 2 - height / dist_to_viewplane;

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
    double dist = (WALL_HEIGHT * WINDOW_HEIGHT) / (wallSize)*fovFactor;

    double cosAngleToForward = v2_cos_angle_between(rayDir, playerForward);

    dist /= cosAngleToForward;

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

void renderTexture(GPU_Image *texture, v2 pos, v2 size, double height, bool affected_by_light) {
    

    v2 screen_pos = worldToScreen(pos, height, false);

    if (v2_equal(screen_pos, OUT_OF_SCREEN_POS)) {
        return;
    }
    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    double dist_to_player = v2_distance(pos, player->pos);

    double dist_to_viewplane = dist_to_player * cos_angle_to_forward;

    double fov_factor = tanHalfFOV / tanHalfStartFOV;

    v2 final_size = v2_div(size, to_vec(dist_to_player * fov_factor));

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
    

    GPU_SetRGB(texture, rgb[0], rgb[1], rgb[2]);
    GPU_BlitRect(texture, NULL, screen, &dstRect);
    GPU_SetRGB(texture, 255, 255, 255);
}

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height) {
    GPU_Image *texture = getSpriteCurrentTexture(dir_sprite_current_sprite(dSprite, pos));

    renderTexture(texture, pos, size, height, true);
}

void renderEntity(Entity entity) {  // RENDER ENTITY
    GPU_Image *texture = getSpriteCurrentTexture(entity.sprite);

    renderTexture(texture, entity.pos, entity.size, entity.height, entity.affected_by_light);
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

RenderObject *getWallStripe(int i) {
    v2 ray_dir = getRayDirByIdx(i);

    RayCollisionData data = castRay(player->pos, ray_dir);

    if (!data.hit) {
        return NULL;
    }

    WallStripe *stripe = malloc(sizeof(WallStripe));
    stripe->i = i;
    stripe->texture = data.colliderTexture;

    double cos_angle_to_forward = v2_cos_angle_between(ray_dir, playerForward);
    double dist = v2_distance(data.startpos, data.collpos) * cos_angle_to_forward;
    double fov_factor = tanHalfStartFOV / tanHalfFOV;
    double final_size = WALL_HEIGHT * WINDOW_HEIGHT / dist * fov_factor;

    stripe->size = final_size;
    
    stripe->brightness = dist > 150? distance_to_color(dist - 150, 0.01) : 1;
    stripe->normal = data.normal;
    stripe->collIdx = data.collIdx;
    stripe->wallWidth = data.wallWidth;
    stripe->pos = data.collpos;

    RenderObject *currentRenderObj = malloc(sizeof(RenderObject));
    if (currentRenderObj == NULL) {
        printf("tf is this trash lang \n");
    }

    double real_dist = dist / cos_angle_to_forward;

    currentRenderObj->dist_squared = real_dist * real_dist;
    currentRenderObj->val = stripe;
    currentRenderObj->type = WALL_STRIPE;

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

arraylist *getRenderList() {
    arraylist *renderList = create_arraylist(RESOLUTION_X + 5);

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

    

    // convert everything to sort objects, sort, get back our sorted goods, put
    // into main list
    SortObject sortObjects[RESOLUTION_X];
    for (int i = 0; i < RESOLUTION_X; i++) {
        SortObject sObj = {wallStripesToRender[i], wallStripesToRender[i] == NULL ? 9999999999 : wallStripesToRender[i]->dist_squared};
        sortObjects[i] = sObj;
    }

    SortObject *sorted = merge_sort(sortObjects, RESOLUTION_X);
    for (int i = RESOLUTION_X - 1; i >= 0; i--) {
        arraylist_add(renderList, sorted[i].val, RENDER_OBJECT);
    }
    free(sorted);

    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);

        RenderObject *currentRObj = malloc(sizeof(RenderObject));
        v2 pos;

        if (is_enemy_type(object->type)) {

            currentRObj->val = object->val;
            currentRObj->type = ENEMY;
            pos = ((Enemy *)object->val)->entity.pos;

        } else if (is_entity_type(object->type)) {

            currentRObj->val = object->val;
            currentRObj->type = ENTITY;
            pos = ((Entity *)object->val)->pos;

        }

        currentRObj->dist_squared = v2_distance_squared(pos, player->pos);

        bool added = false;

        for (int i = 0; i < renderList->length; i++) {
            RenderObject *rObj = arraylist_get(renderList, i)->val;
            if (rObj == NULL || currentRObj->dist_squared <= rObj->dist_squared) {
                continue;
            }
            arraylist_insert(renderList, currentRObj, RENDER_OBJECT, i);
            added = true;
            break;
        }
        if (!added) {
            arraylist_add(renderList, currentRObj, RENDER_OBJECT);
        }
    }

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

    GPU_Rect dstRect = {
        stripe->i * WINDOW_WIDTH / RESOLUTION_X + cameraOffset.x, 
        WINDOW_HEIGHT / 2 - stripe->size / 2 - player->pitch + cameraOffset.y,
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
        GPU_BlitRect(texture, NULL, screen, &leftHandRect);
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

    // SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    GPU_RectangleFilled2(screen, health_rect, (SDL_Color){200, 0, 0, 255});

    GPU_BlitRect(healthbar_texture, NULL, screen, &outline_rect);
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
        GPU_BlitRect(ability->texture, NULL, screen, &rect);
    }
    double primary_progress = ability->timer == ability->cooldown? 0 : ability->timer / ability->cooldown;

    GPU_Rect primary_progress_rect = {
        pos.x, 
        pos.y,
        size.x,
        size.y * primary_progress
    };

    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 170);
    
    GPU_RectangleFilled2(screen, primary_progress_rect, GPU_MakeColor(0, 0, 0, 170));


    // SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    GPU_BlitRect(ability_icon_frame, NULL, screen, &frame_rect);
}

void render_ability_hud() {
    render_ability_helper((v2){WINDOW_WIDTH * 1/16      , WINDOW_HEIGHT * 5/6}, player->primary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 80 , WINDOW_HEIGHT * 5/6}, player->secondary);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 160, WINDOW_HEIGHT * 5/6}, player->utility);
    render_ability_helper((v2){WINDOW_WIDTH * 1/16 + 240, WINDOW_HEIGHT * 5/6}, player->special);
}


void renderHUD() {

    GPU_BlitRect(getSpriteCurrentTexture(dash_anim_sprite), NULL, screen, NULL);
    spriteTick(dash_anim_sprite, 0.016);

    GPU_SetRGB(vignette_texture, vignette_color.r, vignette_color.g, vignette_color.b);
    GPU_BlitRect(vignette_texture, NULL, screen, NULL);

    render_hand();

    render_health_bar();

    render_ability_hud();

    GPU_Rect crosshairRect = {WINDOW_WIDTH / 2 - 8, WINDOW_HEIGHT / 2 - 8, 16, 16};

    GPU_BlitRect(crosshair, NULL, screen, &crosshairRect);

    int shots = max(player->pendingShots, (int)(player->shootChargeTimer * 3));

    GPU_Rect playerPendingShotsRect = {WINDOW_WIDTH / 2 + -10 * shots, WINDOW_HEIGHT * 0.8, 20 * shots, WINDOW_HEIGHT * 0.05};

}

void drawSkybox() {
    GPU_Image *tex = skybox_texture;

    double x = loop_clamp(player->angle / startFov * WINDOW_WIDTH, 0, WINDOW_WIDTH);

    double yOffsets = -player->pitch;

    GPU_Rect skybox_rect = {-x, yOffsets, WINDOW_WIDTH * 2, WINDOW_HEIGHT / 2};

    GPU_BlitRect(tex, NULL, screen, &skybox_rect);
}

void render(double delta) {  // #RENDER

    GPU_Clear(screen);

    char *newTitle = "FPS: ";
    char *fps = malloc(4);
    decimal_to_text(realFps, fps);

    char *final = concat(newTitle, fps);

    SDL_SetWindowTitle(get_window(), final);

    free(final);

    // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    arraylist *renderList = getRenderList();

    render_floor_and_ceiling();

    for (int i = 0; i < renderList->length; i++) {
        RenderObject *rObj = arraylist_get(renderList, i)->val;
        if (rObj == NULL) {
            continue;
        }

        switch (rObj->type) {
            case (int)ENTITY:
                renderEntity(*(Entity *)rObj->val);
                break;
            case (int)WALL_STRIPE:
                renderWallStripe((WallStripe *)rObj->val);
                break;
            case (int)ENEMY:;
                Enemy *enemy = rObj->val;
                if (enemy->dirSprite != NULL) {
                    renderDirSprite(enemy->dirSprite, enemy->entity.pos, enemy->entity.size, enemy->entity.height);
                } else {
                    renderEntity(enemy->entity);
                }
                break;

            case (int)BULLET:;
                Bullet *bullet = rObj->val;
                if (bullet->dirSprite != NULL) {
                    renderDirSprite(bullet->dirSprite, bullet->entity.pos, bullet->entity.size, bullet->entity.height);
                } else {
                    renderEntity(bullet->entity);
                }
                break;
        }
    }

    arraylist_free(renderList);

    if (render_debug) renderDebug();

    renderHUD();

    screen_modulate_r = lerp(screen_modulate_r, 1, delta / 2);
    screen_modulate_g = lerp(screen_modulate_g, 1, delta / 2);
    screen_modulate_b = lerp(screen_modulate_b, 1, delta / 2);

    GPU_Blit(screen_image, NULL, actual_screen, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

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
    player->collider->radius = 5;
    player->collider->pos = player->pos;
    player->maxHealth = 10;
    player->health = player->maxHealth;

    Ability *default_primary = malloc(sizeof(Ability));
    Rapidfire *default_secondary = malloc(sizeof(Rapidfire));
    Ability *default_utility = malloc(sizeof(Ability));
    *default_primary = ability_primary_shoot_create();
    *default_secondary = ability_secondary_shoot_create();
    *default_utility = ability_dash_create();

    player->primary = default_primary;
    player->secondary = default_secondary;
    player->utility = default_utility;
    player->special = NULL;
}

// CLEAR
RayCollisionData ray_object(Raycast ray, obj *object) {

    if (is_enemy_type(object->type)) {
        Enemy *enemy = object->val;
        RayCollisionData enemy_ray_data = ray_circle(ray, *enemy->collider);
        if (enemy_ray_data.hit) {
            enemy_ray_data.collider = (ShooterEnemy *)object->val;
            enemy_ray_data.colliderType = ENEMY_SHOOTER;
        }

        return enemy_ray_data;
    }

    switch (object->type) {
        case (int)PLAYER:;
            RayCollisionData playerRayData = ray_circle(ray, *player->collider);
            if (playerRayData.hit) {
                playerRayData.collider = player;
            }
            return playerRayData;
            break;
    }

    RayCollisionData data;
    data.hit = false;
    return data;
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
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            (*gridPtr)[r][c] = -1;
        }
    }
}

void clearLevel() {

    for (int i = 0; i < gameobjects->length; i++) {
        obj *object = arraylist_get(gameobjects, i);
        freeObject(object->val, object->type);
    }

    arraylist_clear(gameobjects);

    player = NULL;

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

    int fileSize = TILEMAP_HEIGHT * TILEMAP_WIDTH * 4;

    char *data = malloc(sizeof(char) * fileSize);  // sizeof char is 1 but i do it for clarity

    fgets(data, fileSize, fh);

    int idx = 0;

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            floorTileMap[r][c] = data[idx++];
            if (floorTileMap[r][c] == (int)P_FLOOR_LIGHT) {
                LightPoint *test_point = malloc(sizeof(LightPoint));
                test_point->color = (SDL_Color){255, 100, 10};//{255, 200, 100};
                test_point->strength = 4;
                test_point->radius = 140;
                test_point->pos = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                add_game_object(test_point, LIGHT_POINT);
            }
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            levelTileMap[r][c] = data[idx++];
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            ceilingTileMap[r][c] = data[idx++];
            if (ceilingTileMap[r][c] == (int)P_CEILING_LIGHT) {
                LightPoint *test_point = malloc(sizeof(LightPoint));
                test_point->color = (SDL_Color){255, 170, 70};//{255, 200, 100};
                test_point->strength = 5;
                test_point->radius = 400;
                test_point->pos = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                add_game_object(test_point, LIGHT_POINT);
            }
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            int type = data[idx++];
            v2 tile_middle = v2_add(v2_mul((v2){c, r}, to_vec(tileSize)), to_vec(tileSize / 2));

            place_entity(tile_middle, type);
        }
    }

    free(data);

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
    effect->entity.height = WINDOW_HEIGHT / 6;  // idk it works
    effect->lifeTime = lifeTime;

    return effect;
}

// Todo: add shoot cooldown for base shooting
void ability_shoot_activate(Ability *ability) {
    _shoot(0.02);
}

void effectTick(Effect *effect, double delta) {

    effect->lifeTime -= delta;
    if (effect->lifeTime <= 0) {
        remove_game_object(effect, EFFECT);
        return;
    }
    spriteTick(effect->entity.sprite, delta);
}

void enemyTick(Enemy *enemy, double delta) {
    enemy->collider->pos = enemy->entity.pos;

    bool paused = enemy->pause_timer > 0;

    if (paused) {
        enemy->pause_timer -= delta;
    } else {
        enemy_move(enemy, delta);
        enemy->collided_last_frame = false;
        enemy_handle_collisions(enemy);
    }


    // add player vision

    enemy->dir_to_player = v2_dir(enemy->entity.pos, player->pos);
    enemy->dist_squared_to_player = v2_distance_squared(enemy->entity.pos, player->pos);

    v2 dir = enemy->dir_to_player;
    double dist_sqr = enemy->dist_squared_to_player;

    if (dist_sqr > enemy->max_vision_distance * enemy->max_vision_distance) {
        enemy->seeingPlayer = false;
    } else {
        RayCollisionData rayData = castRayForAll(v2_add(enemy->entity.pos, v2_mul(dir, to_vec(enemy->collider->radius + 0.01))), dir);
        enemy->seeingPlayer = rayData.hit && (Player *)rayData.collider == player;

        if (enemy->seeingPlayer) {
            enemy->track_player_timer -= delta;
            if (enemy->track_player_timer <= 0) {
                enemy->track_player_timer = 0.25;
                enemy->last_seen_player_pos = player->pos;
            }
        }
    }

    

    if (enemy->dirSprite != NULL) enemy->dirSprite->dir = enemy->dir;

    dSpriteTick(enemy->dirSprite, enemy->entity.pos, delta);
}

void enemyTakeDmg(Enemy *enemy, int dmg) {
    enemy->health -= dmg;
    shakeCamera(15, 4, true, 0);
    Sprite *sprite = createSprite(false, 0);
    sprite->texture = enemy->hit_texture;
    Effect *hit_effect = createEffect(v2_add(enemy->entity.pos, v2_dir(enemy->entity.pos, player->pos)), enemy->entity.size, sprite, 0.1);
    hit_effect->entity.height = enemy->entity.height;
    hit_effect->entity.affected_by_light = false;
    add_game_object(hit_effect, EFFECT);
    
    play_spatial_sound(enemy_default_hit, 1, player->pos, enemy->entity.pos, enemy->sound_max_radius);

    if (enemy->health <= 0) {
        // play death anim
        play_spatial_sound(enemy_default_kill, 1, player->pos, enemy->entity.pos, enemy->sound_max_radius);
        remove_game_object(enemy, ENEMY);
        return;
    }


    if (enemy->on_take_dmg != NULL){
        enemy->on_take_dmg(enemy, (double)dmg);
    } // its at the end bc it could free
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
    bullet->entity.height = WINDOW_HEIGHT / 6;
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

    if (bullet->hit_enemies) {
        for (int i = 0; i < gameobjects->length; i++) {
            
            obj *game_object = arraylist_get(gameobjects, i);

            if (!is_enemy_type(game_object->type)) continue;

            Enemy *enemy = game_object->val;

            if (intersectCircles(*bullet->collider, *enemy->collider)) {
                enemyTakeDmg(enemy, bullet->dmg);
            }
        }
    }

    if (bullet->dirSprite != NULL) {
        dSpriteTick(bullet->dirSprite, bullet->entity.pos, delta);
    } else {
        spriteTick(bullet->entity.sprite, delta);
    }
}

void enemy_shooter_shoot(ShooterEnemy *shooter) {
    Bullet *bullet = createDefaultBullet(shooter->enemy.entity.pos, v2_rotate(shooter->enemy.dir, randf_range(-0.05, 0.05)));
    if (bullet == NULL) {
        printf("Bullet is null \n");
        return;
    }
    add_game_object(bullet, BULLET);
}

bool pos_in_tile(v2 pos) {
    int row = pos.y / tileSize;
    int col = pos.x / tileSize;

    if (!in_range(row, 0, TILEMAP_HEIGHT - 1) || !in_range(col, 0, TILEMAP_WIDTH - 1)) return false;

    return levelTileMap[row][col] == P_WALL;
}



void shooterTick(Enemy *enemy, double delta) { 

    ShooterEnemy *shooter = enemy;

    const int MODE_RESTING = 0;
    const int MODE_SHOOTING = 1;

    const int PREFFERED_DISTANCE = 200;

    enemy_default_handle_state(shooter, delta);

    


    if (shooter->enemy.state == STATE_IDLE) {
        enemy_idle_movement(shooter, delta);
    } else {

        enemy_default_forget_behaviour(shooter, delta);

        shooter->mode_timer -= delta;
        if (shooter->mode_timer <= 0) {
            shooter->mode = shooter->mode == MODE_RESTING? MODE_SHOOTING : MODE_RESTING; // could make this simpler(1 - mode) but it will be less readable
            if (shooter->mode == MODE_RESTING) {
                shooter->mode_timer = 3;
            } else {
                shooter->mode_timer = 2;
            }
        }

        if (shooter->mode == MODE_RESTING) {
            if (shooter->enemy.collided_last_frame || v2_equal(shooter->resting_move_dir, V2_ZERO)) {
                v2 random_dir = v2_get_random_dir();
                shooter->resting_move_dir = random_dir;
            }
            v2 final = v2_rotate(shooter->resting_move_dir, v2_get_angle(shooter->enemy.dir_to_player));
            shooter->enemy.move_dir = v2_lerp(shooter->enemy.move_dir, final, delta);
            shooter->enemy.speed_multiplier = 0.8;
            shooter->enemy.dir = v2_lerp(shooter->enemy.dir, shooter->enemy.dir_to_player, delta * 5);
        } else {
            if (shooter->enemy.collided_last_frame || v2_equal(shooter->attacking_move_dir, V2_ZERO)) {
                v2 random_dir = v2_get_random_dir();
                shooter->attacking_move_dir = random_dir;
            }
            v2 final = v2_rotate(shooter->attacking_move_dir, v2_get_angle(shooter->enemy.dir_to_player));
            shooter->enemy.move_dir = v2_lerp(shooter->enemy.move_dir, final, delta);
            shooter->enemy.speed_multiplier = 1.2;
            shooter->enemy.dir = v2_lerp(shooter->enemy.dir, shooter->enemy.dir_to_player, delta * 5);

            shooter->attacking_shoot_timer -= delta;
            if (shooter->attacking_shoot_timer <= 0) {
                enemy_shooter_shoot(shooter);
                shooter->attacking_shoot_timer = 0.65;
            }
        }
    }


    enemyTick((Enemy *)shooter, delta);
}

ShooterEnemy *enemy_shooter_create(v2 pos) {
    ShooterEnemy *shooter = malloc(sizeof(ShooterEnemy));
    if (shooter == NULL) {
        printf("Failed to allocate memory \n");
        return NULL;
    }
    shooter->enemy = createEnemy(pos, NULL);
    shooter->enemy.maxHealth = 8;
    shooter->enemy.health = shooter->enemy.maxHealth;
    shooter->enemy.hit_texture = shooter_hit_texture;
    shooter->enemy.tick = shooterTick;
    shooter->enemy.on_take_dmg = NULL;

    shooter->resting_move_dir = V2_ZERO;
    shooter->attacking_move_dir = V2_ZERO;
    shooter->mode = 1;
    shooter->mode_timer = 2;
    shooter->attacking_shoot_timer = 0.65;

    shooter->shootCooldown = 2;
    shooter->shootCooldownTimer = 0;


    return shooter;
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
        case (int)PLAYER:;
            
            break;
        case (int)BULLET: ;
            Bullet *b = val;
            
            break;
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
}

BakedLightColor _lerp_baked_light_color(BakedLightColor a, BakedLightColor b, double w) {
    return (BakedLightColor) {
        lerp(a.r, b.r, w),
        lerp(a.g, b.g, w),
        lerp(a.b, b.b, w)
    };
}

void bake_lights() {
   
    init_loading_screen();

    const int CALC_RES = BAKED_LIGHT_CALC_RESOLUTION; // directly affects performance!

    
    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {

            if (r % 100 == 0 && c == 0) {
                double max_progress = 0.33;
                double progress = (double)r / (TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION);
                update_loading_progress(progress * max_progress);
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

                baked_light_grid[r][c].r += col.r;
                baked_light_grid[r][c].g += col.g;
                baked_light_grid[r][c].b += col.b;
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
    update_loading_progress(0.66);


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

            SDL_Color tex_color = {
                SDL_clamp(color.r * 50, 0, 255),
                SDL_clamp(color.g * 50, 0, 255),
                SDL_clamp(color.b * 50, 0, 255),
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
    return (WALL_HEIGHT * WINDOW_HEIGHT) * fov_factor;
}

bool is_enemy_type(int type) {
    if (in_range(type, ENEMY_START, ENEMY_END)) {
        return true;
    }
    return false;
}

void reset_level() {

    printf("player health: %d \n", player->health);

    if (isValidLevel(levelToLoad)) {
        load_level(levelToLoad);
    } else {
        load_level("Levels/default_level.hclevel");
    }
}

void player_take_dmg(double dmg) {

    printf("Damage: %.2f Health: %.2f \n", dmg, player->health);

    player->health -= dmg;

    // play some effect or animation
    play_sound(player_default_hurt, 0.5);
    shakeCamera(20, 10, true, 2);

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
    
    reset_level();
}

void init_loading_screen() {
    
    is_loading = true;
    loading_progress = 0;

    GPU_Clear(screen);

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

    GPU_RectangleFilled2(screen, bar_container, GPU_MakeColor(255, 255, 255, 255));

    GPU_RectangleFilled2(screen, bar_background, GPU_MakeColor(0, 0, 0, 255));

    GPU_Flip(screen);

}

void update_loading_progress(double progress) {

    GPU_Clear(screen);

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

    GPU_RectangleFilled2(screen, bar_container, GPU_MakeColor(255, 255, 255, 255));

    GPU_RectangleFilled2(screen, bar_background, GPU_MakeColor(0, 0, 0, 255));

    GPU_RectangleFilled2(screen, bar, GPU_MakeColor(255, 255, 255, 255));

    GPU_Flip(screen);

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

    remove_game_object(bullet, ENEMY_SHOOTER); 
}

int charge_time_to_shots(double charge_time) {
    return (int)(charge_time * 3);
}

void enemy_move(Enemy *enemy, double delta) {

    v2 vel = v2_mul(enemy->move_dir, to_vec(enemy->speed * enemy->speed_multiplier));
    vel = v2_mul(vel, to_vec(delta));

    enemy->vel = v2_lerp(enemy->vel, vel, delta * 5);

    enemy->entity.pos = v2_add(enemy->entity.pos, enemy->vel);
    
}

void enemy_handle_collisions(Enemy *enemy) {
    CollisionData coldata = getCircleTileMapCollision(*enemy->collider);
    if (coldata.didCollide) {
        enemy->entity.pos = v2_add(enemy->entity.pos, coldata.offset);
        enemy->collided_last_frame = true;
    }
}

void enemy_idle_movement(Enemy *enemy, double delta) {
    double dist_to_wander_pos = v2_distance_squared(enemy->entity.pos, enemy->current_wander_pos);
    v2 dir_to_wander_pos = v2_dir(enemy->entity.pos, enemy->current_wander_pos);

    enemy->wander_pause_timer -= delta;
    if (enemy->wander_pause_timer <= 0 || enemy->collided_last_frame) {
        v2 random_dir = v2_rotate((v2){1, 0}, randf_range(0, 360));
        RayCollisionData ray = castRay(enemy->entity.pos, random_dir);
        double max_travel_dist = 200;
        if (ray.hit) max_travel_dist = min(max_travel_dist, v2_distance(ray.startpos, ray.collpos));

        double travel_dist = randf_range(max_travel_dist / 2, max_travel_dist);
        enemy->current_wander_pos = v2_add(enemy->home_pos, v2_mul(random_dir, to_vec(travel_dist)));

        enemy->wander_pause_timer = 6;
    }

    if (dist_to_wander_pos > 10) {
        enemy->dir = dir_to_wander_pos;
        enemy->move_dir = enemy->dir;
        enemy->speed_multiplier = 0.7;
    } else {
        enemy->move_dir = V2_ZERO;
        enemy->speed_multiplier = 0;
    }
}

void enemy_notice_player_effect(Enemy *enemy, double delta) {

    if (enemy == NULL) return;

    Sprite *sprite = createSprite(true, 1);
    sprite->animations[0] = create_animation(6, 0, exclam_notice_anim);
    sprite->animations[0].fps = 10;
    sprite->animations[0].loop = false;
    spritePlayAnim(sprite, 0);

    Effect *effect = createEffect(enemy->entity.pos, to_vec(7000), sprite, 0.51);
    effect->entity.height = get_max_height() * 3/4;
    effect->entity.affected_by_light = false;

    add_game_object(effect, EFFECT);
}

void enemy_notice_player(Enemy *enemy, double delta) {
    if (enemy->noticed_player) return;
    enemy->noticed_player = true;
    enemy_notice_player_effect(enemy, delta);

    enemy->dir = v2_dir(enemy->entity.pos, player->pos);
    enemy_pause(enemy, enemy->time_to_pursue);
}

void enemy_default_handle_state(Enemy *enemy, double delta) {
    if (enemy->seeingPlayer) {
        if (!enemy->noticed_player) {
            enemy_notice_player(enemy, delta);
        }
        enemy->forget_timer = enemy->time_to_forget;

    } else if (enemy->noticed_player) {
        enemy->forget_timer -= delta;

        if (enemy->forget_timer <= 0) {
            enemy->noticed_player = false;
            enemy->state = STATE_IDLE;
        } else {
            enemy->dir = v2_dir(enemy->entity.pos, player->pos);
        }

    } else {
        enemy->state = STATE_IDLE;
    }
    
    if (enemy->noticed_player) {

        enemy->pursue_timer -= delta;

        if (enemy->pursue_timer <= 0) {
            enemy->state = STATE_PURSUING;
        }
    } else {
        enemy->pursue_timer = enemy->time_to_pursue;
    }
}   

void enemy_pause(Enemy *enemy, double sec) {
    enemy->pause_timer = sec;
}

ExploderEnemy *enemy_exploder_create(v2 pos) {
    ExploderEnemy *exploder = malloc(sizeof(ExploderEnemy));

    exploder->explosion_kb = 4;
    exploder->explosion_radius = 75;
    
    const int ANIM_LENGTH = 5;

    DirectionalSprite *dir_sprite = createDirSprite(16);
    for (int i = 0; i < 16; i++) {
        Sprite *sprite = createSprite(true, 2);
        sprite->animations[0] = create_animation(ANIM_LENGTH, 0, &exploder_frames[i * 5]);
        sprite->animations[1] = create_animation(1, 0, &exploder_frames[i * 5]);
        if (&sprite->animations[0] == NULL || &sprite->animations[1] == NULL) {
            printf("Something went wrong \n");
            exit(1);
        }

        dir_sprite->sprites[i] = sprite;
    }
    dir_sprite_play_anim(dir_sprite, 1);
    
    exploder->enemy = createEnemy(pos, dir_sprite);
    exploder->enemy.tick = exploder_tick;
    exploder->enemy.on_take_dmg = exploder_on_take_dmg;
    exploder->enemy.hit_texture = exploder_hit;
    exploder->enemy.time_to_pursue = 2;
    exploder->enemy.speed = 90;

    exploder->enemy.entity.height = get_max_height() * 0.18;
    return exploder;
}

bool is_entity_type(int type) {
    return in_range(type, ENTITY_START, ENTITY_END);
}

void exploder_tick(Enemy *enemy, double delta) {
    
    ExploderEnemy *exploder = enemy;

    enemy_default_handle_state(exploder, delta);

    if (exploder->enemy.state == STATE_IDLE) {
        enemy_idle_movement(exploder, delta);
    } else {
        
        exploder->enemy.move_dir = exploder->enemy.dir_to_player;
        exploder->enemy.dir = exploder->enemy.dir_to_player;
        exploder->enemy.speed_multiplier = 1;

        enemy_default_forget_behaviour(exploder, delta);

        if (exploder->enemy.dist_squared_to_player < 400) {
            exploder_explode(exploder);
            return;
        }
    }

    if (v2_equal(V2_ZERO, exploder->enemy.move_dir) || exploder->enemy.pause_timer > 0 || exploder->enemy.speed_multiplier == 0) {
        dir_sprite_play_anim(exploder->enemy.dirSprite, 1);
    } else {
        dir_sprite_play_anim(exploder->enemy.dirSprite, 0);
    }

    enemyTick(exploder, delta);
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

void exploder_explode(ExploderEnemy *exploder) {

    play_spatial_sound(exploder_explosion, 1, exploder->enemy.entity.pos, player->pos, exploder->enemy.sound_max_radius);

    Sprite *sprite = createSprite(true, 1);
    sprite->animations[0] = create_animation(12, 0, exploder_explosion_texture);
    sprite->animations[0].fps = 12;
    sprite->animations[0].loop = false;
    sprite->animations[0].playing = true;

    double size = 24000;
    Effect *effect = createEffect(exploder->enemy.entity.pos, to_vec(size), sprite, 1);
    effect->entity.affected_by_light = false;
    effect->entity.height = size / 2;

    add_game_object(effect, EFFECT);

    for (int i = 0; i < gameobjects->length; i++) {
        obj *gameobject = arraylist_get(gameobjects, i);
        if (is_enemy_type(gameobject->type) && gameobject->val != (void *)exploder) {
            Enemy *enemy = gameobject->val;
            double explosion_radius_squared = exploder->explosion_radius * exploder->explosion_radius;
            double dist_squared = v2_distance_squared(enemy->entity.pos, exploder->enemy.entity.pos);
            if (dist_squared < explosion_radius_squared) {
                double kb = lerp(exploder->explosion_kb, 0, inverse_lerp(0, explosion_radius_squared, dist_squared));
                kb *= 2;
                enemy->vel = v2_mul(v2_dir(exploder->enemy.entity.pos, enemy->entity.pos), to_vec(kb));
                enemyTakeDmg(enemy, 3);
            }
            
        }
    }

    double explosion_radius_squared = exploder->explosion_radius * exploder->explosion_radius;
    double dist_squared = v2_distance_squared(player->pos, exploder->enemy.entity.pos);
    if (dist_squared < explosion_radius_squared) {
        double kb = lerp(exploder->explosion_kb, 0, inverse_lerp(0, explosion_radius_squared, dist_squared));
        player->vel = v2_mul(v2_dir(exploder->enemy.entity.pos, player->pos), to_vec(kb));
        player_take_dmg(1);
    }




    remove_game_object(exploder, ENEMY_EXPLODER);
}


Ability ability_primary_shoot_create() {
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

Rapidfire ability_secondary_shoot_create() {
    return (Rapidfire) {
        .ability.activate = ability_secondary_shoot_activate,
        .ability.tick = default_ability_tick,
        .ability.before_activate = NULL,
        .ability.can_use = false,
        .ability.cooldown = 8,
        .ability.delay = 0,
        .ability.delay_timer = -1000,
        .ability.timer = 0,
        .ability.type = A_SECONDARY,
        .ability.texture = shotgun_icon,
        .shot_amount = 16,
        .shot_timer = 0.06,
        .shots_left = 0
    };
}

void ability_secondary_shoot_activate(Ability *ability) {

    shakeCamera(35, 15, true, 10);
    play_sound(rapidfire_sound, 0.8);

    Rapidfire *rapid_fire = (Rapidfire *)ability;
    rapid_fire->shots_left = rapid_fire->shot_amount;

    while (rapid_fire->shots_left > 0) {
        _shoot(0.05);
        rapid_fire->shots_left--;
    }

    player->vel = v2_mul(playerForward, to_vec(-5));
}

void _shoot(double spread) {

    double pitch = player->pitch + randf_range(1000 * -spread, 1000 * spread);

    play_sound(player_default_shoot, 0.3);
    shakeCamera(10, 4, true, 1);
    spritePlayAnim(leftHandSprite, 1);

    v2 shoot_dir = v2_rotate(playerForward, randf_range(-PI * spread, PI * spread));


    RayCollisionData ray_data = castRayForAll(player->pos, shoot_dir);

    double effect_height = get_max_height() / 2;
    
    double max_distance;

    if (in_range(pitch, -0.01, 0.01)) {
        max_distance = INFINITY;
    } else {
        double p = abs(pitch); // for debugging
        max_distance = 9000 / p;
    }

    if (!ray_data.hit && max_distance == INFINITY) return;


    bool hit_ground_or_ceiling = false;

    double distance_to_collpos = v2_distance(player->pos, ray_data.collpos);

    if (distance_to_collpos > max_distance) {
        effect_height = pitch < 0? get_max_height() : 0;
        hit_ground_or_ceiling = true;
    }

    
    if (is_enemy_type(ray_data.colliderType) && !hit_ground_or_ceiling) {
        
        Entity entity = *((Entity *)ray_data.collider);
        effect_height = entity.height;
        
        enemy_pause(ray_data.collider, 0.1);
        enemyTakeDmg(ray_data.collider, 1);
    } else if (!hit_ground_or_ceiling) {
        
        effect_height = get_max_height() / 2 + -pitch * distance_to_collpos; // an estimate. wouldnt work well with high spread, but hopefully hard to notice. nvm, works very well.
    }
    

    v2 effectPos;

    if (hit_ground_or_ceiling) {
        effectPos = v2_add(player->pos, v2_mul(shoot_dir, to_vec(max_distance)));
    } else {
        effectPos = v2_add(ray_data.collpos, v2_mul(playerForward, to_vec(-4)));
    }

    Effect *hitEffect = createEffect(effectPos, to_vec(8000), createSprite(true, 1), 1);

    if (hitEffect == NULL) {
        printf("hit effect is null \n");
        return;
    }

    hitEffect->entity.sprite->animations[0] = create_animation(5, 0, shootHitEffectFrames);
    
    hitEffect->entity.sprite->animations[0].fps = 12;
    hitEffect->entity.sprite->animations[0].loop = false;
    hitEffect->entity.height = effect_height;
    hitEffect->entity.affected_by_light = false;

    spritePlayAnim(hitEffect->entity.sprite, 0);

    add_game_object(hitEffect, EFFECT);
    
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
        .spawn_rate = 8,
        .spread = 0.1,
        .min_speed = 10,
        .max_speed = 10,
        .min_size = to_vec(5),
        .max_size = to_vec(15),
        .floor_drag = 0.01,
        .bounciness = 0.5,
        .particle_lifetime = 1,
        .active = true
    };

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
    
    double speed = randf_range(spawner->min_speed, spawner->max_speed);
    v2 dir = v2_rotate(spawner->dir, randf_range(-PI * spawner->spread, PI * spawner->spread));
    
    double vel_x = dir.x * speed;
    double vel_y = dir.y * speed;
    double vel_z = spawner->height_dir * speed;

    particle.vel = (v2){vel_x, vel_y};
    particle.h_vel = vel_z;

    particle.accel = spawner->accel;
    particle.h_accel = spawner->height_accel;

    particle.effect.lifeTime = spawner->particle_lifetime;

    particle.bounciness = spawner->bounciness;
    particle.floor_drag = spawner->floor_drag;

    particle.effect.entity.pos = spawner->pos; // change later
    particle.effect.entity.height = spawner->height;
    particle.effect.entity.affected_by_light = true;
    
    double size_rand = randf_range(0, 1);
    v2 size = v2_lerp(spawner->min_size, spawner->max_size, size_rand);
    
    particle.effect.entity.size = size;

    particle.effect.entity.sprite = malloc(sizeof(Sprite));
    *particle.effect.entity.sprite = spawner->sprite;


    Particle *particle_object = malloc(sizeof(Particle));
    *particle_object = particle;

    add_game_object(particle_object, PARTICLE);

}


void particle_tick(Particle *particle, double delta) {

    particle->vel = v2_add(particle->vel, v2_mul(particle->accel, to_vec(delta)));
    particle->h_vel += particle->h_accel * delta;

    particle->effect.entity.pos = v2_add(particle->effect.entity.pos, v2_mul(particle->vel, to_vec(delta)));
    particle->effect.entity.height += particle->h_vel * delta;
    double floor_bound = particle->effect.entity.size.y / 2;
    if (particle->effect.entity.height < floor_bound) {
        particle->effect.entity.height = floor_bound;
        particle->h_vel *= -particle->bounciness;
        particle->vel = v2_mul(particle->vel, to_vec(1 - particle->floor_drag));
    }

    effectTick((Effect *)particle, delta);
}

void exploder_on_take_dmg(Enemy *enemy, double dmg) {
    
    if (enemy->health < 2) {
        exploder_explode(enemy);
    }
}

void place_entity(v2 pos, int type) {
    switch (type) {
        case (int)P_PLAYER:
            init_player(pos);
            add_game_object(player, PLAYER);
            break;
        case (int)P_SHOOTER:;
            ShooterEnemy *shooter = enemy_shooter_create(pos);
            add_game_object(shooter, ENEMY_SHOOTER);
            break;
        case (int)P_EXPLODER:;
            ExploderEnemy *exploder = enemy_exploder_create(pos);
            add_game_object(exploder, ENEMY_EXPLODER);
    }
}

void enemy_default_forget_behaviour(Enemy *enemy, double delta) {
    if (enemy->forget_timer > 0 && !enemy->seeingPlayer) { // speed multiplier could be whatever it was before
        enemy->move_dir = v2_dir(enemy->entity.pos, enemy->last_seen_player_pos);
        enemy->dir = enemy->move_dir;
    }
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
    fov = startFov * 1.3;


    RayCollisionData ray = castRay(pos, dir);

    

    if (ray.hit) {
        double dist_sqr = v2_distance_squared(ray.startpos, ray.collpos);
        if (dist_sqr < dash_distance * dash_distance) {
            dash_distance = sqrt(dist_sqr) - 1;
        }
    }

    player->pos = v2_add(player->pos, v2_mul(dir, to_vec(dash_distance)));

}

void activate_ability(Ability *ability) {
    
    if (ability == NULL) return;

    if (!ability->can_use) return;

    ability->can_use = false;

    if (ability->before_activate != NULL) {
        ability->before_activate(ability);
    }

    ability->delay_timer = ability->delay;
}

void ability_dash_before_activate(Ability *ability) {
    spritePlayAnim(dash_anim_sprite, 0);
}


// #END


#pragma GCC diagnostic pop