#include "raylib.h"
#include "raymath.h"
#include <assert.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAX_ENTITY 64
#define E_NULL 0

#define PLAYER_THRUST 300.0f
#define PLAYER_ROT_SPEED 5.0f
#define PLAYER_MAX_SPEED 400.0f
#define PLAYER_SHIP_SIZE 15.0f

#define BULLET_SPEED 500.0f
#define BULLET_RADIUS 2.0f
#define BULLET_LIFETIME 1.5f

#define ASTEROID_LARGE 3
#define ASTEROID_MEDIUM 2
#define ASTEROID_SMALL 1

#define ASTEROID_LARGE_RADIUS 40.0f
#define ASTEROID_MEDIUM_RADIUS 25.0f
#define ASTEROID_SMALL_RADIUS 15.0f

#define ASTEROID_MIN_SPEED 50.0f
#define ASTEROID_MAX_SPEED 150.0f
#define ASTEROID_ROT_SPEED 1.5f
#define ASTEROID_VERTS 8
#define INITIAL_ASTEROIDS 4

typedef enum { E_none, E_player, E_bullet, E_asteroid } Type;

typedef unsigned int EntityId;

typedef struct Entity {
    Type type;
    int health;
    Vector2 pos;
    Vector2 vel;
    float angle;
    float timer;
} Entity;

EntityId alloc_entity(void);
void free_entity(EntityId id);

Entity entities[MAX_ENTITY];

EntityId alloc_entity(void)
{
    for (int i = 1; i < MAX_ENTITY; i++) {
        if (entities[i].type == E_none)
            return i;
    }
    return E_NULL;
}

void free_entity(EntityId id) { entities[id].type = E_none; }

void update_player(EntityId id, float dt);
void draw_player(EntityId id);

void spawn_bullet(EntityId player_id);
void update_bullet(EntityId id, float dt);
void draw_bullet(EntityId id);

void spawn_asteroid(int size);
void spawn_asteroid_wave(void);
void update_asteroid(EntityId id, float dt);
void draw_asteroid(EntityId id);

void reset_game(void);
void check_collisions(void);
void render(void);

EntityId playerId;

void player_reset(void)
{
    Entity *player = &entities[playerId];
    player->pos = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    player->vel = (Vector2){0.0f, 0.0f};
    player->angle = 0.0f;
}

static float asteroid_radius(int size)
{
    switch (size) {
        case ASTEROID_LARGE:
            return ASTEROID_LARGE_RADIUS;
        case ASTEROID_MEDIUM:
            return ASTEROID_MEDIUM_RADIUS;
        case ASTEROID_SMALL:
            return ASTEROID_SMALL_RADIUS;
        default:
            return ASTEROID_SMALL_RADIUS;
    }
}

static Vector2 random_edge_pos(void)
{
    switch (GetRandomValue(0, 3)) {
        case 0:
            return (Vector2){(float)GetRandomValue(0, SCREEN_WIDTH),
                             -asteroid_radius(ASTEROID_LARGE)};
        case 1:
            return (Vector2){(float)GetRandomValue(0, SCREEN_WIDTH),
                             SCREEN_HEIGHT + asteroid_radius(ASTEROID_LARGE)};
        case 2:
            return (Vector2){-asteroid_radius(ASTEROID_LARGE),
                             (float)GetRandomValue(0, SCREEN_HEIGHT)};
        default:
            return (Vector2){SCREEN_WIDTH + asteroid_radius(ASTEROID_LARGE),
                             (float)GetRandomValue(0, SCREEN_HEIGHT)};
    }
}

static int circles_collide(Vector2 a, float ra, Vector2 b, float rb)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float r = ra + rb;
    return dx * dx + dy * dy <= r * r;
}

static void wrap_position(Vector2 *pos)
{
    if (pos->x < 0.0f)
        pos->x += SCREEN_WIDTH;
    else if (pos->x >= SCREEN_WIDTH)
        pos->x -= SCREEN_WIDTH;

    if (pos->y < 0.0f)
        pos->y += SCREEN_HEIGHT;
    else if (pos->y >= SCREEN_HEIGHT)
        pos->y -= SCREEN_HEIGHT;
}

static void split_asteroid_at(Vector2 pos, int size)
{
    if (size <= ASTEROID_SMALL)
        return;

    int next_size = size - 1;
    for (int i = 0; i < 2; i++) {
        EntityId id = alloc_entity();
        assert(id != E_NULL);
        if (id == E_NULL)
            continue;
        Entity *asteroid = &entities[id];
        float angle = DEG2RAD * (float)GetRandomValue(0, 360);
        float speed = (float)GetRandomValue((int)ASTEROID_MIN_SPEED, (int)ASTEROID_MAX_SPEED);

        asteroid->type = E_asteroid;
        asteroid->health = next_size;
        asteroid->pos = pos;
        asteroid->vel = (Vector2){cosf(angle) * speed, sinf(angle) * speed};
        asteroid->angle = DEG2RAD * (float)GetRandomValue(0, 360);
    }
}

static Vector2 asteroid_vertex(int i, float radius)
{
    float a = (float)i / (float)ASTEROID_VERTS * 2.0f * PI;
    float wobble = 0.7f + 0.3f * (float)((i * 3) % 5) / 4.0f;
    return (Vector2){cosf(a) * radius * wobble, sinf(a) * radius * wobble};
}

void spawn_asteroid(int size)
{
    EntityId id = alloc_entity();
    assert(id != E_NULL);
    if (id == E_NULL)
        return;
    Entity *asteroid = &entities[id];
    Vector2 pos = random_edge_pos();
    Vector2 to_center = Vector2Normalize(
        Vector2Subtract((Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f}, pos));
    float speed = (float)GetRandomValue((int)ASTEROID_MIN_SPEED, (int)ASTEROID_MAX_SPEED);
    float spread = DEG2RAD * (float)GetRandomValue(-45, 45);

    asteroid->type = E_asteroid;
    asteroid->health = size;
    asteroid->pos = pos;
    asteroid->vel = Vector2Scale(Vector2Rotate(to_center, spread), speed);
    asteroid->angle = DEG2RAD * (float)GetRandomValue(0, 360);
}

void spawn_asteroid_wave(void)
{
    for (int i = 0; i < INITIAL_ASTEROIDS; i++)
        spawn_asteroid(ASTEROID_LARGE);
}

void update_asteroid(EntityId id, float dt)
{
    Entity *asteroid = &entities[id];
    float spin = (id & 1) ? ASTEROID_ROT_SPEED : -ASTEROID_ROT_SPEED;

    asteroid->angle += spin * dt;
    asteroid->pos = Vector2Add(asteroid->pos, Vector2Scale(asteroid->vel, dt));
    wrap_position(&asteroid->pos);
}

void draw_asteroid(EntityId id)
{
    Entity *asteroid = &entities[id];
    float radius = asteroid_radius(asteroid->health);
    Vector2 verts[ASTEROID_VERTS];

    for (int i = 0; i < ASTEROID_VERTS; i++) {
        Vector2 local = asteroid_vertex(i, radius);
        verts[i] = Vector2Add(asteroid->pos, Vector2Rotate(local, asteroid->angle));
    }

    for (int i = 0; i < ASTEROID_VERTS; i++)
        DrawLineV(verts[i], verts[(i + 1) % ASTEROID_VERTS], WHITE);
}

void update_player(EntityId id, float dt)
{
    Entity *player = &entities[id];

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        player->angle -= PLAYER_ROT_SPEED * dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        player->angle += PLAYER_ROT_SPEED * dt;

    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        Vector2 thrust = Vector2Rotate((Vector2){0.0f, -PLAYER_THRUST * dt}, player->angle);
        player->vel = Vector2Add(player->vel, thrust);
    }

    float speed = Vector2Length(player->vel);
    if (speed > PLAYER_MAX_SPEED)
        player->vel = Vector2Scale(Vector2Normalize(player->vel), PLAYER_MAX_SPEED);

    player->pos = Vector2Add(player->pos, Vector2Scale(player->vel, dt));
    wrap_position(&player->pos);

    if (IsKeyPressed(KEY_SPACE))
        spawn_bullet(id);
}

void draw_player(EntityId id)
{
    Entity *player = &entities[id];
    Vector2 nose =
        Vector2Add(player->pos, Vector2Rotate((Vector2){0.0f, -PLAYER_SHIP_SIZE}, player->angle));
    Vector2 left = Vector2Add(
        player->pos,
        Vector2Rotate((Vector2){-PLAYER_SHIP_SIZE * 0.7f, PLAYER_SHIP_SIZE * 0.6f}, player->angle));
    Vector2 right = Vector2Add(
        player->pos,
        Vector2Rotate((Vector2){PLAYER_SHIP_SIZE * 0.7f, PLAYER_SHIP_SIZE * 0.6f}, player->angle));
    DrawTriangle(nose, left, right, WHITE);
}

void spawn_bullet(EntityId player_id)
{
    Entity *player = &entities[player_id];
    EntityId id = alloc_entity();
    assert(id != E_NULL);
    if (id == E_NULL)
        return;
    Entity *bullet = &entities[id];

    Vector2 dir = Vector2Rotate((Vector2){0.0f, -1.0f}, player->angle);

    bullet->type = E_bullet;
    bullet->pos = Vector2Add(player->pos, Vector2Scale(dir, PLAYER_SHIP_SIZE));
    bullet->vel = Vector2Add(player->vel, Vector2Scale(dir, BULLET_SPEED));
    bullet->timer = BULLET_LIFETIME;
}

void update_bullet(EntityId id, float dt)
{
    Entity *bullet = &entities[id];

    bullet->pos = Vector2Add(bullet->pos, Vector2Scale(bullet->vel, dt));
    wrap_position(&bullet->pos);

    bullet->timer -= dt;
    if (bullet->timer <= 0.0f)
        free_entity(id);
}

void draw_bullet(EntityId id)
{
    Entity *bullet = &entities[id];
    DrawCircleV(bullet->pos, BULLET_RADIUS, WHITE);
}

void check_collisions(void)
{
    for (int bi = 1; bi < MAX_ENTITY; bi++) {
        if (entities[bi].type != E_bullet)
            continue;

        for (int ai = 1; ai < MAX_ENTITY; ai++) {
            if (entities[ai].type != E_asteroid)
                continue;

            float ar = asteroid_radius(entities[ai].health);
            if (!circles_collide(entities[bi].pos, BULLET_RADIUS, entities[ai].pos, ar))
                continue;

            int size = entities[ai].health;
            Vector2 pos = entities[ai].pos;
            free_entity(bi);
            free_entity(ai);
            split_asteroid_at(pos, size);
            break;
        }
    }

    Entity *player = &entities[playerId];
    if (player->type != E_player)
        return;

    for (int ai = 1; ai < MAX_ENTITY; ai++) {
        if (entities[ai].type != E_asteroid)
            continue;

        float ar = asteroid_radius(entities[ai].health);
        if (!circles_collide(player->pos, PLAYER_SHIP_SIZE, entities[ai].pos, ar))
            continue;

        player->health--;
        if (player->health <= 0)
            reset_game();
        else
            player_reset();
        break;
    }
}

void reset_game(void)
{
    for (int i = 1; i < MAX_ENTITY; i++)
        entities[i].type = E_none;

    playerId = alloc_entity();
    assert(playerId != E_NULL);
    if (playerId == E_NULL)
        return;
    entities[playerId].type = E_player;
    entities[playerId].health = 3;
    entities[playerId].pos = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    entities[playerId].vel = (Vector2){0.0f, 0.0f};
    entities[playerId].angle = 0.0f;

    spawn_asteroid_wave();
}

void render(void)
{
    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 1; i < MAX_ENTITY; i++) {
        switch (entities[i].type) {
            case E_asteroid:
                draw_asteroid(i);
                break;
            case E_bullet:
                draw_bullet(i);
                break;
            case E_player:
                draw_player(i);
                break;
            default:
                break;
        }
    }
    DrawText("Astroids", SCREEN_WIDTH - MeasureText("Astroids", 20) - 20, 20, 20, WHITE);
    DrawFPS(10, 10);
    EndDrawing();
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astroids");
    SetTargetFPS(60);

    reset_game();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_R))
            reset_game();

        for (int i = 1; i < MAX_ENTITY; i++) {
            switch (entities[i].type) {
                case E_asteroid:
                    update_asteroid(i, dt);
                    break;
                case E_bullet:
                    update_bullet(i, dt);
                    break;
                case E_player:
                    update_player(i, dt);
                    break;
                default:
                    break;
            }
        }

        check_collisions();

        render();
    }

    CloseWindow();
    return 0;
}
