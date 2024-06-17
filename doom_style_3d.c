
#include "game_utils.c"
#include "globals.h"

SDL_Renderer *renderer;
SDL_Window *window;

// #DEFINITIONS

#define TPS 300
#define FPS 300
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 580
#define RESOLUTION_X 360
#define RESOLUTION_Y 180
#define X_SENSITIVITY 0.1
#define Y_SENSITIVITY 0.5
#define COLOR_BLACK \
    (SDL_Color) { 0, 0, 0 }
#define TRANSPARENT \
    (SDL_Color) { 0, 0, 0, 0 }
#define RENDER_DISTANCE 350
#define WALL_HEIGHT 36
#define NUM_WALL_THREADS 4

#define BAKED_LIGHT_RESOLUTION 36

#define OUT_OF_SCREEN_POS \
    (v2) { WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100 }

enum Types { PLAYER, RAYCAST, CIRCLE_COLLIDER, RAY_COLL_DATA, ENTITY, RENDER_OBJECT, WALL_STRIPE, LIGHT_POINT, SPRITE, PARTICLES, PARTICLE, EFFECT, ENEMY, BULLET, DIR_SPRITE, ENEMY_SHOOTER };

enum Tiles { WALL1 = 1, WALL2 = 2 };

// #STRUCTS

typedef struct Animation {
    SDL_Texture **frames;
    int frameCount;
    int frame;
    double fps;
    double timeToNextFrame;
    bool loop;
    bool playing;
} Animation;

typedef struct Sprite {
    SDL_Texture *texture;  // not used if animated
    bool isAnimated;
    int currentAnimationIdx;
    Animation **animations;
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
    double collIdx, wallWidth;
} RayCollisionData;

typedef struct CircleCollider {
    v2 pos;
    double radius;
} CircleCollider;

typedef struct Player {
    v2 pos, vel;
    double speed, angle, torque, collSize;
    double height;
    bool sprinting;
    bool canShoot;
    double shootCooldown;
    double shootChargeTimer;
    int pendingShots;
    double ShootTickTimer;
    v2 handOffset;
    int health, maxHealth;
    CircleCollider *collider;
} Player;

typedef struct Entity {
    v2 pos, size;
    Sprite *sprite;
    double height;
} Entity;

typedef struct LightPoint {
    v2 pos;
    double strength;
    double radius;
    SDL_Color color;
} LightPoint;

typedef struct Particle {
    Entity *entity;
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
    Entity *entity;
    double lifeTime;
} Effect;

typedef struct RenderObject {
    void *val;
    int type;
    double dist_squared;
} RenderObject;

typedef struct Enemy {
    Entity *entity;
    DirectionalSprite *dirSprite;
    CircleCollider *collider;
    v2 dir;
    bool seeingPlayer;
    v2 lastSeenPlayerPos;
    double maxHealth, health;

} Enemy;

typedef struct EnemyBullet {
    Entity *entity;
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
    Enemy *enemy;
    double shootCooldown;
    double shootCooldownTimer;
} ShooterEnemy;

// #ENEMIES END

typedef struct CollisionData {
    v2 offset;  // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;

// #FUNCTIONS

BakedLightColor get_light_color_by_pos(v2 pos);

void bake_lights();

void update_fullscreen();

void drawSkybox();

void update_entity_collisions(void *val, int type);

void init_tilemap(int ***gridPtr, int cols, int rows);

void reset_tilemap(int ***gridPtr, int cols, int rows);

void load_level(char *file);

void init();

void render(u64 delta);

RayCollisionData *ray_circle(Raycast ray, CircleCollider circle);

RayCollisionData *ray_object(Raycast ray, obj *object);

RayCollisionData *castRayForEntities(v2 pos, v2 dir);

RayCollisionData *castRay(v2 pos, v2 dir);

CollisionData getCircleTileCollision(CircleCollider circle, v2 tilePos);

CollisionData getCircleTileMapCollision(CircleCollider circle);

void key_pressed(SDL_Keycode key);

void key_released(SDL_Keycode key);

void tick(u64 delta);

void handle_input(SDL_Event event);

void add_game_object(void *val, int type);

void remove_game_object(void *val, int type);

Player *init_player(v2 pos);

double mili_to_sec(u64 mili);

v2 get_player_forward();

Animation *create_animation(int frameCount);

void freeAnimation(Animation *anim);

SDL_Texture *getSpriteCurrentTexture(Sprite *sprite);

Sprite *getDSpriteCurrentSprite(DirectionalSprite *dSprite, v2 spritePos);

void objectTick(void *obj, int type, u64 delta);

void playerTick(u64 delta);

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, u64 delta);

void spriteTick(Sprite *sprite, u64 delta);

void enemyTick(Enemy *enemy, u64 delta);

void animationTick(Animation *anim, u64 delta);

void effectTick(Effect *effect, u64 delta);

void bulletTick(EnemyBullet *bullet, u64 delta);

void shooterTick(ShooterEnemy *shooter, u64 delta);

Sprite *createSprite(bool isAnimated, int animCount);

DirectionalSprite *createDirSprite(int dirCount);

void spritePlayAnim(Sprite *sprite, int idx);

Sprite *getRandomWallSprite();

Particles *createParticles(int amount, Sprite *sprite);

void particlesTick(Particles *particles, u64 delta);

void playerShoot();

void enemyTakeDmg(void *enemy, int type, int dmg);

void renderTexture(SDL_Texture *texture, v2 pos, v2 size, double height);

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height);

double angleDist(double a1, double a2);

void shakeCamera(double strength, int ticks, bool fade);

ShooterEnemy *createShooterEnemy(v2 pos);

RayCollisionData *castRayForAll(v2 pos, v2 dir);

bool isValidLevel(char *file);

void getTextureFiles(char *fileName, int fileCount, SDL_Texture ***textures);

// #FUNCTIONS END

// #VARIABLES

BakedLightColor baked_light_grid[TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION][TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION];

bool fullscreen = false;
bool running = true;
arraylist *gameobjects;
Player *player;
bool keyPressArr[26];
bool render_debug = false;
bool lockMouse = true;
bool renderLight = false;
bool isLMouseDown = false;

double startFov = 90;
double fov = 90;
const char *font = "font.ttf";
const SDL_Color fogColor = {0, 0, 0, 255};

TextureData *floorTexture;
TextureData *floorTexture2;
TextureData *ceilingTexture;

SDL_Texture *floorAndCeiling;
SDL_Texture *wallTexture;
SDL_Texture *entityTexture;
SDL_Texture *crosshair;
SDL_Texture *fenceTexture;

double tanHalfFOV;
double tanHalfStartFOV;

int floorRenderStart;

int **levelTileMap;
int **floorTileMap;
int **ceilingTileMap;

const int tileSize = WINDOW_WIDTH / TILEMAP_WIDTH;

double realFps;

SDL_Texture **wallFrames;
SDL_Texture **shootHitEffectFrames;
SDL_Texture *skybox_texture;

bool isCameraShaking = false;
int cameraShakeTicks;
int cameraShakeTicksLeft = 0;
double cameraShakeTimeToNextTick = 0.02;
double cameraShakeCurrentStrength = 0;
bool cameraShakeFadeActive = false;

RenderObject *wallStripesToRender[RESOLUTION_X];

v2 cameraOffset = {0, 0};

double wallHeights[RESOLUTION_X];  // for floor and ceiling optimization

Sprite *animatedWallSprite;

Sprite *leftHandSprite;

v2 playerForward;

char *levelToLoad = NULL;

const double PLAYER_SHOOT_COOLDOWN = 0.5;

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
            tick(tick_timer);
            render(tick_timer);
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

Enemy *createEnemy(v2 pos) {
    Enemy *enemy = malloc(sizeof(Enemy));
    enemy->maxHealth = 5;
    enemy->health = enemy->maxHealth;
    enemy->dir = (v2){1, 0};

    enemy->entity = malloc(sizeof(Entity));
    enemy->entity->pos = pos;
    enemy->entity->size = to_vec(50);
    enemy->entity->sprite = NULL;

    enemy->dirSprite = createDirSprite(16);
    for (int i = 0; i < 16; i++) {
        char *baseFileName = "Textures/CubeEnemyAnim/CubeEnemy";
        char num[get_num_digits(i + 1)];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(baseFileName, num);
        char *fileWithExtension = concat(fileWithNum, ".bmp");

        enemy->dirSprite->sprites[i] = createSprite(false, 0);
        enemy->dirSprite->sprites[i]->texture = make_texture(renderer, fileWithExtension);
    }
    enemy->dirSprite->dir = (v2){1, 0};

    enemy->entity->height = WINDOW_HEIGHT / 6;

    enemy->collider = malloc(sizeof(CircleCollider));
    enemy->collider->radius = 10;
    enemy->collider->pos = enemy->entity->pos;

    enemy->seeingPlayer = false;
    enemy->lastSeenPlayerPos = (v2){0, 0};

    return enemy;
}

void init() {  // #INIT

    init_cd_print();

    fenceTexture = make_texture(renderer, "Textures/fence.bmp");
    SDL_SetTextureBlendMode(fenceTexture, SDL_BLENDMODE_BLEND);

    floorAndCeiling = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, RESOLUTION_X, RESOLUTION_Y);
    SDL_SetTextureBlendMode(floorAndCeiling, SDL_BLENDMODE_BLEND);

    floorTexture = TextureData_from_bmp("Textures/floor.bmp");
    floorTexture2 = TextureData_from_bmp("Textures/floor2.bmp");
    ceilingTexture = TextureData_from_bmp("Textures/ceiling.bmp");

    player = init_player((v2){0, 0});

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
    animatedWallSprite->animations[0] = create_animation(17);
    animatedWallSprite->animations[0]->frames = wallFrames;
    animatedWallSprite->animations[0]->fps = 10;
    spritePlayAnim(animatedWallSprite, 0);

    leftHandSprite = createSprite(true, 2);
    leftHandSprite->animations[0] = create_animation(1);
    leftHandSprite->animations[0]->frames[0] = make_texture(renderer, "Textures/rightHandAnim/rightHandAnim6.bmp");
    leftHandSprite->animations[1] = create_animation(6);
    leftHandSprite->animations[1]->frames = malloc(sizeof(SDL_Texture *) * 6);
    getTextureFiles("Textures/rightHandAnim/rightHandAnim", 6, &leftHandSprite->animations[1]->frames);
    leftHandSprite->animations[1]->fps = 12;

    leftHandSprite->animations[1]->loop = false;
    spritePlayAnim(leftHandSprite, 0);

    shootHitEffectFrames = malloc(sizeof(SDL_Texture *) * 5);

    getTextureFiles("Textures/ShootEffectAnim/shootHitEffect", 5, &shootHitEffectFrames);

    // int x, y;
    // SDL_QueryTexture(floorTexture, NULL, NULL, &x, &y);
    // floorTextureSize = (v2){x, y};

    for (int i = 0; i < 26; i++) keyPressArr[i] = false;

    entityTexture = make_texture(renderer, "Textures/scary_monster.bmp");

    if (isValidLevel(levelToLoad)) {
        load_level(levelToLoad);

    } else {
        load_level("default_level.hclevel");
    }

    skybox_texture = make_texture(renderer, "Textures/skybox.bmp");

    SDL_RenderSetLogicalSize(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

	LightPoint *test_point = malloc(sizeof(LightPoint));
	test_point->color = (SDL_Color){255, 200, 100};
	test_point->strength = 2;
    test_point->radius = 400;
	test_point->pos = player->pos;

    LightPoint *test_point2 = malloc(sizeof(LightPoint));
    test_point2->color = (SDL_Color){120, 180, 255};
    test_point2->strength = 2;
    test_point2->radius = 400;
	test_point2->pos = v2_add(player->pos, (v2){0, -100});

	add_game_object(test_point, LIGHT_POINT);
    add_game_object(test_point2, LIGHT_POINT);

	bake_lights();

}  // #INIT END

v2 get_player_forward() { return v2_rotate_to((v2){1, 0}, deg_to_rad(player->angle)); }

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
            // player->height += event.motion.yrel * Y_SENSITIVITY;
            // player->height = clamp(player->height, -30, 30);
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
                int numShots = (int)(player->shootChargeTimer * 3);
                player->shootChargeTimer = 0;
                player->pendingShots = numShots;
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
    if (key == SDLK_LSHIFT) {
        player->sprinting = true;
    }
    if (key == SDLK_l) {
        renderLight = !renderLight;
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

bool is_key_pressed(SDL_Keycode key) { return keyPressArr[(char)key - 'a']; }

int get_key_axis(SDL_Keycode key1, SDL_Keycode key2) { return (int)is_key_pressed(key2) - (int)is_key_pressed(key1); }

v2 get_key_vector(SDL_Keycode k1, SDL_Keycode k2, SDL_Keycode k3, SDL_Keycode k4) { return (v2){get_key_axis(k1, k2), get_key_axis(k3, k4)}; }

void checkCollisions() {
    // new plan: Circle AABB collision.
    // step 1: get tiles I want to intersect with
    // step 2: For collision with a tile, get the closest point from the tile to
    // the circle. If the distance to that point is less than or equal to the
    // radius of the circle then push the circle back by the overlap, don't change
    // the velocity.

    v2 velDir = player->vel;

    v2 dirs[] = {V2_ZERO};

    RayCollisionData *collData[1];
    int dataLen = 0;

    v2 normal;

    for (int i = 0; i < 1; i++) {
        RayCollisionData *result = castRay(v2_add(player->pos, v2_mul(dirs[i], to_vec(player->collSize))), velDir);
        if (result != NULL && v2_distance_squared(player->pos, result->collpos) < player->collSize * player->collSize) {
            collData[dataLen++] = result;
        }
    }

    if (dataLen > 0) {
        v2 normals[dataLen];
        for (int i = 0; i < dataLen; i++) normals[i] = collData[i]->normal;
        v2 final = v2_mode(normals, dataLen);
        player->vel = v2_slide(player->vel, final);
    }
}

void playerTick(u64 delta) {
    double deltaSec = mili_to_sec(delta);

    player->collider->pos = player->pos;

    if (player->pendingShots > 0) {
        player->ShootTickTimer -= deltaSec;

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
        player->height = t;
        player->handOffset.y = t * 2.5;
    } else {
        player->height = lerp(player->height, 0, 0.1);
        player->handOffset.y = lerp(player->handOffset.y, 0, 0.1);
    }

    player->torque = get_key_axis(SDLK_n, SDLK_m);

    player->vel = v2_lerp(player->vel, v2_add(v2_mul(move_dir, to_vec(keyVec.x)), v2_mul(move_dir_rotated, to_vec(keyVec.y))), 0.15);

    double moveSpeed = player->speed;
    if (player->sprinting) {
        moveSpeed *= 1.5;
        fov = lerp(fov, startFov * 1.2, 0.1);
    } else {
        fov = lerp(fov, startFov, 0.1);
    }

    v2 finalVel = v2_mul(player->vel, to_vec(moveSpeed));
    if (delta == 0) {
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(0.016)));
    } else {
        player->pos = v2_add(player->pos, v2_mul(finalVel, to_vec(deltaSec)));
    }

    if (!player->canShoot) {
        player->shootCooldown -= deltaSec;
        if (player->shootCooldown <= 0) {
            player->canShoot = true;
            player->shootCooldown = PLAYER_SHOOT_COOLDOWN;
        }
    }
}

void spriteTick(Sprite *sprite, u64 delta) {
    if (sprite == NULL) return;
    if (!sprite->isAnimated) return;
    if (sprite->animations == NULL) return;
    if (sprite->currentAnimationIdx == -1) return;
    animationTick(sprite->animations[sprite->currentAnimationIdx], delta);
}

void objectTick(void *obj, int type, u64 delta) {
    switch (type) {
        case (int)PLAYER:
            playerTick(delta);
            break;
        case (int)EFFECT:
            effectTick(obj, delta);
            break;
        case (int)SPRITE:;
            Sprite *sprite = obj;
            animationTick(sprite->animations[sprite->currentAnimationIdx], delta);
            break;
        case (int)PARTICLES:
            particlesTick(obj, delta);
            break;
        case (int)ENEMY:
            enemyTick(obj, delta);
            break;
        case (int)BULLET:
            bulletTick(obj, delta);
            break;
        case (int)ENEMY_SHOOTER:
            shooterTick(obj, delta);
            break;
    }

    update_entity_collisions(obj, type);
}

void tick(u64 delta) {
    playerForward = get_player_forward();

    double deltaSec = mili_to_sec(delta);

    tanHalfFOV = tan(deg_to_rad(fov / 2));

    if (lockMouse) {
        SDL_WarpMouseInWindow(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
    }

    if (isLMouseDown) {
        player->shootChargeTimer += deltaSec;
        if (player->shootChargeTimer >= 0.7) {
            shakeCamera(80 * min(15, player->shootChargeTimer - 0.7), 5, false);
        }
    }

    if (isCameraShaking) {
        cameraShakeTimeToNextTick -= deltaSec;
        if (cameraShakeTimeToNextTick <= 0) {
            cameraShakeTicksLeft -= 1;
            cameraShakeTimeToNextTick = 0.02;
            if (cameraShakeTicksLeft <= 0) {
                isCameraShaking = false;
            } else {
                v2 rawShake = {randf_range(-cameraShakeCurrentStrength, cameraShakeCurrentStrength), randf_range(-cameraShakeCurrentStrength, cameraShakeCurrentStrength)};
                cameraOffset = v2_mul(rawShake, to_vec((double)cameraShakeTicksLeft / cameraShakeTicks));
            }
        }
    }
    cameraOffset = v2_lerp(cameraOffset, to_vec(0), 0.2);

    SDL_ShowCursor(!lockMouse);

    animationTick(animatedWallSprite->animations[animatedWallSprite->currentAnimationIdx], delta);
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

    for (int row = 0; row < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; row++) {
        for (int col = 0; col < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; col++) {
            BakedLightColor color = baked_light_grid[row][col];
            SDL_SetRenderDrawColor(renderer, clamp(125 * color.r, 0, 255), clamp(125 * color.g, 0, 255), clamp(125 * color.b, 0, 255), 255);
            double y = (double)row / BAKED_LIGHT_RESOLUTION * tileSize;
            double x = (double)col / BAKED_LIGHT_RESOLUTION * tileSize;
            v2 px_size = v2_div((v2){WINDOW_WIDTH, WINDOW_HEIGHT}, to_vec(tileSize * BAKED_LIGHT_RESOLUTION)); 
            SDL_Rect rect = {
                x,
                y,
                10,
                10
            };

            SDL_RenderFillRect(renderer, &rect);
        }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

v2 getRayDirByIdx(int i) {
    double x = tanHalfFOV;
    double idx = lerp(-x, x, ((double)(i + 1)) / RESOLUTION_X);

    v2 temp = (v2){1, idx};

    temp = v2_normalize(temp);
    v2 rayDir = v2_dir((v2){0, 0}, v2_rotate(temp, deg_to_rad(player->angle)));
    return rayDir;
}

v2 floorToScreen(v2 pos) {
    // know: player pos, distance to pos, angle to pos
    // need to find:

    v2 offset = v2_sub(pos, player->pos);
    double dist = v2_length(offset);

    double signedAngle = v2_signed_angle_between(playerForward, offset);

    double lowBound = fov * -0.5;
    double highBound = fov * 0.5;
    double signedAngleDeg = rad_to_deg(signedAngle);

    if (signedAngleDeg <= lowBound || signedAngleDeg >= highBound) {
        return OUT_OF_SCREEN_POS;
    }

    double idx = inverse_lerp(lowBound, highBound, signedAngleDeg);

    int x = idx * WINDOW_WIDTH;

    double wallSize = WALL_HEIGHT * WINDOW_HEIGHT / dist;

    int y = WINDOW_HEIGHT / 2 + wallSize / 2;

    return (v2){x, y};
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

    TextureData *floorTex = floorTexture;
    TextureData *ceilingTex = floorTexture;
    v2 textureSize = (v2){floorTex->w, floorTex->h};

    for (int row = RESOLUTION_Y / 2; row < RESOLUTION_Y; row++) {
        int screenY = row * WINDOW_HEIGHT / RESOLUTION_Y;

        v2 left = screenToFloor((v2){0, screenY});
        v2 right = screenToFloor((v2){RESOLUTION_X - 1, screenY});
        for (int col = 0; col < RESOLUTION_X; col++) {
            int screenX = col * WINDOW_WIDTH / RESOLUTION_X;

            v2 point = v2_lerp(left, right, (double)col / RESOLUTION_X);

            
            double light = inverse_lerp(RESOLUTION_Y / 2, RESOLUTION_Y, row);
            if (row < RESOLUTION_Y * 3/4) {
                light = inverse_lerp(RESOLUTION_Y / 2, RESOLUTION_Y * 3/4, row) * 0.8;
            } else {
                light = 0.8;
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

            if (floorTile == P_FLOOR) {
                floorTex = floorTexture2;
            } else {
                floorTex = floorTexture;
            }

            if (ceilingTile == P_CEILING) {
                ceilingTex = ceilingTexture;
            }

            int floor_row = row;
            int floor_col = col;

            floor_row = clamp(floor_row, 0, RESOLUTION_Y - 1);
            floor_col = clamp(floor_col, 0, WINDOW_WIDTH - 1);

            int floor_pixel_idx = floor_row * RESOLUTION_X + floor_col;

            Pixel floor_pixel = TextureData_get_pixel(floorTex, loop_clamp(point.x, 0, 36), loop_clamp(point.y, 0, 36));

            floor_pixel.r *= light;
            floor_pixel.g *= light;
            floor_pixel.b *= light;

			BakedLightColor baked_light_color = get_light_color_by_pos(point);

			// baked lights
			floor_pixel.r = clamp(floor_pixel.r * baked_light_color.r, 0, 255);
			floor_pixel.g = clamp(floor_pixel.g * baked_light_color.g, 0, 255);
			floor_pixel.b = clamp(floor_pixel.b * baked_light_color.b, 0, 255);
            

            int floor_pixel_i = floor_pixel.r << 24 | floor_pixel.g << 16 | floor_pixel.b << 8 | floor_pixel.a;

            ((int *)pixels)[floor_pixel_idx] = floor_pixel_i;

            int ceil_col = col;
            int ceil_row = RESOLUTION_Y / 2 - abs(RESOLUTION_Y / 2 - row);

            int ceil_pixel_idx = ceil_row * RESOLUTION_X + ceil_col;

            Pixel ceil_pixel;

            if (ceilingTile == -1) {
                ceil_pixel = (Pixel){0, 0, 0, 0};
            } else {
                ceil_pixel = TextureData_get_pixel(ceilingTex, loop_clamp(point.x, 0, 36), loop_clamp(point.y, 0, 36));
                ceil_pixel.r *= light;
                ceil_pixel.g *= light;
                ceil_pixel.b *= light;

                ceil_pixel.r = clamp(ceil_pixel.r * baked_light_color.r, 0, 255);
                ceil_pixel.g = clamp(ceil_pixel.g * baked_light_color.g, 0, 255);
                ceil_pixel.b = clamp(ceil_pixel.b * baked_light_color.b, 0, 255);
            }

            int ceil_pixel_i = ceil_pixel.r << 24 | ceil_pixel.g << 16 | ceil_pixel.b << 8 | ceil_pixel.a;

            ((int *)pixels)[ceil_pixel_idx] = ceil_pixel_i;
        }
    }

    SDL_UnlockTexture(floorAndCeiling);
}

void drawFloorAndCeiling() {
    drawSkybox();

    v2 offsets = (v2){cameraOffset.x, cameraOffset.y - player->height};

    SDL_Rect rect = {offsets.x, offsets.y, WINDOW_WIDTH, WINDOW_HEIGHT};

    SDL_RenderCopy(renderer, floorAndCeiling, NULL, &rect);
}

void renderTexture(SDL_Texture *texture, v2 pos, v2 size, double height) {
    double cosAngleToForward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    v2 screenFloorPos = floorToScreen(pos);
    if (v2_equal(screenFloorPos, OUT_OF_SCREEN_POS)) {
        return;
    } else {
        // printf("ftoscreen pos: (%.2f, %.2f), out of screen pos; (%.2f, %.2f) \n",
        // screenFloorPos.x, screenFloorPos.y, OUT_OF_SCREEN_POS.x,
        // OUT_OF_SCREEN_POS.y);
    }

    double dist = v2_distance(pos, player->pos);

    v2 textureSize = get_texture_size(texture);

    // WALL_HEIGHT * WINDOW_HEIGHT/dist * 80/fov;
    double fovFactor = tanHalfStartFOV / tanHalfFOV;
    double finalSize = WALL_HEIGHT * WINDOW_HEIGHT / dist * fovFactor / cosAngleToForward;
    v2 finalSizeVec = (v2){finalSize * size.x / 100, finalSize * size.y / 100};

    SDL_Rect dstRect = {screenFloorPos.x - finalSizeVec.x / 2 + cameraOffset.x, screenFloorPos.y - finalSizeVec.y / 2 - (height * 100 / dist) - player->height + cameraOffset.y, finalSizeVec.x,
                        finalSizeVec.y};

    double light = distance_to_color(dist, 0.005);

    int color = light * 255;

    SDL_SetTextureColorMod(texture, color, color, color);
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    SDL_SetTextureColorMod(texture, 255, 255, 255);
}

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height) {
    SDL_Texture *texture = getSpriteCurrentTexture(getDSpriteCurrentSprite(dSprite, pos));

    renderTexture(texture, pos, size, height);
}

void renderEntity(Entity *entity) {  // RENDER ENTITY
    SDL_Texture *texture = getSpriteCurrentTexture(entity->sprite);

    renderTexture(texture, entity->pos, entity->size, entity->height);
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
    v2 rayDir = getRayDirByIdx(i);

    RayCollisionData *data = castRay(player->pos, rayDir);

    if (data == NULL) {
        wallHeights[i] = 0;
        free(data);
        return NULL;
    }

    WallStripe *stripe = malloc(sizeof(WallStripe));
    stripe->i = i;
    stripe->texture = data->colliderTexture;

    double cosAngleToForward = v2_cos_angle_between(rayDir, playerForward);
    double collDist = v2_distance(data->startpos, data->collpos) * cosAngleToForward;
    double fovFactor = tanHalfStartFOV / tanHalfFOV;
    double finalSize = WALL_HEIGHT * WINDOW_HEIGHT / collDist * fovFactor;

    stripe->size = finalSize;
    wallHeights[i] = finalSize;
    stripe->brightness = collDist > 150? distance_to_color(collDist - 150, 0.01) : 1;
    stripe->normal = data->normal;
    stripe->collIdx = data->collIdx;
    stripe->wallWidth = data->wallWidth;
    stripe->pos = data->collpos;

    RenderObject *currentRenderObj = malloc(sizeof(RenderObject));
    if (currentRenderObj == NULL) {
        printf("tf is this trash lang \n");
    }
    currentRenderObj->dist_squared = collDist * collDist;
    currentRenderObj->val = stripe;
    currentRenderObj->type = WALL_STRIPE;

    free(data);
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

        switch (object->type) {
            case (int)ENTITY:
                currentRObj->val = object->val;
                currentRObj->type = ENTITY;
                pos = ((Entity *)object->val)->pos;
                break;
            case (int)EFFECT:
                currentRObj->val = ((Effect *)(object->val))->entity;
                currentRObj->type = ENTITY;
                pos = ((Entity *)currentRObj->val)->pos;
                break;
            case (int)PARTICLES:
                currentRObj->val = object->val;
                currentRObj->type = PARTICLES;
                pos = ((Particles *)currentRObj->val)->pos;
                break;
            case (int)ENEMY:
                currentRObj->val = object->val;
                currentRObj->type = ENEMY;
                pos = ((Enemy *)object->val)->entity->pos;
                break;
            case (int)ENEMY_SHOOTER:
                currentRObj->val = ((ShooterEnemy *)object->val)->enemy;
                currentRObj->type = ENEMY;
                pos = ((ShooterEnemy *)object->val)->enemy->entity->pos;
                break;
            case (int)BULLET:
                currentRObj->val = object->val;
                currentRObj->type = BULLET;
                pos = ((EnemyBullet *)object->val)->entity->pos;
                break;
            default:
                continue;
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

void renderWallStripe(WallStripe *stripe) {
    double brightness = stripe->brightness;

    double angleLightModifier = sin(v2_get_angle(stripe->normal));

    SDL_Texture *texture = stripe->texture;
    v2 textureSize = get_texture_size(texture);

    SDL_Rect srcRect = {(int)loop_clamp(stripe->collIdx * stripe->wallWidth, 0, textureSize.x), 0, 1, textureSize.y};

    SDL_Rect dstRect = {stripe->i * WINDOW_WIDTH / RESOLUTION_X + cameraOffset.x, WINDOW_HEIGHT / 2 - stripe->size / 2 - player->height + cameraOffset.y, WINDOW_WIDTH / RESOLUTION_X + 1,
                        stripe->size};
    

    BakedLightColor baked_light_color = get_light_color_by_pos(v2_add(stripe->pos, v2_mul(stripe->normal, to_vec(0.5))));

    // baked lights
    
    int r = clamp(125 * baked_light_color.r * brightness, 0, 255);
    int g = clamp(125 * baked_light_color.g * brightness, 0, 255);
    int b = clamp(125 * baked_light_color.b * brightness, 0, 255);

    SDL_SetTextureColorMod(texture, r, g, b);
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
        return baked_light_grid[baked_light_row][baked_light_col];
    } else {
        return (BakedLightColor){1, 1, 1};
    }
}

void renderHUD() {
    SDL_Rect leftHandRect = {player->handOffset.x + cameraOffset.x, player->handOffset.y + cameraOffset.y, 76 * 7, 91 * 7};

    leftHandRect.x += WINDOW_WIDTH * 3/5 - 40;
    leftHandRect.y -= 20;

    BakedLightColor baked_light_color;

    Animation *current_anim = leftHandSprite->animations[leftHandSprite->currentAnimationIdx];
    int current_anim_idx = leftHandSprite->currentAnimationIdx;

    bool first_check = current_anim_idx == 0;
    bool second_check = current_anim_idx == 1 && current_anim->frame > 1;
    if (first_check || second_check) {
        baked_light_color = get_light_color_by_pos(player->pos);
        baked_light_color.r = 0.5 + baked_light_color.r * 0.5;
        baked_light_color.g = 0.5 + baked_light_color.g * 0.5;
        baked_light_color.b = 0.5 + baked_light_color.b * 0.5;
    } else {
        baked_light_color.r = 2;
        baked_light_color.g = 1.8;
        baked_light_color.b = 1.5;
    }


    SDL_Texture *texture = getSpriteCurrentTexture(leftHandSprite);
    

    if (texture == NULL) {
        return;
    }
    int r = clamp(baked_light_color.r * 125, 0, 255);
    int g = clamp(baked_light_color.g * 125, 0, 255);
    int b = clamp(baked_light_color.b * 125, 0, 255);
    SDL_SetTextureColorMod(texture, r, g, b);
    SDL_RenderCopy(renderer, texture, NULL, &leftHandRect);

    SDL_Rect crosshairRect = {WINDOW_WIDTH / 2 - 8, WINDOW_HEIGHT / 2 - 8, 16, 16};

    SDL_RenderCopy(renderer, crosshair, NULL, &crosshairRect);

    int shots = max(player->pendingShots, (int)(player->shootChargeTimer * 3));

    SDL_Rect playerPendingShotsRect = {WINDOW_WIDTH / 2 + -10 * shots, WINDOW_HEIGHT * 0.8, 20 * shots, WINDOW_HEIGHT * 0.05};

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &playerPendingShotsRect);
}

void drawSkybox() {
    SDL_Texture *tex = skybox_texture;

    double x = loop_clamp(player->angle / fov * WINDOW_WIDTH, 0, WINDOW_WIDTH);

    double yOffsets = -player->height;

    SDL_Rect skybox_rect = {-x, yOffsets, WINDOW_WIDTH * 2, WINDOW_HEIGHT / 2};

    SDL_RenderCopy(renderer, tex, NULL, &skybox_rect);
}

void render(u64 delta) {  // #RENDER

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
                renderEntity((Entity *)rObj->val);
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
                    renderDirSprite(enemy->dirSprite, enemy->entity->pos, enemy->entity->size, enemy->entity->height);
                } else {
                    renderEntity(enemy->entity);
                }
                break;

            case (int)BULLET:;
                EnemyBullet *bullet = rObj->val;
                if (bullet->dirSprite != NULL) {
                    renderDirSprite(bullet->dirSprite, bullet->entity->pos, bullet->entity->size, bullet->entity->height);
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
Player *init_player(v2 pos) {
    Player *player = malloc(sizeof(Player));
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
    player->collider = malloc(sizeof(CircleCollider));
    player->collider->radius = 5;
    player->collider->pos = player->pos;
    player->maxHealth = 10;
    player->health = player->maxHealth;

    return player;
}

// CLEAR
RayCollisionData *ray_object(Raycast ray, obj *object) {
    switch (object->type) {
        case (int)ENEMY:;
            Enemy *enemy = object->val;
            RayCollisionData *enemyRayData = ray_circle(ray, *enemy->collider);
            if (enemyRayData != NULL) {
                enemyRayData->collider = enemy;
                enemyRayData->colliderType = ENEMY;
            }

            return enemyRayData;
            break;
        case (int)ENEMY_SHOOTER:;
            Enemy *shooter = ((ShooterEnemy *)object->val)->enemy;
            RayCollisionData *shooterRayData = ray_circle(ray, *shooter->collider);
            if (shooterRayData != NULL) {
                shooterRayData->collider = (ShooterEnemy *)object->val;
                shooterRayData->colliderType = ENEMY_SHOOTER;
            }

            return shooterRayData;
            break;
        case (int)PLAYER:;
            RayCollisionData *playerRayData = ray_circle(ray, *player->collider);
            if (playerRayData != NULL) {
                playerRayData->collider = player;
            }

            return playerRayData;
            break;
    }

    return NULL;
}

RayCollisionData *castRay(v2 pos, v2 dir) {
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

    if (found) {
        RayCollisionData *data = malloc(sizeof(RayCollisionData));
        data->wallWidth = tileSize;
        data->startpos = pos;
        data->collpos = v2_add(pos, v2_mul(dir, to_vec(dist * tileSize)));
        data->normal = v2_mul(lastStepDir, to_vec(-1));
        data->colliderTexture = wallTexture;
        if (lastStepDir.x != 0) {
            data->collIdx = (data->collpos.y - floor(data->collpos.y / tileSize) * tileSize) / tileSize;
        } else {
            data->collIdx = (data->collpos.x - floor(data->collpos.x / tileSize) * tileSize) / tileSize;
        }

        return data;
    } else {
        return NULL;
    }
}

RayCollisionData *ray_circle(Raycast ray, CircleCollider circle) {
    if (v2_distance(ray.pos, circle.pos) <= circle.radius || v2_dot(ray.dir, v2_dir(ray.pos, circle.pos)) < 0) {
        return NULL;
    }

    RayCollisionData *data = malloc(sizeof(RayCollisionData));

    v2 rayToCircle = v2_sub(circle.pos, ray.pos);
    double a = v2_dot(rayToCircle, ray.dir);
    double cSquared = v2_length_squared(rayToCircle);
    double bSquared = cSquared - a * a;  // pythagoras
    if (circle.radius * circle.radius - bSquared < 0) {
        free(data);
        return NULL;
    }                                                           // no imaginary numbers pls
    double d = sqrt(circle.radius * circle.radius - bSquared);  // more pythagoras

    // raypos + raydir * (a - d)
    v2 collision_pos = v2_add(ray.pos, v2_mul(ray.dir, to_vec(a - d)));  // woohoo!

    v2 normal = v2_dir(circle.pos, collision_pos);
    data->normal = normal;
    data->startpos = ray.pos;
    data->collpos = collision_pos;

    double collIdx = v2_get_angle(v2_dir(circle.pos, data->collpos)) / (2 * PI);

    data->collIdx = collIdx;
    data->wallWidth = circle.radius * PI;

    return data;
}

void freeObject(void *val, int type) {
    switch (type) {
        case (int)PARTICLES:;
            Particles *p = (Particles *)val;
            for (int i = 0; i < p->particleAmount; i++) {
                free(p->particles[i]->entity);
                free(p->particles[i]);
            }
            free(p->particles);
            free(p);
            break;
        case (int)EFFECT:;
            Effect *effect = (Effect *)val;
            freeObject(effect->entity->sprite, SPRITE);
            free(effect);
            break;
        case (int)SPRITE:;  // not gonna destroy the texture i think
            Sprite *s = val;
            for (int i = 0; i < s->animCount; i++) {
                Animation *anim = s->animations[i];
                free(anim->frames);
                free(anim);
            }
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
        case (int)ENTITY:;
            Entity *entity = val;
            freeObject(entity->sprite, SPRITE);
            free(entity);
            break;

        case (int)BULLET:;
            EnemyBullet *bullet = val;
            free(bullet->collider);
            if (bullet->dirSprite != NULL) {
                freeObject(bullet->dirSprite, DIR_SPRITE);
            }
            if (bullet->entity->sprite != NULL) {
                freeObject(bullet->entity->sprite, SPRITE);
            }

            free(bullet);
            break;

        default:
            free(val);
            break;
    }
}

void shakeCamera(double strength, int ticks, bool fade) {
    if (isCameraShaking) return;
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
                particles->particles[i]->entity = malloc(sizeof(Entity));
                particles->particles[i]->entity->pos = particles->pos;
                particles->particles[i]->entity->size = particles->particleSize;
                particles->particles[i]->entity->sprite = particles->particleSprite;
                particles->particles[i]->entity->height = particles->height;
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

Sprite *getRandomWallSprite() {
    Sprite **sprites = malloc(sizeof(Sprite *) * 2);
    Sprite *sprite1 = createSprite(true, 1);
    sprite1->currentAnimationIdx = 0;
    Animation *anim = create_animation(17);
    anim->frames = wallFrames;
    anim->loop = true;
    anim->playing = true;
    anim->fps = 10;
    sprite1->animations[0] = anim;

    Sprite *sprite2 = createSprite(false, 0);
    sprite2->texture = wallTexture;

    sprites[0] = sprite1;
    sprites[1] = sprite2;

    if (randi_range(0, 2) == 1) {
        return sprites[0];
    }

    return sprites[1];
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
        }
    }

    for (int r = 0; r < TILEMAP_HEIGHT; r++) {
        for (int c = 0; c < TILEMAP_WIDTH; c++) {
            int type = data[idx++];

            v2 tileMid = v2_add(v2_mul((v2){c, r}, to_vec(tileSize)), to_vec(tileSize / 2));

            switch (type) {
                case (int)P_PLAYER:
                    player->pos = tileMid;
                    arraylist_add(gameobjects, player, PLAYER);
                    break;
                case (int)P_SHOOTER:;
                    ShooterEnemy *shooter = createShooterEnemy(tileMid);
                    arraylist_add(gameobjects, shooter, ENEMY_SHOOTER);
                    break;
            }
        }
    }

    free(data);

    fclose(fh);
}

void freeAnimation(Animation *anim) {
    free(anim->frames);
    free(anim);
}

Animation *create_animation(int frameCount) {
    Animation *anim = malloc(sizeof(Animation));
    anim->playing = false;
    anim->frameCount = frameCount;
    anim->frames = malloc(sizeof(SDL_Texture *) * frameCount);
    anim->frame = 0;
    anim->fps = 5;
    anim->loop = true;
    anim->timeToNextFrame = 0;

    return anim;
}

void animationTick(Animation *anim, u64 delta) {
    if (!anim->playing) return;
    anim->timeToNextFrame -= mili_to_sec(delta);
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
        sprite->animations = malloc(sizeof(Animation *) * animCount);
    } else {
        sprite->animations = NULL;
    }
    return sprite;
}

void spritePlayAnim(Sprite *sprite, int idx) {
    sprite->currentAnimationIdx = idx;
    for (int i = 0; i < sprite->animCount; i++) {
        sprite->animations[i]->playing = false;
    }
    sprite->animations[idx]->frame = 0;
    sprite->animations[idx]->timeToNextFrame = 1 / sprite->animations[idx]->fps;
    sprite->animations[idx]->playing = true;
}

SDL_Texture *getSpriteCurrentTexture(Sprite *sprite) {
    if (!(sprite->isAnimated)) {
        return sprite->texture;
    } else {
        if (sprite->currentAnimationIdx == -1) return NULL;
        Animation *current = sprite->animations[sprite->currentAnimationIdx];
        if (current == NULL) return NULL;
        return current->frames[current->frame];
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
        particle->entity->pos = v2_add(particle->entity->pos, finalVel);

        particle->lifeTimer -= deltaSec;
        if (particle->lifeTimer <= 0) {
            particle->lifeTimer = particles->particleLifetime;
            particle->vel = particles->startVel;
            particle->entity->pos = particles->pos;
        }
    }
}

Effect *createEffect(v2 pos, v2 size, Sprite *sprite, double lifeTime) {
    Effect *effect = malloc(sizeof(Effect));

    if (effect == NULL) {
        printf("Failed to malloc effect \n");
    }

    effect->entity = malloc(sizeof(Entity));

    if (effect == NULL) {
        printf("Failed to malloc effect entity \n");
    }

    effect->entity->pos = pos;
    effect->entity->size = size;
    effect->entity->sprite = sprite;
    effect->entity->height = WINDOW_HEIGHT / 6;  // idk it works
    effect->lifeTime = lifeTime;

    return effect;
}

// Todo: add shoot cooldown for base shooting
void playerShoot() {
    shakeCamera(10, 4, true);

    player->canShoot = false;
    spritePlayAnim(leftHandSprite, 1);

    if (player->pendingShots > 0) {
        player->vel = v2_mul(get_player_forward(), to_vec(-1));
    }

    // first cast for walls
    RayCollisionData *entityRayData = castRayForEntities(player->pos, playerForward);
    RayCollisionData *wallRayData = castRay(player->pos, playerForward);

    v2 hitPos;

    double entitySquaredDist = INFINITY, wallSquaredDist = INFINITY;
    if (entityRayData != NULL) {
        entitySquaredDist = v2_distance_squared(player->pos, entityRayData->collpos);
    }
    if (wallRayData != NULL) {
        wallSquaredDist = v2_distance_squared(player->pos, wallRayData->collpos);
    }

    if (entitySquaredDist < wallSquaredDist) {
        enemyTakeDmg(entityRayData->collider, entityRayData->colliderType, 1);
        hitPos = entityRayData->collpos;
    } else if (entitySquaredDist > wallSquaredDist) {
        hitPos = wallRayData->collpos;
    } else {
        // didnt hit anything
        return;
    }

    v2 effectPos = v2_add(hitPos, v2_mul(playerForward, to_vec(-4)));

    Effect *hitEffect = createEffect(effectPos, to_vec(35), createSprite(true, 1), 1);

    if (hitEffect == NULL) {
        printf("hit effect is null \n");
        return;
    }

    hitEffect->entity->sprite->animations[0] = create_animation(5);
    hitEffect->entity->sprite->animations[0]->frames[0] = shootHitEffectFrames[0];
    hitEffect->entity->sprite->animations[0]->frames[1] = shootHitEffectFrames[1];
    hitEffect->entity->sprite->animations[0]->frames[2] = shootHitEffectFrames[2];
    hitEffect->entity->sprite->animations[0]->frames[3] = shootHitEffectFrames[3];
    hitEffect->entity->sprite->animations[0]->frames[4] = shootHitEffectFrames[4];
    hitEffect->entity->sprite->animations[0]->fps = 10;
    hitEffect->entity->sprite->animations[0]->loop = false;

    spritePlayAnim(hitEffect->entity->sprite, 0);

    add_game_object(hitEffect, EFFECT);
}

void effectTick(Effect *effect, u64 delta) {
    double deltaSec = mili_to_sec(delta);

    effect->lifeTime -= deltaSec;
    if (effect->lifeTime <= 0) {
        remove_game_object(effect, EFFECT);
        return;
    }
    spriteTick(effect->entity->sprite, delta);
}

void enemyTick(Enemy *enemy, u64 delta) {
    enemy->collider->pos = enemy->entity->pos;

    // add player vision

    v2 dir = v2_dir(enemy->entity->pos, player->pos);
    RayCollisionData *rayData = castRayForAll(v2_add(enemy->entity->pos, v2_mul(dir, to_vec(15))), dir);

    enemy->seeingPlayer = rayData != NULL && (Player *)rayData->collider == player;

    dSpriteTick(enemy->dirSprite, enemy->entity->pos, delta);
}

void enemyTakeDmg(void *enemy, int type, int dmg) {
    switch (type) {
        case (int)ENEMY:;
            Enemy *e = (Enemy *)enemy;
            e->health -= dmg;
            if (e->health <= 0) {
                // play death anim
                remove_game_object(e, ENEMY);
            }
            break;
        case (int)ENEMY_SHOOTER:;
            ShooterEnemy *shooter = (ShooterEnemy *)enemy;
            shooter->enemy->health -= dmg;
            if (shooter->enemy->health <= 0) {
                // play death anim
                remove_game_object(shooter, ENEMY_SHOOTER);
            }
            break;
    }
}

RayCollisionData *castRayForEntities(v2 pos, v2 dir) {
    Raycast ray = {pos, dir};

    RayCollisionData *data = NULL;
    double minSquaredDist = INFINITY;

    for (int i = 0; i < gameobjects->length; i++) {
        RayCollisionData *newData = ray_object(ray, arraylist_get(gameobjects, i));
        if (newData == NULL) continue;

        double currentSquaredDist = v2_distance_squared(pos, newData->collpos);
        if (currentSquaredDist < minSquaredDist) {
            minSquaredDist = currentSquaredDist;
            if (data != NULL) free(data);
            data = newData;
        } else {
            free(newData);
        }
    }

    return data;
}

RayCollisionData *castRayForAll(v2 pos, v2 dir) {
    RayCollisionData *entityRayData = castRayForEntities(pos, dir);
    RayCollisionData *wallRayData = castRay(pos, dir);

    if (entityRayData == NULL) return wallRayData;
    if (wallRayData == NULL) return entityRayData;

    double entitySquaredDist = v2_distance_squared(pos, entityRayData->collpos);
    double wallSquaredDist = v2_distance_squared(pos, wallRayData->collpos);

    if (entitySquaredDist > wallSquaredDist) return entityRayData;
    return wallRayData;
}

DirectionalSprite *createDirSprite(int dirCount) {
    DirectionalSprite *dSprite = malloc(sizeof(DirectionalSprite));
    dSprite->dir = (v2){1, 0};
    dSprite->dirCount = dirCount;
    dSprite->sprites = malloc(sizeof(Sprite *) * dirCount);

    return dSprite;
}

Sprite *getDSpriteCurrentSprite(DirectionalSprite *dSprite, v2 spritePos) {
    double playerAngleToSprite = v2_get_angle(v2_dir(player->pos, spritePos));

    if (dSprite->dirCount < 2) {
        return dSprite->sprites[0];
    } else {
        double relativeAngle = rad_to_deg(v2_get_angle(dSprite->dir)) + rad_to_deg(playerAngleToSprite);
        double min = 9999999;
        int minIdx = -1;
        for (int i = 0; i < dSprite->dirCount; i++) {
            double angle = (360.0 / dSprite->dirCount) * i;
            double aDist = angleDist(angle, relativeAngle);  // ANGLE DIST WORKS CORRECTLY 100%
            if (aDist < min && aDist != 0) {
                min = aDist;
                minIdx = i;
            }
        }

        return dSprite->sprites[minIdx];
    }

    return NULL;
}

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, u64 delta) { spriteTick(getDSpriteCurrentSprite(dSprite, spritePos), delta); }

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

    bullet->entity = malloc(sizeof(Entity));
    bullet->entity->pos = pos;
    bullet->entity->size = to_vec(20);
    bullet->entity->sprite = createSprite(false, 0);
    bullet->entity->sprite->texture = entityTexture;  // CHANGE LATER
    bullet->entity->height = WINDOW_HEIGHT / 6;
    bullet->dirSprite = NULL;
    bullet->dmg = 1;
    bullet->speed = 5.5;
    bullet->dir = dir;
    bullet->lifeTime = 5;
    bullet->lifeTimer = bullet->lifeTime;

    bullet->collider = malloc(sizeof(CircleCollider));
    bullet->collider->pos = bullet->entity->pos;
    bullet->collider->radius = 5;

    return bullet;
}

EnemyBullet *createTestBullet(v2 pos) { return createDefaultBullet(pos, (v2){1, 0}); }

bool intersectCircles(CircleCollider c1, CircleCollider c2) {
    return v2_distance_squared(c1.pos, c2.pos) < (c1.radius + c2.radius) * (c1.radius + c2.radius);  // dist^2 < (r1 + r2)^2
}

void bulletTick(EnemyBullet *bullet, u64 delta) {
    double deltaSec = mili_to_sec(delta);

    bullet->entity->pos = v2_add(bullet->entity->pos, v2_mul(bullet->dir, to_vec(bullet->speed)));
    bullet->collider->pos = bullet->entity->pos;

    bullet->lifeTimer -= deltaSec;
    if (bullet->lifeTimer <= 0) {
        remove_game_object(bullet, BULLET);
        return;
    }

    if (intersectCircles(*bullet->collider, *(player->collider))) {
        remove_game_object(bullet, BULLET);
        // deal damage to player
    }
}

void shooterEnemyShoot(ShooterEnemy *shooter) {
    EnemyBullet *bullet = createDefaultBullet(shooter->enemy->entity->pos, shooter->enemy->dir);
    if (bullet == NULL) {
        printf("Bullet is null \n");
        return;
    }
    add_game_object(bullet, BULLET);
}

void shooterTick(ShooterEnemy *shooter, u64 delta) {
    double deltaSec = mili_to_sec(delta);

    enemyTick(shooter->enemy, delta);

    if (!shooter->enemy->seeingPlayer) {
        shooter->enemy->dir = v2_dir(shooter->enemy->entity->pos, player->pos);
        if (shooter->shootCooldownTimer <= 0) {
            shooterEnemyShoot(shooter);
            shooter->shootCooldownTimer = shooter->shootCooldown;
        } else {
            shooter->shootCooldownTimer -= deltaSec;
        }
    }
}

ShooterEnemy *createShooterEnemy(v2 pos) {
    ShooterEnemy *shooter = malloc(sizeof(ShooterEnemy));
    if (shooter == NULL) {
        printf("Failed to allocate memory \n");
        return NULL;
    }
    shooter->enemy = createEnemy(pos);
    shooter->enemy->maxHealth = 3;
    shooter->enemy->health = shooter->enemy->maxHealth;

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
            CollisionData colData = getCircleTileMapCollision(*player->collider);
            if (colData.didCollide) {
                player->pos = v2_add(player->pos, colData.offset);
            }
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
    for (int r = 0; r < TILEMAP_HEIGHT * BAKED_LIGHT_RESOLUTION; r++) {
        for (int c = 0; c < TILEMAP_WIDTH * BAKED_LIGHT_RESOLUTION; c++) {
            // col, row / light_res * tile_size
            v2 current_pos = v2_mul(v2_div((v2){c, r}, to_vec(BAKED_LIGHT_RESOLUTION)), to_vec(tileSize));
            BakedLightColor col = {1, 1, 1};

            for (int i = 0; i < gameobjects->length; i++) {
				obj *current = arraylist_get(gameobjects, i);
				if (current->type != LIGHT_POINT) continue;
				LightPoint *point = current->val;

                double dist_to_point = v2_distance(point->pos, current_pos);
                
                v2 tile_pos = v2_div(current_pos, to_vec(tileSize));


                if (in_range(tile_pos.y, 0, TILEMAP_HEIGHT - 1) && in_range(tile_pos.x, 0, TILEMAP_WIDTH - 1) && levelTileMap[(int)tile_pos.y][(int)tile_pos.x] == P_WALL) {
                    continue;
                }

				RayCollisionData *data = castRay(current_pos, v2_dir(current_pos, point->pos));
				if (data == NULL) {
					double s = 1 / dist_to_point;
					col.r += s * point->strength * (double)point->color.r / 255;
					col.g += s * point->strength * (double)point->color.g / 255;
					col.b += s * point->strength * (double)point->color.b / 255;
				} else {
                    v2 collpos = data->collpos;
                    double dist = v2_distance(collpos, current_pos);
                    if (dist >= dist_to_point) {
                        double s = clamp(lerp(1, 0, dist_to_point / point->radius), 0, 1);
                        s *= s * s; // cubic
                        col.r += s * point->strength * (double)point->color.r / 255;
                        col.g += s * point->strength * (double)point->color.g / 255;
                        col.b += s * point->strength * (double)point->color.b / 255;
                    }
                }
            }

			baked_light_grid[r][c] = col;
        }
    }
}