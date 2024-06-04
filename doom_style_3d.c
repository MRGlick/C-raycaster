
#include "game_utils.c"
#include "globals.h"

SDL_Renderer *renderer;
SDL_Window *window;

// #DEFINITIONS

#define TPS 300
#define FPS 300
#define WINDOW_WIDTH 1080
#define WINDOW_HEIGHT 720
#define RESOLUTION_X 360
#define RESOLUTION_Y 180
#define X_SENSITIVITY 0.1
#define Y_SENSITIVITY 0.5
#define COLOR_BLACK (SDL_Color){0, 0, 0}
#define TRANSPARENT (SDL_Color){0, 0, 0, 0}
#define RENDER_DISTANCE 350
#define WALL_HEIGHT 36
#define NUM_WALL_THREADS 4

#define OUT_OF_SCREEN_POS (v2){WINDOW_WIDTH * 100, WINDOW_HEIGHT * 100}

enum Types {
    PLAYER,
    RAYCAST,
    CIRCLE_COLLIDER,
    BOX_COLLIDER,
    LINE_SEGMENT,
    RAY_COLL_DATA,
    DOOR,
    FLOOR_TILE,
    ENTITY,
    RENDER_OBJECT,
    WALL_STRIPE,
    LIGHT_POINT,
    CHASER,
    SPRITE,
    PARTICLES,
    PARTICLE,
    EFFECT,
    ENEMY,
    BULLET,
    DIR_SPRITE,
    ENEMY_SHOOTER
};

enum Tiles {
    WALL1 = 1,
    WALL2 = 2
};

typedef struct debugLine {
    char *name, *text;
} debugLine;


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
    SDL_Texture *texture; // not used if animated
    bool isAnimated;
    int currentAnimationIdx;
    Animation **animations;
    int animCount;
} Sprite;

typedef struct DirectionalSprite {
    Sprite **sprites;
    v2 dir; // global direction
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

typedef struct LineSegment {
    v2 pos1, pos2;
    double height;
    SDL_Color color;
    Sprite *sprite;
} LineSegment;

typedef struct FloorTile {
    v2 pos, size;
    Sprite *sprite;
} FloorTile;

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
    SDL_Color color;
} LightPoint;

typedef struct Chaser {
    Entity *entity;
    double speed;
    bool seeingPlayer;
    v2 lastSeenPlayerAt;
} Chaser;

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
    v2 offset; // adjusting position by this offset makes the object only touch and not overlap
    bool didCollide;
} CollisionData;

// #FUNCTIONS

void updateEntityCollisions(void *val, int type);

void initGrid(int ***gridPtr, int cols, int rows);

void resetGrid(int ***gridPtr, int cols, int rows);

void loadLevel(char *file);

void init();

void render(u64 delta);

RayCollisionData *rayCircle(Raycast ray, CircleCollider circle);

RayCollisionData *rayObject(Raycast ray, obj *object);

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

v2 getPlayerForward();

Animation *createAnimation(int frameCount);

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

FILE *ERROR_LOG;

bool running = true;
arraylist *gameobjects;
arraylist *heldKeys;
Player *player;
bool keyPressArr[26];
bool render_debug = false;
bool lockMouse = true;
bool renderLight = false;
bool isLMouseDown = false;

const double printCooldown = 0.05;
double startFov = 80;
double fov = 80;
double printCooldownTimer = 0.05;
const char *font = "font.ttf";
const SDL_Color floorColor = {50, 50, 50, 255};
const SDL_Color skyColor = {0, 0, 0, 255};
const SDL_Color fogColor = {0, 0, 0, 255};


TextureData *floorTexture;
TextureData *floorTexture2;
TextureData *ceilingTexture;
SDL_Texture *floorAndCeiling;

SDL_Texture *wallTexture;
SDL_Texture *entityTexture;
SDL_Texture *crosshair;

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

bool isCameraShaking = false;
int cameraShakeTicks;
int cameraShakeTicksLeft = 0;
double cameraShakeTimeToNextTick = 0.02;
double cameraShakeCurrentStrength = 0;
bool cameraShakeFadeActive = false;

RenderObject *wallStripesToRender[RESOLUTION_X];

v2 cameraOffset = {0, 0};

double wallHeights[RESOLUTION_X]; // for floor and ceiling optimization

Sprite *animatedWallSprite;

Sprite *leftHandSprite;

v2 playerForward;

char *levelToLoad = NULL;

const double PLAYER_SHOOT_COOLDOWN = 0.5;

// #MAIN
int main(int argc, char *argv[]) {
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) printf("Shit. \n");

    window = SDL_CreateWindow(
        "Doom style 3D!",
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
        levelToLoad = argv[1];
    }

    init();

    u64 tick_timer = 0, render_timer = 0;
    u64 last_time = SDL_GetTicks64();
    while (running) { // #GAME LOOP
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_input(event);
        }

        u64 now = SDL_GetTicks64();
        u64 delta = now - last_time;
        last_time = now;
        tick_timer += delta;
        // render_timer += delta;
        if (tick_timer >= 1000/TPS) {
            realFps = 1000.0 / tick_timer;
            tick(tick_timer);
            render(tick_timer);
            tick_timer = 0;
        }
        // if (render_timer >= 1000/FPS) {
        //     render(render_timer);
        //     render_timer = 0;
        // }
    }

    fclose(ERROR_LOG);

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

void initChaser() {
    Chaser *chaser = malloc(sizeof(Chaser));
    chaser->seeingPlayer = false;
    chaser->entity = malloc(sizeof(Entity));
    chaser->entity->pos = player->pos;
    chaser->entity->height = WINDOW_HEIGHT / 6;
    chaser->entity->size = to_vec(30);
    chaser->entity->sprite = createSprite(false, 0);
    chaser->entity->sprite->texture = entityTexture;
    chaser->speed = 40;
    add_game_object(chaser, CHASER);
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
        char num[getNumDigits(i + 1)];
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

void init() { // #INIT

    ERROR_LOG = fopen("error_log.txt", "w");
    if (ERROR_LOG == NULL) printf("What the fuck errno: %d\n", errno);

    floorAndCeiling = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, RESOLUTION_X, RESOLUTION_Y);
    
    floorTexture = TextureData_fromBMP("Textures/floor.bmp");
    floorTexture2 = TextureData_fromBMP("Textures/floor2.bmp");
    ceilingTexture = TextureData_fromBMP("Textures/ceiling.bmp");

    player = init_player((v2){0, 0});

    initGrid(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    initGrid(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    initGrid(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    tanHalfFOV = tan(deg_to_rad(fov / 2));
    tanHalfStartFOV = tan(deg_to_rad(startFov / 2));

    gameobjects = create_arraylist(10);

    heldKeys = create_arraylist(10);

    wallTexture = make_texture(renderer, "Textures/wall.bmp");

    wallFrames = malloc(sizeof(SDL_Texture *) * 17);
    getTextureFiles("Textures/WallAnim/wallAnim", 17, &wallFrames);

    crosshair = make_texture(renderer, "Textures/crosshair.bmp");

    animatedWallSprite = createSprite(true, 1);
    animatedWallSprite->animations[0] = createAnimation(17);
    animatedWallSprite->animations[0]->frames = wallFrames;
    animatedWallSprite->animations[0]->fps = 10;
    spritePlayAnim(animatedWallSprite, 0);

    leftHandSprite = createSprite(true, 2);
    leftHandSprite->animations[0] = createAnimation(1);
    leftHandSprite->animations[0]->frames[0] = make_texture(renderer, "Textures/rightHandAnim/rightHandAnim5.bmp");
    leftHandSprite->animations[1] = createAnimation(5);
    leftHandSprite->animations[1]->frames = malloc(sizeof(SDL_Texture *) * 5);
    getTextureFiles("Textures/rightHandAnim/rightHandAnim", 5, &leftHandSprite->animations[1]->frames);
    leftHandSprite->animations[1]->fps = 10;

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
        loadLevel(levelToLoad);
    } else {
        loadLevel("default_level.hclevel");
    }


} // #INIT END

v2 getPlayerForward() {
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

void checkCollisions() {
    // new plan: Circle AABB collision.
    // step 1: get tiles I want to intersect with
    // step 2: For collision with a tile, get the closest point from the tile to the circle. If the distance to that point is less than or equal to the radius of the circle
    // then push the circle back by the overlap, don't change the velocity.

    v2 velDir = player->vel;

    v2 dirs[] = {
        V2_ZERO
    };

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
    v2 move_dir_rotated = v2_rotate(move_dir, PI/2);

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

    player->vel = v2_lerp(
        player->vel,
        v2_add(v2_mul(move_dir, to_vec(keyVec.x)), v2_mul(move_dir_rotated, to_vec(keyVec.y))),
        0.15
    );

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

void chaserTick(Chaser *chaser, u64 delta) {
    spriteTick(chaser->entity->sprite, delta);

    v2 pos = chaser->entity->pos;

    v2 toPlayer = v2_sub(player->pos, pos);
    double dist = v2_length(toPlayer);

    v2 dirToPlayer = v2_div(toPlayer, to_vec(dist));

    RayCollisionData *data = castRay(pos, dirToPlayer);
    if (data == NULL || v2_distance_squared(pos, data->collpos) < dist * dist) { // something is between the chaser and the player
        chaser->seeingPlayer = false;
    } else {
        chaser->seeingPlayer = true;
        chaser->lastSeenPlayerAt = player->pos;
    }

    v2 desiredPos = chaser->seeingPlayer ? player->pos : chaser->lastSeenPlayerAt;

    v2 vel = v2_mul(v2_dir(pos, desiredPos), to_vec(chaser->speed));
    chaser->entity->pos = v2_add(pos, v2_mul(vel, to_vec(mili_to_sec(delta))));
    if (v2_distance_squared(pos, player->pos) < 490) {
        chaser->entity->sprite->currentAnimationIdx = 1;
    } else {
        chaser->entity->sprite->currentAnimationIdx = 0;
    }

}

void objectTick(void *obj, int type, u64 delta) {

    switch (type) {
        case (int)PLAYER:
            playerTick(delta);
            break;
        case (int)CHASER:
            chaserTick(obj, delta);
            break;
        case (int)EFFECT:
            effectTick(obj, delta);
            break;
        case (int)SPRITE: ;
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

    updateEntityCollisions(obj, type);
}

void tick(u64 delta) {

    playerForward = getPlayerForward();

    double deltaSec = mili_to_sec(delta);

    tanHalfFOV = tan(deg_to_rad(fov / 2));

    if (!canPrint) {
        printCooldownTimer -= deltaSec;
        if (printCooldownTimer <= 0) {
            canPrint = true;
            printCooldownTimer = printCooldown;
        } 
    }

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

void renderDebug() { // #DEBUG

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect playerRect = {player->pos.x - 5, player->pos.y - 5, 10, 10};
    SDL_Rect playerDirRect = {
        player->pos.x + v2_rotate_to((v2){10, 0}, deg_to_rad(player->angle)).x - 3,
        player->pos.y + v2_rotate_to((v2){10, 0}, deg_to_rad(player->angle)).y - 3,
        6,
        6
    };
    
    for (int row = 0; row < TILEMAP_HEIGHT; row++) {
        for (int col = 0; col < TILEMAP_WIDTH; col++) {
            
            int floorTile = floorTileMap[row][col];
            if (floorTile != -1) {
                SDL_Rect rect = {
                    col * tileSize,
                    row * tileSize,
                    tileSize,
                    tileSize
                };

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &rect);
            }

            int tile = levelTileMap[row][col];
            if (tile != -1) {
                SDL_Rect rect = {
                    col * tileSize,
                    row * tileSize,
                    tileSize,
                    tileSize
                };

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &playerRect);
    SDL_RenderFillRect(renderer, &playerDirRect);
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

    double wallSize = WALL_HEIGHT * WINDOW_HEIGHT/dist;

    int y = WINDOW_HEIGHT / 2 + wallSize / 2; 

    return (v2){x, y};
}

v2 screenToFloor(v2 pos) {

    v2 rayDir = getRayDirByIdx((int)pos.x);

    double wallSize = abs(pos.y - WINDOW_HEIGHT / 2) * 2; // size of a wall at that position
    if (wallSize == 0) {
        return to_vec(0);
    }
    
    double fovFactor = tanHalfStartFOV / tanHalfFOV;
    double dist = (WALL_HEIGHT * WINDOW_HEIGHT) / (wallSize) * fovFactor;

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

            int floor_pixel_i = floor_pixel.r << 24 | floor_pixel.g << 16 | floor_pixel.b << 8 | floor_pixel.a;

            ((int *)pixels)[floor_pixel_idx] = floor_pixel_i;


            int ceil_col = col;
            int ceil_row = RESOLUTION_Y / 2 - abs(RESOLUTION_Y / 2 - row);

            int ceil_pixel_idx = ceil_row * RESOLUTION_X + ceil_col;

            Pixel ceil_pixel;

            if (ceilingTile == -1) {
                ceil_pixel = (Pixel){0, 0, 0, 1};
            } else {

                ceil_pixel = TextureData_get_pixel(ceilingTex, loop_clamp(point.x, 0, 36), loop_clamp(point.y, 0, 36));
                ceil_pixel.r *= light;
                ceil_pixel.g *= light;
                ceil_pixel.b *= light;
            }

            int ceil_pixel_i = ceil_pixel.r << 24 | ceil_pixel.g << 16 | ceil_pixel.b << 8 | ceil_pixel.a;

            ((int *)pixels)[ceil_pixel_idx] = ceil_pixel_i;
               
        }
        
        
    }

    SDL_UnlockTexture(floorAndCeiling);
}

void drawFloorAndCeiling() {

    v2 offsets = (v2){cameraOffset.x, cameraOffset.y - player->height};

    SDL_Rect rect = {
        offsets.x,
        offsets.y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT
    };

    SDL_RenderCopy(renderer, floorAndCeiling, NULL, &rect);
}


void renderTexture(SDL_Texture *texture, v2 pos, v2 size, double height) {

    double cosAngleToForward = v2_cos_angle_between(playerForward, v2_sub(pos, player->pos));

    v2 screenFloorPos = floorToScreen(pos);
    if (v2_equal(screenFloorPos, OUT_OF_SCREEN_POS)) {
        return;
    } else {
        // printf("ftoscreen pos: (%.2f, %.2f), out of screen pos; (%.2f, %.2f) \n", screenFloorPos.x, screenFloorPos.y, OUT_OF_SCREEN_POS.x, OUT_OF_SCREEN_POS.y);
    }


    double dist = v2_distance(pos, player->pos);

    v2 textureSize = getTextureSize(texture);

    //WALL_HEIGHT * WINDOW_HEIGHT/dist * 80/fov;
    double fovFactor = tanHalfStartFOV/tanHalfFOV;
    double finalSize = WALL_HEIGHT * WINDOW_HEIGHT/dist * fovFactor / cosAngleToForward;
    v2 finalSizeVec = (v2){finalSize * size.x / 100, finalSize * size.y / 100};

    SDL_Rect dstRect = {
        screenFloorPos.x - finalSizeVec.x / 2 + cameraOffset.x,
        screenFloorPos.y - finalSizeVec.y / 2 - (height * 100 / dist) - player->height + cameraOffset.y,
        finalSizeVec.x,
        finalSizeVec.y
    };

    double light = distance_to_color(dist, 0.005);

    int color = light * 255;

    SDL_SetTextureColorMod(texture, color, color, color);
    SDL_RenderCopy(
        renderer,
        texture,
        NULL,
        &dstRect
    );
    SDL_SetTextureColorMod(texture, 255, 255, 255);
}

void renderDirSprite(DirectionalSprite *dSprite, v2 pos, v2 size, double height) {
    SDL_Texture *texture = getSpriteCurrentTexture(getDSpriteCurrentSprite(dSprite, pos));

    renderTexture(texture, pos, size, height);
}

void renderEntity(Entity *entity) { // RENDER ENTITY
    SDL_Texture *texture = getSpriteCurrentTexture(entity->sprite);

    renderTexture(texture, entity->pos, entity->size, entity->height);

}



typedef struct WallStripe {
    SDL_Texture *texture;
    double size;
    int i;
    double brightness; // a bunch of rendering bullshit: 
    v2 normal;
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
    double fovFactor = tanHalfStartFOV/tanHalfFOV;
    double finalSize = WALL_HEIGHT * WINDOW_HEIGHT/collDist * fovFactor;

    stripe->size = finalSize;
    wallHeights[i] = finalSize;
    stripe->brightness = distance_to_color(collDist, 0.01);
    stripe->normal = data->normal;
    stripe->collIdx = data->collIdx;
    stripe->wallWidth = data->wallWidth;

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

    int indicies[NUM_WALL_THREADS]; // trash lang

    for (int i = 0; i < NUM_WALL_THREADS; i++) {
        indicies[i] = i;
        threads[i] = SDL_CreateThread(addWallStripes_Threaded, "thread wall", &indicies[i]);

    }
    for (int i = 0; i < NUM_WALL_THREADS; i++) {
        SDL_WaitThread(threads[i], NULL);
    }
    
    // convert everything to sort objects, sort, get back our sorted goods, put into main list
    SortObject sortObjects[RESOLUTION_X];
    for (int i = 0; i < RESOLUTION_X; i++) {
        SortObject sObj = {wallStripesToRender[i], wallStripesToRender[i] == NULL ? 9999999999 : wallStripesToRender[i]->dist_squared};
        sortObjects[i] = sObj;
    }

    SortObject *sorted = mergeSort(sortObjects, RESOLUTION_X);
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
            case (int)CHASER:
                currentRObj->val = ((Chaser *)(object->val))->entity;
                currentRObj->type = ENTITY;
                pos = ((Entity *)currentRObj->val)->pos;
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

    SDL_Color colorMode = {
        clamp(200 + angleLightModifier * 80, 0, 255) * brightness,
        clamp(200 + angleLightModifier * 80, 0, 255) * brightness,
        clamp(200 + angleLightModifier * 80, 0, 255) * brightness,
    };

    SDL_Texture *texture = stripe->texture;
    v2 textureSize = getTextureSize(texture);

    SDL_Rect srcRect = {
        (int)loop_clamp(stripe->collIdx * stripe->wallWidth, 0, textureSize.x),
        0,
        1,
        textureSize.y
    };

    

    SDL_Rect dstRect = {
        stripe->i * WINDOW_WIDTH/RESOLUTION_X + cameraOffset.x,
        WINDOW_HEIGHT / 2 - stripe->size / 2 - player->height + cameraOffset.y,
        WINDOW_WIDTH/RESOLUTION_X,
        stripe->size
    };

    
    SDL_SetTextureColorMod(texture, colorMode.r, colorMode.g, colorMode.b);
    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
    free(stripe);

}

void renderParticles(Particles *particles) {
    for (int i = 0; i < particles->particleAmount; i++) {
        renderEntity(particles->particles[i]->entity);
    }
}

void renderHUD() {

    SDL_Rect leftHandRect = {
        player->handOffset.x + cameraOffset.x,
        player->handOffset.y + cameraOffset.y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT
    };

    SDL_Texture *texture = getSpriteCurrentTexture(leftHandSprite);
    
    if (texture == NULL) {
        return;
    }


    double redness = pow(0.5, (int)(max(player->shootChargeTimer, player->pendingShots) * 3));// 0.5 0.25 0.125 0.075
    if (redness == 1) redness = 0;
    else redness = (0.5 - redness) * 2;
    Uint8 redness8 = redness * 255;

    SDL_SetTextureColorMod(texture, 255, 255 - redness8, 255 - redness8);
    SDL_RenderCopy(renderer, texture, NULL, &leftHandRect);

    SDL_Rect crosshairRect = {
        WINDOW_WIDTH / 2 - 8,
        WINDOW_HEIGHT / 2 - 8,
        16,
        16
    };

    SDL_RenderCopy(renderer, crosshair, NULL, &crosshairRect);


    int shots = max(player->pendingShots, (int)(player->shootChargeTimer * 3));

    SDL_Rect playerPendingShotsRect = {
        WINDOW_WIDTH / 2 + -10  * shots,
        WINDOW_HEIGHT * 0.8,
        20 * shots,
        WINDOW_HEIGHT * 0.05
    };

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &playerPendingShotsRect);

}

void render(u64 delta) { // #RENDER

    char *newTitle = "FPS: ";
    char *fps = malloc(4);
    decimalToText(realFps, fps);


    SDL_SetWindowTitle(window, concat(newTitle, fps));

    

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    arraylist *renderList = getRenderList();

    
    

    SDL_SetRenderDrawColor(renderer, skyColor.r, skyColor.g, skyColor.b, 255);
    SDL_RenderFillRect(renderer, NULL);

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
            case (int)ENEMY: ;
                Enemy *enemy = rObj->val;
                if (enemy->dirSprite != NULL) {
                    renderDirSprite(enemy->dirSprite, enemy->entity->pos, enemy->entity->size, enemy->entity->height);
                } else {
                    renderEntity(enemy->entity);
                }
                break;

            case (int) BULLET: ;
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
RayCollisionData *rayObject(Raycast ray, obj *object) {
    switch (object->type) {
        case (int)ENEMY: ;
            Enemy *enemy = object->val;
            RayCollisionData *enemyRayData = rayCircle(ray, *enemy->collider);
            if (enemyRayData != NULL) {
                enemyRayData->collider = enemy;
                enemyRayData->colliderType = ENEMY;
            }

            return enemyRayData;
            break;
        case (int)ENEMY_SHOOTER: ;
            Enemy *shooter = ((ShooterEnemy *)object->val)->enemy;
            RayCollisionData *shooterRayData = rayCircle(ray, *shooter->collider);
            if (shooterRayData != NULL) {
                shooterRayData->collider = (ShooterEnemy *)object->val;
                shooterRayData->colliderType = ENEMY_SHOOTER;
            }

            return shooterRayData;
            break;
        case (int)PLAYER: ;
            RayCollisionData *playerRayData = rayCircle(ray, *player->collider);
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
    v2 scalingVec = {
        sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)),
        sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))    
    };

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

RayCollisionData *rayCircle(Raycast ray, CircleCollider circle) {

    if (v2_distance(ray.pos, circle.pos) <= circle.radius || v2_dot(ray.dir, v2_dir(ray.pos, circle.pos)) < 0) {
        return NULL;
    }

    RayCollisionData *data = malloc(sizeof(RayCollisionData));

    v2 rayToCircle = v2_sub(circle.pos, ray.pos);
    double a = v2_dot(rayToCircle, ray.dir);
    double cSquared = v2_length_squared(rayToCircle);
    double bSquared = cSquared - a * a; // pythagoras
    if (circle.radius * circle.radius - bSquared < 0) {
        free(data);
        return NULL;
    } // no imaginary numbers pls
    double d = sqrt(circle.radius * circle.radius - bSquared); // more pythagoras
    
    // raypos + raydir * (a - d)
    v2 collision_pos = v2_add(ray.pos, v2_mul(ray.dir, to_vec(a - d))); // woohoo!

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
        case (int)PARTICLES: ;
            Particles *p = (Particles *)val;
            for (int i = 0; i < p->particleAmount; i++) {
                free(p->particles[i]->entity);
                free(p->particles[i]);
            }
            free(p->particles);
            free(p);
            break;
        case (int)EFFECT: ;
            Effect *effect= (Effect *)val;
            freeObject(effect->entity->sprite, SPRITE);
            free(effect);
            break;
        case (int)SPRITE: ; // not gonna destroy the texture i think
            Sprite *s = val;
            for (int i = 0; i < s->animCount; i++) {
                Animation *anim = s->animations[i];
                free(anim->frames);
                free(anim);
            }
            free(s->animations);
            free(s);
            break;
        case (int)DIR_SPRITE: ;
            DirectionalSprite *dSprite = val;
            for (int i = 0; i < dSprite->dirCount; i++) {
                freeObject(dSprite->sprites[i], SPRITE);
            }
            free(dSprite->sprites);
            free(dSprite);
            break;
        case (int)ENTITY: ;
            Entity *entity = val;
            freeObject(entity->sprite, SPRITE);
            free(entity);
            break;
            
        case (int)BULLET: ;
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
        case (int)PARTICLES: ;
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
    Animation *anim = createAnimation(17);
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

void initGrid(int ***gridPtr, int cols, int rows) {
    *gridPtr = malloc(sizeof(int *) * rows);
    for (int i = 0; i < rows; i++) {
        (*gridPtr)[i] = malloc(sizeof(int) * cols);
        for (int j = 0; j < cols; j++) {
            (*gridPtr)[i][j] = -1;
        }
    }
}

void resetGrid(int ***gridPtr, int cols, int rows) {
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

void loadLevel(char *file) {

    clearLevel();

    resetGrid(&levelTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    resetGrid(&floorTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);
    resetGrid(&ceilingTileMap, TILEMAP_WIDTH, TILEMAP_HEIGHT);

    FILE *fh = fopen(file, "r");
    if (fh == NULL) {
        printf("File doesnt exist. \n");
        return;
    }

    fseek(fh, 0L, SEEK_END);
    int fileSize = ftell(fh);
    rewind(fh);


    char *data = malloc(sizeof(char) * fileSize); // sizeof char is 1 but i do it for clarity
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
                case (int)P_SHOOTER: ;
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

Animation *createAnimation(int frameCount) {
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
            if (anim->loop) anim->frame = 0;
            else anim->playing = false;
        } else {
            anim->frame++;
        }
        
    }
}

Sprite *createSprite(bool isAnimated, int animCount) {
    Sprite *sprite = malloc(sizeof(Sprite));
    sprite->isAnimated = isAnimated;
    sprite->texture = NULL;
    sprite->currentAnimationIdx = -1;
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
    sprite->animations[idx]->timeToNextFrame = 1/sprite->animations[idx]->fps;
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
    effect->entity->height = WINDOW_HEIGHT / 6; // idk it works
    effect->lifeTime = lifeTime;
    

    return effect;
}

// Todo: add shoot cooldown for base shooting
void playerShoot() {

    shakeCamera(10, 4, true);

    player->canShoot = false;
    spritePlayAnim(leftHandSprite, 1);

    if (player->pendingShots > 0) {
        player->vel = v2_mul(getPlayerForward(), to_vec(-1));
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
    
    hitEffect->entity->sprite->animations[0] = createAnimation(5);
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
        case (int)ENEMY: ;
            Enemy *e = (Enemy *)enemy;
            e->health -= dmg;
            if (e->health <= 0) {
                // play death anim
                remove_game_object(e, ENEMY);
            }
            break;
        case (int)ENEMY_SHOOTER: ;
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
        RayCollisionData *newData = rayObject(ray, arraylist_get(gameobjects, i));
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
            double aDist = angleDist(angle, relativeAngle); // ANGLE DIST WORKS CORRECTLY 100%
            if (aDist < min && aDist != 0) {
                min = aDist;
                minIdx = i;
            }
            
        }

        return dSprite->sprites[minIdx];

    }

    return NULL;
}

void dSpriteTick(DirectionalSprite *dSprite, v2 spritePos, u64 delta) {
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
    

    bullet->entity = malloc(sizeof(Entity));
    bullet->entity->pos = pos;
    bullet->entity->size = to_vec(20);
    bullet->entity->sprite = createSprite(false, 0);
    bullet->entity->sprite->texture = entityTexture; // CHANGE LATER
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

EnemyBullet *createTestBullet(v2 pos) {

    return createDefaultBullet(pos, (v2){1, 0});
}

bool intersectCircles(CircleCollider c1, CircleCollider c2) {
    return v2_distance_squared(c1.pos, c2.pos) < (c1.radius + c2.radius) * (c1.radius + c2.radius); // dist^2 < (r1 + r2)^2
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

    gridCheckEnd.x = (int)((circle.pos.x + circle.radius) / tileSize) + 2; //+ 2 to account for rounding up
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

void updateEntityCollisions(void *val, int type) {
    switch (type) {
        case (int)PLAYER: ;
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

    int charCount = getNumDigits(fileCount);

    for (int i = 0; i < fileCount; i++) {
        char num[charCount];
        sprintf(num, "%d", i + 1);
        char *fileWithNum = concat(fileName, num);
        char *fileWithExtension = concat(fileWithNum, ".bmp");
        SDL_Texture *tex = make_texture(renderer, fileWithExtension);
        (*textures)[i] = tex;
    }
}
