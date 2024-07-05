
#include "game_utils.c"
#include "globals.h"


SDL_Renderer *renderer;
SDL_Window *window;

// #DEFINITIONS

#define TPS 120
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 580
#define RESOLUTION_X 360
#define RESOLUTION_Y 180
#define X_SENSITIVITY 0.1
#define Y_SENSITIVITY 0.8
#define COLOR_BLACK \
    (SDL_Color) { 0, 0, 0 }
#define TRANSPARENT \
    (SDL_Color) { 0, 0, 0, 0 }
#define RENDER_DISTANCE 350
#define WALL_HEIGHT 30
#define NUM_WALL_THREADS 1

#define BAKED_LIGHT_RESOLUTION 36

#define OUT_OF_SCREEN_POS \
    (v2) { WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100 }

enum Types { 
    PLAYER, 
    RAYCAST, 
    CIRCLE_COLLIDER, 
    RAY_COLL_DATA, 
    RENDER_OBJECT, 
    WALL_STRIPE, 
    LIGHT_POINT, 
    SPRITE, 
    PARTICLES, 
    PARTICLE, 
    
    BULLET, 
    DIR_SPRITE, 
    
    ENTITY_START,

    ENTITY,
    EFFECT,
        ENEMY_START,
        
        ENEMY,
        ENEMY_SHOOTER,
        ENEMY_EXPLODER,
        
        ENEMY_END,

    ENTITY_END
};

enum Tiles { WALL1 = 1, WALL2 = 2 };

typedef enum {
    STATE_IDLE,
    STATE_PURSUING
} EnemyState;

// #STRUCTS

typedef struct Animation {
    SDL_Texture **frames;
    int frameCount;
    int frame;
    double fps;
    double timeToNextFrame;
    bool loop;
    bool playing;
    int priority;
} Animation;

typedef struct Sprite {
    SDL_Texture *texture;  // not used if animated
    bool isAnimated;
    int currentAnimationIdx;
    Animation *animations;
    int animCount;
} Sprite;

typedef struct DirectionalSprite {
    Sprite **sprites;
    v2 dir;  // global direction
    int dirCount;
} DirectionalSprite;

typedef struct Raycast {
    v2 pos, dir;
} Raycast;

typedef struct RayCollisionData {
    v2 startpos, collpos, normal;
    void *collider;
    int colliderType;
    SDL_Texture *colliderTexture;
    bool hit;
    double collIdx, wallWidth;
} RayCollisionData;

typedef struct CircleCollider {
    v2 pos;
    double radius;
} CircleCollider;

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
    int health, maxHealth;
    CircleCollider *collider;
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

typedef struct Particle {
    Entity entity;
    v2 vel;
    double lifeTimer;
} Particle;

typedef struct Particles {
    v2 pos;
    double height;

    Particle **particles;
    v2 particleSize;
    int particleAmount;
    double particleLifetime;
    Sprite *particleSprite;
    v2 startVel;
    v2 gravity;
} Particles;

typedef struct Effect {
    Entity entity;
    double lifeTime;
} Effect;

typedef struct RenderObject {
    void *val;
    int type;
    double dist_squared;
} RenderObject;

typedef struct Enemy {
    Entity entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
    SDL_Texture *hit_texture;
    v2 dir;
    double speed, speed_multiplier;
    bool seeingPlayer;
    v2 lastSeenPlayerPos;
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


} Enemy;

typedef struct EnemyBullet {
    Entity entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
    v2 dir;
    double dmg;
    double speed;
    double lifeTime;
    double lifeTimer;
    

} EnemyBullet;

typedef struct BakedLightColor {
    double r, g, b;
} BakedLightColor;

// ENEMY IDEAS:
// Regular pew pew enemy
// Small enemy that chases you and explodes
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
} ExploderEnemy;

// #ENEMIES END

typedef struct CollisionData {
    v2 offset;  // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;

// #FUNC

void exploder_tick(ExploderEnemy *exploder, double delta);

bool is_entity_type(int type);

ExploderEnemy *enemy_exploder_create(v2 pos);

void enemy_pause(Enemy *enemy, double sec);

void enemy_default_handle_state(Enemy *enemy, double delta);

void enemy_notice_player_effect(Enemy *enemy, double delta);

void enemy_idle_movement(Enemy *enemy, double delta);

void enemy_handle_collisions(Enemy *enemy);

void enemy_move(Enemy *enemy, double delta);

int charge_time_to_shots(double charge_time);

void enemy_bullet_destroy(EnemyBullet *bullet);

SDL_Color lerp_color(SDL_Color col1, SDL_Color col2, double w);

void remove_loading_screen();

void update_loading_progress(double progress);

void init_loading_screen();

void player_die();

void player_take_dmg(int dmg);

void reset_level();

bool is_enemy_type(int type);

double get_max_height();

v2 worldToScreen(v2 pos, double height);

void clampColors(int rgb[3]);

BakedLightColor get_light_color_by_pos(v2 pos);

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

Animation create_animation(int frameCount, int priority, SDL_Texture **frames);

void freeAnimation(Animation *anim);

SDL_Texture *getSpriteCurrentTexture(Sprite *sprite);

Sprite *getDSpriteCurrentSprite(DirectionalSprite *dSprite, v2 spritePos);

void objectTick(void *obj, int type, double delta);

void playerTick(double delta);

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, double delta);

void spriteTick(Sprite *sprite, double delta);

void enemyTick(Enemy *enemy, double delta);

void animationTick(Animation *anim, double delta);

void effectTick(Effect *effect, double delta);

void bulletTick(EnemyBullet *bullet, double delta);

void shooterTick(ShooterEnemy *shooter, double delta);

Sprite *createSprite(bool isAnimated, int animCount);

DirectionalSprite *createDirSprite(int dirCount);

void spritePlayAnim(Sprite *sprite, int idx);

Sprite *getRandomWallSprite();

Particles *createParticles(int amount, Sprite *sprite);

void particlesTick(Particles *particles, u64 delta);

void playerShoot();

void enemyTakeDmg(Enemy *enemy, int dmg);

void renderTexture(SDL_Texture *texture, v2 pos, v2 size, double height, bool affected_by_light);

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height);

double angleDist(double a1, double a2);

void shakeCamera(double strength, int ticks, bool fade, int priority);

ShooterEnemy *enemy_shooter_create(v2 pos);



bool isValidLevel(char *file);

void getTextureFiles(char *fileName, int fileCount, SDL_Texture ***textures);

// #FUNC END



// #TEXTURES
SDL_Texture *mimran_jumpscare;
SDL_Texture *shooter_hit_texture;
SDL_Texture *healthbar_texture;
SDL_Texture *vignette_texture;
SDL_Texture *enemy_bullet_texture;
SDL_Texture *floorAndCeiling;
SDL_Texture *wallTexture;
SDL_Texture *entityTexture;
SDL_Texture *crosshair;
SDL_Texture *fenceTexture;
SDL_Texture *skybox_texture;
SDL_Texture **wallFrames;
SDL_Texture **shootHitEffectFrames;
SDL_Texture **enemy_bullet_destroy_anim;
SDL_Texture **exclam_notice_anim;

TextureData *floorTexture;
TextureData *floorTexture2;
TextureData *floorLightTexture;
TextureData *ceilingTexture;
TextureData *ceilingLightTexture;

// #SPRITES
Sprite *animatedWallSprite;
Sprite *leftHandSprite;

// #SOUNDS
Sound *enemy_default_hit;
Sound *enemy_default_kill;
Sound *player_default_shoot;
Sound *player_default_hurt;

// #VAR
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
const int tileSize = WINDOW_WIDTH / TILEMAP_WIDTH;
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

    window = SDL_CreateWindow("Doom style 3D!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    renderer = SDL_CreateRenderer(window, -1, RENDERER_FLAGS);

    if (argc >= 2) {
        levelToLoad = argv[1];
    }

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

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

void initParticles() {
    Particles *particles = createParticles(8, NULL);
    Sprite *sprite = createSprite(false, 0);
    sprite->texture = entityTexture;
    v2_print(player->pos, "\n");
    particles->particleSprite = sprite;
    particles->pos = player->pos;
    particles->startVel = to_vec(0);
    particles->gravity = to_vec(0);
    particles->height = WINDOW_HEIGHT / 6;
    add_game_object(particles, PARTICLES);
}

Enemy createEnemy(v2 pos) {
    Enemy enemy;
    enemy.maxHealth = 5;
    enemy.health = enemy.maxHealth;
    enemy.dir = (v2){1, 0};
    enemy.move_dir = (v2){0, 0};
    enemy.speed = 50;
    enemy.speed_multiplier = 1;
    enemy.state = STATE_IDLE;

    // idle movement
    enemy.home_pos = pos;
    enemy.current_wander_pos = pos;
    enemy.wander_pause_timer = 10;

    enemy.time_to_forget = 1;
    enemy.forget_timer = enemy.time_to_forget;

    enemy.noticed_player = false;
    enemy.time_to_pursue = 1;
    enemy.pursue_timer = enemy.time_to_pursue;

    enemy.pause_timer = 0;

    enemy.max_vision_distance = 300;

    enemy.dir_to_player = v2_dir(pos, player->pos);
    enemy.dist_squared_to_player = v2_distance_squared(pos, player->pos);

    enemy.collided_last_frame = false;

    enemy.entity.pos = pos;
    enemy.entity.size = to_vec(50);
    enemy.entity.sprite = NULL;
    enemy.entity.affected_by_light = true;
    enemy.hit_texture = NULL;
    enemy.sound_max_radius = 400;

    enemy.dirSprite = createDirSprite(16);
    for (int i = 0; i < 16; i++) {
        char *baseFileName = "Textures/CubeEnemyAnim/CubeEnemy";
        char num[get_num_digits(i + 1)];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(baseFileName, num);
        char *fileWithExtension = concat(fileWithNum, ".bmp");

        enemy.dirSprite->sprites[i] = createSprite(false, 0);
        enemy.dirSprite->sprites[i]->texture = make_texture(renderer, fileWithExtension);
    }
    enemy.dirSprite->dir = (v2){1, 0};

    enemy.entity.height = get_max_height() * 0.1;

    enemy.collider = malloc(sizeof(CircleCollider));
    enemy.collider->radius = 10;
    enemy.collider->pos = enemy.entity.pos;

    enemy.seeingPlayer = false;
    enemy.lastSeenPlayerPos = (v2){0, 0};

    return enemy;
}

void init() {  // #INIT

    exclam_notice_anim = malloc(sizeof(SDL_Texture *) * 6);
    getTextureFiles("Textures/ExclamNoticeAnim/noticeAnim", 6, &exclam_notice_anim);

    player_default_hurt = create_sound("Sounds/player_default_hurt.wav");
    player_default_shoot = create_sound("Sounds/player_default_shoot.wav");
    enemy_default_hit = create_sound("Sounds/enemy_default_hit.wav");
    enemy_default_kill = create_sound("Sounds/enemy_default_kill.wav");

    init_cd_print();

    enemy_bullet_destroy_anim = malloc(sizeof(SDL_Texture *) * 5);

    getTextureFiles("Textures/CubeEnemyAnim/CEBulletHitAnim/CEBullethitAnim", 5, &enemy_bullet_destroy_anim);

    mimran_jumpscare = make_texture(renderer, "Textures/scary_monster2.bmp");

    shooter_hit_texture = make_texture(renderer, "Textures/CubeEnemyAnim/CubeEnemyHit2.bmp");

    enemy_bullet_texture = make_texture(renderer, "Textures/CubeEnemyAnim/CubeEnemyBullet.bmp");

    healthbar_texture = make_texture(renderer, "Textures/health_bar.bmp");

    vignette_texture = make_texture(renderer, "Textures/vignette.bmp");

    fenceTexture = make_texture(renderer, "Textures/fence.bmp");
    SDL_SetTextureBlendMode(fenceTexture, SDL_BLENDMODE_BLEND);

    floorAndCeiling = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, RESOLUTION_X, RESOLUTION_Y);
    SDL_SetTextureBlendMode(floorAndCeiling, SDL_BLENDMODE_BLEND);

    floorTexture = TextureData_from_bmp("Textures/floor.bmp");
    floorLightTexture = TextureData_from_bmp("Textures/floor_light.bmp");
    floorTexture2 = TextureData_from_bmp("Textures/floor2.bmp");
    ceilingTexture = TextureData_from_bmp("Textures/ceiling.bmp");
    ceilingLightTexture = TextureData_from_bmp("Textures/ceiling_light.bmp");

    init_tilemap(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    init_tilemap(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    init_tilemap(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    tanHalfFOV = tan(deg_to_rad(fov / 2));
    tanHalfStartFOV = tan(deg_to_rad(startFov / 2));

    gameobjects = create_arraylist(10);

    wallTexture = make_texture(renderer, "Textures/wall.bmp");

    wallFrames = malloc(sizeof(SDL_Texture *) * 17);
    getTextureFiles("Textures/WallAnim/wallAnim", 17, &wallFrames);

    crosshair = make_texture(renderer, "Textures/crosshair.bmp");

    animatedWallSprite = createSprite(true, 1);
    animatedWallSprite->animations[0] = create_animation(17, 0, wallFrames);
    animatedWallSprite->animations[0].fps = 10;
    spritePlayAnim(animatedWallSprite, 0);

    leftHandSprite = createSprite(true, 2);
    SDL_Texture **default_hand = malloc(sizeof(SDL_Texture *)); 
    default_hand[0] = make_texture(renderer, "Textures/rightHandAnim/rightHandAnim6.bmp");
    leftHandSprite->animations[0] = create_animation(1, 0, default_hand);


    leftHandSprite->animations[1] = create_animation(6, 0, NULL);
    getTextureFiles("Textures/rightHandAnim/rightHandAnim", 6, &leftHandSprite->animations[1].frames);
    leftHandSprite->animations[1].fps = 12;

    leftHandSprite->animations[1].loop = false;
    spritePlayAnim(leftHandSprite, 0);

    shootHitEffectFrames = malloc(sizeof(SDL_Texture *) * 5);

    getTextureFiles("Textures/ShootEffectAnim/shootHitEffect", 5, &shootHitEffectFrames);

    for (int i = 0; i < 26; i++) keyPressArr[i] = false;

    entityTexture = make_texture(renderer, "Textures/scary_monster.bmp");

    if (isValidLevel(levelToLoad)) {
        load_level(levelToLoad);

    } else {
        load_level("Levels/new_default_level.hclevel");
    }

    skybox_texture = make_texture(renderer, "Textures/skybox.bmp");

    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

	

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
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                isLMouseDown = false;
            }

            if (player->shootChargeTimer > 0.4) {
                player->pendingShots = charge_time_to_shots(player->shootChargeTimer);
                player->shootChargeTimer = 0;
                player->ShootTickTimer = 0.15;
            } else if (player->canShoot) {
                player->pendingShots = 0;
                player->shootChargeTimer = 0;
                playerShoot();
                player->canShoot = false;
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
        player->sprinting = true;
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

    bool reached_max_charge = charge_time_to_shots(player->shootChargeTimer) == player->max_pending_shots;

    if (isLMouseDown) {
        if (!reached_max_charge) player->shootChargeTimer += delta;
        
        shakeCamera(80 * min(15, player->shootChargeTimer), 5, false, 0);
        
        speed_multiplier *= 0.5;
    }


    if (player->pendingShots > 0) {
        player->ShootTickTimer -= delta;

        if (player->ShootTickTimer <= 0) {
            player->ShootTickTimer = 0.15;
            playerShoot();
            player->pendingShots -= 1;
        }
    }

    v2 right = {1, 0};
    v2 move_dir = v2_rotate_to(right, deg_to_rad(player->angle));
    v2 move_dir_rotated = v2_rotate(move_dir, PI / 2);

    v2 keyVec = get_key_vector(SDLK_s, SDLK_w, SDLK_a, SDLK_d);

    if (!v2_equal(keyVec, to_vec(0))) {
        double t = sin(mili_to_sec(SDL_GetTicks64()) * (15 + (int)player->sprinting * 5)) * 3;
        //player->pitch = t;
        player->handOffset.y = t * 2.5;
    } else {
        //player->pitch = lerp(player->pitch, 0, 0.1);
        player->handOffset.y = lerp(player->handOffset.y, 0, 0.1);
    }

    player->vel = v2_lerp(player->vel, v2_add(v2_mul(move_dir, to_vec(keyVec.x)), v2_mul(move_dir_rotated, to_vec(keyVec.y))), 0.15);

    CollisionData player_coldata = getCircleTileMapCollision(*player->collider);
    if (player_coldata.didCollide) {
        player->pos = v2_add(player->pos, player_coldata.offset);
    }
    if (player->sprinting) {
        speed_multiplier *= 1.5;
        fov = lerp(fov, startFov * 1.2, 0.1);
    } else {
        fov = lerp(fov, startFov, 0.1);
    }

    v2 finalVel = v2_mul(player->vel, to_vec(player->speed * speed_multiplier));
    if (delta == 0) {
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(0.016)));
    } else {
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(delta)));
    }

    if (!player->canShoot) {
        player->shootCooldown -= delta;
        if (player->shootCooldown <= 0) {
            player->canShoot = true;
            player->shootCooldown = PLAYER_SHOOT_COOLDOWN;
        }
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
    
    switch (type) {
        case (int)PLAYER:
            playerTick(delta);
            break;
        case (int)EFFECT:
            effectTick(obj, delta);
            break;
        case (int)SPRITE:;
            Sprite *sprite = obj;
            animationTick(&sprite->animations[sprite->currentAnimationIdx], delta);
            break;
        case (int)PARTICLES:
            particlesTick(obj, delta);
            break;
        case (int)BULLET:
            bulletTick(obj, delta);
            break;
        case (int)ENEMY_SHOOTER:
            shooterTick(obj, delta);
            break;
        case (int)ENEMY_EXPLODER:
            exploder_tick(obj, delta);
            break;
    }
}

// #TICK
void tick(double delta) {

    vignette_color = lerp_color(vignette_color, (SDL_Color){0, 0, 0}, delta);

    playerForward = get_player_forward();

    

    tanHalfFOV = tan(deg_to_rad(fov / 2));

    if (lockMouse) {
        SDL_WarpMouseInWindow(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
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
                cameraOffset = v2_mul(rawShake, to_vec((double)cameraShakeTicksLeft / cameraShakeTicks));
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
}

// for slower decay, make 'a' smaller.
double distance_to_color(double distance, double a) {
    return exp(-a * distance);
}

void renderDebug() {  // #DEBUG

    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int resolution = 2;

    for (int row = 0; row < TILEMAP_HEIGHT * resolution; row++) {
        for (int col = 0; col < TILEMAP_WIDTH * resolution; col++) {
            BakedLightColor color = baked_light_grid[row  * BAKED_LIGHT_RESOLUTION/ resolution][col * BAKED_LIGHT_RESOLUTION / resolution];
            SDL_SetRenderDrawColor(renderer, clamp(125 * color.r, 0, 255), clamp(125 * color.g, 0, 255), clamp(125 * color.b, 0, 255), 255);
            double y = (double)row * tileSize / resolution;
            double x = (double)col * tileSize / resolution;
            v2 px_size = v2_div((v2){WINDOW_WIDTH, WINDOW_HEIGHT}, to_vec(tileSize * BAKED_LIGHT_RESOLUTION)); 
            SDL_Rect rect = {
                x,
                y,
                tileSize / resolution,
                tileSize / resolution
            };

            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect player_rect = {
        player->pos.x,
        player->pos.y,
        10,
        10
    };
    SDL_RenderFillRect(renderer, &player_rect);
}

v2 getRayDirByIdx(int i) {
    double x = tanHalfFOV;
    double idx = lerp(-x, x, ((double)(i + 1)) / RESOLUTION_X);

    v2 temp = (v2){1, idx};

    temp = v2_normalize(temp);
    v2 rayDir = v2_dir((v2){0, 0}, v2_rotate(temp, deg_to_rad(player->angle)));
    return rayDir;
}

v2 worldToScreen(v2 pos, double height) {
    // know: player pos, distance to pos, angle to pos
    // need to find:

    double signed_angle_to_forward = v2_signed_angle_between(playerForward, v2_sub(pos, player->pos));

    double signed_angle_degrees = deg_to_rad(signed_angle_to_forward);

    if (!in_range(signed_angle_degrees, -0.5 * fov, 0.5 * fov)) {
        return OUT_OF_SCREEN_POS;
    }

    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    double dist_to_player = v2_distance(pos, player->pos);

    double dist_to_viewplane = dist_to_player * cos_angle_to_forward;

    double fov_width_at_texture = 2 * dist_to_viewplane * tanHalfFOV;

    double angle = acos(cos_angle_to_forward);

    double texture_thing_width = dist_to_player * sin(angle); 

    double x_pos_sign = signed_angle_to_forward >= 0 ? 1 : -1;
    double x_pos = WINDOW_WIDTH / 2 + (texture_thing_width / fov_width_at_texture * WINDOW_WIDTH) * x_pos_sign;

    double fov_factor = tanHalfStartFOV / tanHalfFOV;
    double wallSize = WALL_HEIGHT * WINDOW_HEIGHT / dist_to_viewplane * fov_factor;
    double y_pos = WINDOW_HEIGHT / 2 + wallSize / 2 - (height / dist_to_viewplane);

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

void calcFloorAndCeiling() {
    // render by scanlines.
    // interpolate between left and right points and put the correct texture

    void *pixels;
    int pitch;

    SDL_LockTexture(floorAndCeiling, NULL, &pixels, &pitch);

    TextureData *textureData = floorTexture;
    v2 textureSize = (v2){textureData->w, textureData->h};

    for (int row = 0; row < RESOLUTION_Y; row++) {

        int screenY = row * WINDOW_HEIGHT / RESOLUTION_Y;

        bool is_ceiling = screenY + player->pitch < WINDOW_HEIGHT / 2;

        v2 left = screenToFloor((v2){0, screenY + player->pitch});
        v2 right = screenToFloor((v2){RESOLUTION_X - 1, screenY + player->pitch});

        for (int col = 0; col < RESOLUTION_X; col++) {

            ((int *)pixels)[row * RESOLUTION_X + col] = 0x000000ff;

            

            int screenX = col * WINDOW_WIDTH / RESOLUTION_X;

            v2 point = v2_lerp(left, right, (double)col / RESOLUTION_X);

            
            double light = 0.8;

            int offsetted_row = row + (player->pitch / WINDOW_HEIGHT * RESOLUTION_Y);

            if (in_range(offsetted_row, RESOLUTION_Y/4, RESOLUTION_Y/2)) {
                light = (1 - inverse_lerp(RESOLUTION_Y/4, RESOLUTION_Y/2, offsetted_row)) * 0.8;
            } else if (in_range(offsetted_row, RESOLUTION_Y/2, RESOLUTION_Y * 3/4)) {
                light = inverse_lerp(RESOLUTION_Y/2, RESOLUTION_Y * 3/4, offsetted_row) * 0.8;
            }

            int color = light * 255;

            int tileRow = point.y / tileSize;
            int tileCol = point.x / tileSize;

            int floorTile = -1;
            int ceilingTile = -1;

            if (in_range(tileRow, 0, TILEMAP_HEIGHT - 1) && in_range(tileCol, 0, TILEMAP_WIDTH - 1)) {
                floorTile = floorTileMap[tileRow][tileCol];
                ceilingTile = ceilingTileMap[tileRow][tileCol];
            }

            bool has_ceiling = ceilingTile != -1;

            if (is_ceiling && !has_ceiling) {
                continue;
            } else if (has_ceiling && is_ceiling) {
                if (ceilingTile == P_CEILING) {
                    textureData = ceilingTexture;
                } else if (ceilingTile == P_CEILING_LIGHT) {
                    textureData = ceilingLightTexture;
                }
            } else { // its a floor
                if (floorTile == P_FLOOR) {
                    textureData = floorTexture2;
                } else if (floorTile == P_FLOOR_LIGHT) {
                    textureData = floorLightTexture;
                } else {
                    textureData = floorTexture;
                }
            }

            int floor_row = row;
            int floor_col = col;

            floor_row = clamp(floor_row, 0, RESOLUTION_Y - 1);
            floor_col = clamp(floor_col, 0, WINDOW_WIDTH - 1);

            int floor_pixel_idx = floor_row * RESOLUTION_X + floor_col;

            Pixel floor_pixel = TextureData_get_pixel(
                textureData,
                loop_clamp(point.x / tileSize * 36, 0, 36), 
                loop_clamp(point.y / tileSize * 36, 0, 36)
            );

            floor_pixel.r *= light;
            floor_pixel.g *= light;
            floor_pixel.b *= light;

			BakedLightColor baked_light_color = get_light_color_by_pos(point);

            int rgb[3] = {
                floor_pixel.r * baked_light_color.r,
                floor_pixel.g * baked_light_color.g,
                floor_pixel.b * baked_light_color.b
            };

            clampColors(rgb);

			floor_pixel.r = rgb[0];
			floor_pixel.g = rgb[1];
			floor_pixel.b = rgb[2];
            

            int floor_pixel_i = floor_pixel.r << 24 | floor_pixel.g << 16 | floor_pixel.b << 8 | floor_pixel.a;

            ((int *)pixels)[floor_pixel_idx] = floor_pixel_i;

        }
    }

    SDL_UnlockTexture(floorAndCeiling);
}

void drawFloorAndCeiling() {
    drawSkybox();

    v2 offsets = (v2){cameraOffset.x, cameraOffset.y};

    SDL_Rect rect = {offsets.x, offsets.y, WINDOW_WIDTH, WINDOW_HEIGHT};

    SDL_RenderCopy(renderer, floorAndCeiling, NULL, &rect);
}

void renderTexture(SDL_Texture *texture, v2 pos, v2 size, double height, bool affected_by_light) {
    

    v2 screen_pos = worldToScreen(pos, height);

    if (v2_equal(screen_pos, OUT_OF_SCREEN_POS)) {
        return;
    }
    double cos_angle_to_forward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    double dist_to_player = v2_distance(pos, player->pos);

    double dist_to_viewplane = dist_to_player * cos_angle_to_forward;

    v2 final_size = v2_div(size, to_vec(dist_to_player));
    final_size = v2_div(to_vec(300 * WALL_HEIGHT), to_vec(dist_to_viewplane));

    SDL_Rect dstRect = {
        screen_pos.x - final_size.x / 2,
        screen_pos.y - final_size.y / 2,
        final_size.x,
        final_size.y
    };

    int rgb[3] = {255, 255, 255};
    if (affected_by_light) {
        double light;
        if (dist_to_player > 150) {
            light = distance_to_color(dist_to_player - 150, 0.01) * 0.8;
        } else {
            light = 0.8;
        }

        int color = 255 * light;
    
        BakedLightColor baked_color = get_light_color_by_pos(pos);

        rgb[0] = color * baked_color.r;
        rgb[1] = color * baked_color.g;
        rgb[2] = color * baked_color.b;

        clampColors(rgb);
    }
    

    SDL_SetTextureColorMod(texture, rgb[0], rgb[1], rgb[2]);
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_SetTextureColorMod(texture, 255, 255, 255);
}

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height) {
    SDL_Texture *texture = getSpriteCurrentTexture(getDSpriteCurrentSprite(dSprite, pos));

    renderTexture(texture, pos, size, height, true);
}

void renderEntity(Entity entity) {  // RENDER ENTITY
    SDL_Texture *texture = getSpriteCurrentTexture(entity.sprite);

    renderTexture(texture, entity.pos, entity.size, entity.height, entity.affected_by_light);
}

typedef struct WallStripe {
    SDL_Texture *texture;
    double size;
    int i;
    double brightness;  // a bunch of rendering bullshit:
    v2 pos, normal;
    double collIdx;
    double wallWidth;
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

    SDL_Thread *threads[NUM_WALL_THREADS];

    int indicies[NUM_WALL_THREADS];  // trash lang

    for (int i = 0; i < NUM_WALL_THREADS; i++) {
        indicies[i] = i;
        threads[i] = SDL_CreateThread(addWallStripes_Threaded, "thread wall", &indicies[i]);
    }
    for (int i = 0; i < NUM_WALL_THREADS; i++) {
        SDL_WaitThread(threads[i], NULL);
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

        } else if (object->type == BULLET) {

            currentRObj->val = object->val;
            currentRObj->type = BULLET;
            pos = ((EnemyBullet *)object->val)->entity.pos;
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
    double brightness = stripe->brightness;

    double angleLightModifier = sin(v2_get_angle(stripe->normal));

    SDL_Texture *texture = stripe->texture;
    v2 textureSize = get_texture_size(texture);

    SDL_Rect srcRect = {(int)loop_clamp(stripe->collIdx * stripe->wallWidth, 0, textureSize.x), 0, 1, textureSize.y};

    SDL_Rect dstRect = {
        stripe->i * WINDOW_WIDTH / RESOLUTION_X + cameraOffset.x, 
        WINDOW_HEIGHT / 2 - stripe->size / 2 - player->pitch + cameraOffset.y,
        WINDOW_WIDTH / RESOLUTION_X + 1,
        stripe->size
    };
    

    BakedLightColor baked_light_color = get_light_color_by_pos(v2_add(stripe->pos, v2_mul(stripe->normal, to_vec(0.5))));

    // baked lights
    
    int rgb[3] = {
        125 * baked_light_color.r * brightness,
        125 * baked_light_color.g * brightness,
        125 * baked_light_color.b * brightness
    };
    
    clampColors(rgb);


    SDL_SetTextureColorMod(texture, rgb[0], rgb[1], rgb[2]);
    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
    free(stripe);
}

void renderParticles(Particles *particles) {
    for (int i = 0; i < particles->particleAmount; i++) {
        renderEntity(particles->particles[i]->entity);
    }
}

BakedLightColor get_light_color_by_pos(v2 pos) {
    double py = pos.y / tileSize;
    double px = pos.x / tileSize;
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

    SDL_Rect leftHandRect = {player->handOffset.x + cameraOffset.x, player->handOffset.y + cameraOffset.y, WINDOW_WIDTH, WINDOW_HEIGHT};

    BakedLightColor baked_light_color;

    Animation current_anim = leftHandSprite->animations[leftHandSprite->currentAnimationIdx];
    int current_anim_idx = leftHandSprite->currentAnimationIdx;

    bool first_check = current_anim_idx == 0;
    bool second_check = current_anim_idx == 1 && current_anim.frame > 1;
    if (first_check || second_check) {
        baked_light_color = get_light_color_by_pos(player->pos);
        baked_light_color.r = 0.3 + baked_light_color.r * 0.7;
        baked_light_color.g = 0.3 + baked_light_color.g * 0.7;
        baked_light_color.b = 0.3 + baked_light_color.b * 0.7;
    } else {
        baked_light_color.r = 2;
        baked_light_color.g = 1.8;
        baked_light_color.b = 1.5;
    }


    SDL_Texture *texture = getSpriteCurrentTexture(leftHandSprite);
    

    if (texture != NULL) {
        int rgb[3] = {
            baked_light_color.r * 125,
            baked_light_color.g * 125,
            baked_light_color.b * 125
        };

        clampColors(rgb);

        SDL_SetTextureColorMod(texture, rgb[0], rgb[1], rgb[2]);
        SDL_RenderCopy(renderer, texture, NULL, &leftHandRect);
    }
}

void render_health_bar() {
    
    v2 tex_size = get_texture_size(healthbar_texture);

    v2 scale = {3, 3};

    SDL_Rect outline_rect = {
        0,
        0,
        tex_size.x * scale.x,
        tex_size.y * scale.y 
    };

    SDL_Rect health_rect = {
        16 * scale.x,
        18 * scale.y,
        78 * scale.x * ((double)player->health / player->maxHealth),
        11 * scale.y
    };

    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(renderer, &health_rect);

    SDL_RenderCopy(renderer, healthbar_texture, NULL, &outline_rect);
}

void renderHUD() {

    SDL_SetTextureColorMod(vignette_texture, vignette_color.r, vignette_color.g, vignette_color.b);
    SDL_RenderCopy(renderer, vignette_texture, NULL, NULL);

    render_hand();

    render_health_bar();

    SDL_Rect crosshairRect = {WINDOW_WIDTH / 2 - 8, WINDOW_HEIGHT / 2 - 8, 16, 16};

    SDL_RenderCopy(renderer, crosshair, NULL, &crosshairRect);

    int shots = max(player->pendingShots, (int)(player->shootChargeTimer * 3));

    SDL_Rect playerPendingShotsRect = {WINDOW_WIDTH / 2 + -10 * shots, WINDOW_HEIGHT * 0.8, 20 * shots, WINDOW_HEIGHT * 0.05};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &playerPendingShotsRect);

}

void drawSkybox() {
    SDL_Texture *tex = skybox_texture;

    double x = loop_clamp(player->angle / startFov * WINDOW_WIDTH, 0, WINDOW_WIDTH);

    double yOffsets = -player->pitch;

    SDL_Rect skybox_rect = {-x, yOffsets, WINDOW_WIDTH * 2, WINDOW_HEIGHT / 2};

    SDL_RenderCopy(renderer, tex, NULL, &skybox_rect);
}

void render(double delta) {  // #RENDER

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    char *newTitle = "FPS: ";
    char *fps = malloc(4);
    decimal_to_text(realFps, fps);

    SDL_SetWindowTitle(window, concat(newTitle, fps));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    arraylist *renderList = getRenderList();

    calcFloorAndCeiling();
    drawFloorAndCeiling();

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
            case (int)PARTICLES:
                renderParticles((Particles *)rObj->val);
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
                EnemyBullet *bullet = rObj->val;
                if (bullet->dirSprite != NULL) {
                    renderDirSprite(bullet->dirSprite, bullet->entity.pos, bullet->entity.size, bullet->entity.height);
                } else {
                    renderEntity(bullet->entity);
                }
                break;
        }
    }

    if (render_debug) renderDebug();

    renderHUD();

    SDL_RenderPresent(renderer);
}

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
}

// CLEAR
RayCollisionData ray_object(Raycast ray, obj *object) {
    switch (object->type) {
        case (int)ENEMY:;
            Enemy *enemy = object->val;
            RayCollisionData enemyRayData = ray_circle(ray, *enemy->collider);
            if (enemyRayData.hit) {
                enemyRayData.collider = enemy;
                enemyRayData.colliderType = ENEMY;
            }

            return enemyRayData;
            break;
        case (int)ENEMY_SHOOTER:;
            Enemy *shooter = object->val;
            RayCollisionData shooterRayData = ray_circle(ray, *shooter->collider);
            if (shooterRayData.hit) {
                shooterRayData.collider = (ShooterEnemy *)object->val;
                shooterRayData.colliderType = ENEMY_SHOOTER;
            }

            return shooterRayData;
            break;
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
    double maxDist = 20;
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
        case (int)PARTICLES:;
            Particles *p = (Particles *)val;
            for (int i = 0; i < p->particleAmount; i++) {
                free(p->particles[i]);
            }
            free(p->particles);
            free(p);
            break;
        case (int)EFFECT:;
            Effect *effect = (Effect *)val;
            freeObject(effect->entity.sprite, SPRITE);
            free(effect);
            break;
        case (int)SPRITE:;  // not gonna destroy the texture
            Sprite *s = val;
            // for (int i = 0; i < s->animCount; i++) {
            //     Animation anim = s->animations[i];
            //     free(anim.frames);
            // }
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
            EnemyBullet *bullet = val;
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
        case (int)PARTICLES:;
            Particles *particles = (Particles *)val;
            for (int i = 0; i < particles->particleAmount; i++) {
                particles->particles[i]->vel = particles->startVel;
                particles->particles[i]->lifeTimer = particles->particleLifetime;

                particles->particles[i]->entity.pos = particles->pos;
                particles->particles[i]->entity.size = particles->particleSize;
                particles->particles[i]->entity.sprite = particles->particleSprite;
                particles->particles[i]->entity.height = particles->height;
            }
            arraylist_add(gameobjects, particles, PARTICLES);
            break;
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
                test_point->strength = 3;
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
                test_point->color = (SDL_Color){100, 200, 255};//{255, 200, 100};
                test_point->strength = 3;
                test_point->radius = 400;
                test_point->pos = (v2){(c + 0.5) * tileSize, (r + 0.5) * tileSize};
                add_game_object(test_point, LIGHT_POINT);
            }
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            int type = data[idx++];

            v2 tileMid = v2_add(v2_mul((v2){c, r}, to_vec(tileSize)), to_vec(tileSize / 2));

            switch (type) {
                case (int)P_PLAYER:
                    init_player(tileMid);
                    arraylist_add(gameobjects, player, PLAYER);
                    break;
                case (int)P_SHOOTER:;
                    ShooterEnemy *shooter = enemy_shooter_create(tileMid);
                    arraylist_add(gameobjects, shooter, ENEMY_SHOOTER);
                    break;
            }
        }
    }

    free(data);

    fclose(fh);


    bake_lights();

}

void freeAnimation(Animation *anim) {
    free(anim->frames);
    free(anim);
}

Animation create_animation(int frameCount, int priority, SDL_Texture **frames) {
    Animation anim;
    anim.playing = false;
    anim.frameCount = frameCount;
    if (frames == NULL) {
        anim.frames = malloc(sizeof(SDL_Texture *) * frameCount);
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

SDL_Texture *getSpriteCurrentTexture(Sprite *sprite) {
    if (!(sprite->isAnimated)) {
        return sprite->texture;
    } else {
        if (sprite->currentAnimationIdx == -1) return NULL;
        Animation current = sprite->animations[sprite->currentAnimationIdx];
        return current.frames[current.frame];
    }
}

Particles *createParticles(int amount, Sprite *sprite) {
    Particles *particles = malloc(sizeof(Particles));
    particles->pos = to_vec(0);
    particles->particleSprite = sprite;
    particles->particleAmount = amount;
    particles->particleLifetime = 1;
    particles->particles = malloc(sizeof(Particle *) * amount);
    for (int i = 0; i < amount; i++) {
        particles->particles[i] = malloc(sizeof(Particle));
    }

    return particles;
}

void particlesTick(Particles *particles, u64 delta) {
    double deltaSec = mili_to_sec(delta);

    for (int i = 0; i < particles->particleAmount; i++) {
        Particle *particle = particles->particles[i];
        particle->vel = v2_add(particle->vel, particles->gravity);

        v2 finalVel = v2_mul(particle->vel, to_vec(deltaSec));
        particle->entity.pos = v2_add(particle->entity.pos, finalVel);

        particle->lifeTimer -= deltaSec;
        if (particle->lifeTimer <= 0) {
            particle->lifeTimer = particles->particleLifetime;
            particle->vel = particles->startVel;
            particle->entity.pos = particles->pos;
        }
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
void playerShoot() {

    play_sound(player_default_shoot, 0.3);
    shakeCamera(10, 4, true, 1);

    player->canShoot = false;
    spritePlayAnim(leftHandSprite, 1);

    if (player->pendingShots > 0) {
        player->vel = v2_mul(get_player_forward(), to_vec(-1));
    }

    RayCollisionData ray_data = castRayForAll(player->pos, playerForward);

    double effect_height = get_max_height() / 2;

    if (ray_data.hit) {
        if (is_enemy_type(ray_data.colliderType)) {
            
            Entity entity = *((Entity *)ray_data.collider);
            effect_height = entity.height - entity.size.y / 2;
            
            enemy_pause(ray_data.collider, 0.4);
            enemyTakeDmg(ray_data.collider, 1);
            
            
        }
    } else {
        return;
    }

    v2 effectPos = v2_add(ray_data.collpos, v2_mul(playerForward, to_vec(-4)));

    Effect *hitEffect = createEffect(effectPos, to_vec(35), createSprite(true, 1), 1);

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
    }

    

    if (enemy->dirSprite != NULL) enemy->dirSprite->dir = enemy->dir;


    

    dSpriteTick(enemy->dirSprite, enemy->entity.pos, delta);
}

void enemyTakeDmg(Enemy *enemy, int dmg) {
    enemy->health -= dmg;
    shakeCamera(15, 4, true, 0);
    Sprite *sprite = createSprite(false, 0);
    sprite->texture = enemy->hit_texture;
    Effect *hit_effect = createEffect(v2_add(enemy->entity.pos, v2_dir(enemy->entity.pos, player->pos)), to_vec(50), sprite, 0.1);
    hit_effect->entity.height = enemy->entity.height;
    hit_effect->entity.affected_by_light = false;
    add_game_object(hit_effect, EFFECT);
    
    play_spatial_sound(enemy_default_hit, 1, player->pos, enemy->entity.pos, enemy->sound_max_radius);

    if (enemy->health <= 0) {
        // play death anim
        play_spatial_sound(enemy_default_kill, 1, player->pos, enemy->entity.pos, enemy->sound_max_radius);
        remove_game_object(enemy, ENEMY);
    }
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

    return dSprite;
}

Sprite *getDSpriteCurrentSprite(DirectionalSprite *dSprite, v2 spritePos) {
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
    spriteTick(getDSpriteCurrentSprite(dSprite, spritePos), delta);
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

EnemyBullet *createDefaultBullet(v2 pos, v2 dir) {
    EnemyBullet *bullet = malloc(sizeof(EnemyBullet));

    bullet->entity.pos = pos;
    bullet->entity.size = to_vec(20);
    bullet->entity.sprite = createSprite(false, 0);
    bullet->entity.sprite->texture = enemy_bullet_texture;  // CHANGE LATER
    bullet->entity.height = WINDOW_HEIGHT / 6;
    bullet->entity.affected_by_light = false;
    bullet->dirSprite = NULL;
    bullet->dmg = 1;
    bullet->speed = 3.5;
    bullet->dir = dir;
    bullet->lifeTime = 5;
    bullet->lifeTimer = bullet->lifeTime;

    bullet->collider = malloc(sizeof(CircleCollider));
    bullet->collider->pos = bullet->entity.pos;
    bullet->collider->radius = 5;

    return bullet;
}

EnemyBullet *createTestBullet(v2 pos) { return createDefaultBullet(pos, (v2){1, 0}); }

bool intersectCircles(CircleCollider c1, CircleCollider c2) {
    return v2_distance_squared(c1.pos, c2.pos) < (c1.radius + c2.radius) * (c1.radius + c2.radius);  // dist^2 < (r1 + r2)^2
}

void bulletTick(EnemyBullet *bullet, double delta) {

    bullet->entity.pos = v2_add(bullet->entity.pos, v2_mul(bullet->dir, to_vec(bullet->speed)));
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

    if (intersectCircles(*bullet->collider, *(player->collider))) {
        enemy_bullet_destroy(bullet);
        player_take_dmg(1);
        return;
    }
}

void enemy_shooter_shoot(ShooterEnemy *shooter) {
    EnemyBullet *bullet = createDefaultBullet(shooter->enemy.entity.pos, shooter->enemy.dir);
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



void shooterTick(ShooterEnemy *shooter, double delta) { 

    const int MODE_RESTING = 0;
    const int MODE_SHOOTING = 1;

    const int PREFFERED_DISTANCE = 200;

    enemy_default_handle_state(shooter, delta);

    


    if (shooter->enemy.state == STATE_IDLE) {
        enemy_idle_movement(shooter, delta);
    } else {
        shooter->mode_timer -= delta;
        if (shooter->mode_timer <= 0) {
            shooter->mode = shooter->mode == MODE_RESTING? MODE_SHOOTING : MODE_RESTING; // could make this simpler(1 - mode) but it will be less readable
            if (shooter->mode == MODE_RESTING) {
                shooter->mode_timer = 4;
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
            shooter->enemy.dir = shooter->enemy.dir_to_player;
        } else {
            if (shooter->enemy.collided_last_frame || v2_equal(shooter->attacking_move_dir, V2_ZERO)) {
                v2 random_dir = v2_get_random_dir();
                shooter->attacking_move_dir = random_dir;
            }
            v2 final = v2_rotate(shooter->attacking_move_dir, v2_get_angle(shooter->enemy.dir_to_player));
            shooter->enemy.move_dir = v2_lerp(shooter->enemy.move_dir, final, delta);
            shooter->enemy.speed_multiplier = 1.2;
            shooter->enemy.dir = shooter->enemy.dir_to_player;

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
    shooter->enemy = createEnemy(pos);
    shooter->enemy.maxHealth = 5;
    shooter->enemy.health = shooter->enemy.maxHealth;
    shooter->enemy.hit_texture = shooter_hit_texture;

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
            EnemyBullet *b = val;
            
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

// Takes a file name with no extension and assumes it's a bmp
void getTextureFiles(char *fileName, int fileCount, SDL_Texture ***textures) {
    int charCount = get_num_digits(fileCount);

    for (int i = 0; i < fileCount; i++) {
        char num[charCount + 10];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(fileName, num);
        char *fileWithExtension = concat(fileWithNum, ".bmp");

        SDL_Texture *tex = make_texture(renderer, fileWithExtension);
        (*textures)[i] = tex;

        free(fileWithNum);
        free(fileWithExtension);
    }
}

void update_fullscreen() {
    if (fullscreen) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window, 0);
    }
}

void bake_lights() {
   
    init_loading_screen();


    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {
            baked_light_grid[r][c] = (BakedLightColor){ambient_light, ambient_light, ambient_light};
        }
    }

    int light_count = 0;

    for (int i = 0; i < gameobjects->length; i++) {
        obj *current = arraylist_get(gameobjects, i);
		if (current->type != LIGHT_POINT) continue;
        light_count++;
    }

    if (light_count == 0) {
        return;
    }


    int current_count = 0;

    for (int i = 0; i < gameobjects->length; i++) {
        obj *current = arraylist_get(gameobjects, i);
		if (current->type != LIGHT_POINT) continue;
        LightPoint *point = current->val;

        v2 grid_pos = v2_mul(v2_div(point->pos, to_vec(tileSize)), to_vec(BAKED_LIGHT_RESOLUTION));
        double scaled_radius = point->radius / tileSize * BAKED_LIGHT_RESOLUTION;

        int row_lbound = max(grid_pos.y - scaled_radius, 0);
        int row_rbound = min(grid_pos.y + scaled_radius, TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION);
        int col_lbound = max(grid_pos.x - scaled_radius, 0);
        int col_rbound = min(grid_pos.x + scaled_radius, TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION);

        for (int r = row_lbound; r < row_rbound; r++) {
            for (int c = col_lbound; c < col_rbound; c++) {

                v2 current_pos = v2_mul(v2_div((v2){c, r}, to_vec(BAKED_LIGHT_RESOLUTION)), to_vec(tileSize));

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

        current_count++;

        double loading_progress = (double)current_count / light_count;
        update_loading_progress(loading_progress);
    }

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

    if (isValidLevel(levelToLoad)) {
        load_level(levelToLoad);
    } else {
        load_level("Levels/new_default_level.hclevel");
    }
}

void player_take_dmg(int dmg) {
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

    reset_level();
}


void init_loading_screen() {
    
    is_loading = true;
    loading_progress = 0;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const v2 bar_container_size = {
        200,
        50
    };

    SDL_Rect bar_container = {
        WINDOW_WIDTH / 2 - bar_container_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_container_size.y / 2,
        bar_container_size.x,
        bar_container_size.y
    };

    const v2 bar_size = {
        190,
        40
    };

    SDL_Rect bar_background = {
        WINDOW_WIDTH / 2 - bar_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_size.y / 2,
        bar_size.x,
        bar_size.y
    };

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bar_container);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bar_background);

    SDL_RenderPresent(renderer);

}

void update_loading_progress(double progress) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const v2 bar_container_size = {
        200,
        50
    };

    SDL_Rect bar_container = {
        WINDOW_WIDTH / 2 - bar_container_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_container_size.y / 2,
        bar_container_size.x,
        bar_container_size.y
    };

    v2 bar_bg_size = {
        190,
        40
    };

    SDL_Rect bar_background = {
        WINDOW_WIDTH / 2 - bar_bg_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_bg_size.y / 2,
        bar_bg_size.x,
        bar_bg_size.y
    };

    v2 bar_size = v2_sub(bar_bg_size, to_vec(10));

    SDL_Rect bar = {
        WINDOW_WIDTH / 2 - bar_size.x / 2,
        WINDOW_HEIGHT / 2 - bar_size.y / 2,
        bar_size.x * progress,
        bar_size.y
    };
    bar.w = bar_size.x * progress;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bar_container);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bar_background);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bar);

    SDL_RenderPresent(renderer);
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

void enemy_bullet_destroy(EnemyBullet *bullet) {
   
   
    Sprite *sprite = createSprite(true, 1);
    sprite->animations[0] = create_animation(5, 0, enemy_bullet_destroy_anim);
    sprite->animations[0].playing = true;
    sprite->animations[0].fps = 10;
    sprite->animations[0].loop = false;

    v2 pos = v2_sub(bullet->entity.pos, v2_mul(bullet->dir, to_vec(5)));

    Effect *effect = createEffect(pos, to_vec(50), sprite, 1);
    effect->entity.height = bullet->entity.height;
    effect->entity.affected_by_light = false;

    add_game_object(effect, EFFECT);




    remove_game_object(bullet, ENEMY_SHOOTER); 
}

int charge_time_to_shots(double charge_time) {
    return (int)(charge_time * 3);
}

void enemy_move(Enemy *enemy, double delta) {

    v2 vel = v2_mul(enemy->move_dir, to_vec(enemy->speed * enemy->speed_multiplier));
    vel = v2_mul(vel, to_vec(delta));

    enemy->entity.pos = v2_add(enemy->entity.pos, vel);
    
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
    if (enemy->wander_pause_timer <= 0) {
        v2 random_dir = v2_rotate((v2){1, 0}, randf_range(0, 360));
        RayCollisionData ray = castRay(enemy->entity.pos, random_dir);
        double max_travel_dist = 200;
        if (ray.hit) max_travel_dist = min(max_travel_dist, v2_distance(ray.startpos, ray.collpos));

        double travel_dist = randf_range(max_travel_dist / 2, max_travel_dist);
        enemy->current_wander_pos = v2_add(enemy->home_pos, v2_mul(random_dir, to_vec(travel_dist)));

        enemy->wander_pause_timer = 10;
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

    Effect *effect = createEffect(enemy->entity.pos, to_vec(400), sprite, 0.51);
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
    exploder->enemy = createEnemy(pos);
    
    return exploder;
}

bool is_entity_type(int type) {
    return in_range(type, ENTITY_START, ENTITY_END);
}

void exploder_tick(ExploderEnemy *exploder, double delta) {
    
}


// #END