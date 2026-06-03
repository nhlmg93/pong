#include "raylib.h"
#include "raymath.h"
#include <assert.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MAX_ENTITY 64
#define E_NULL 0

typedef enum {
    E_none,
    E_player_one,
    E_player_two,
    E_ball,
} Type;

typedef unsigned int EntityId;

typedef struct Entity {
    Type type;
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
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

void reset_game(void);

void update_player_one(float dt);

void update_player_two(Entity *player, Entity *ball, float dt);
void draw_paddle(Entity *player);

void update_ball(Entity *ball, float dt);
void draw_ball(Entity *ball);
void check_collisions(void);

EntityId playerOneId;
EntityId playerTwoId;
EntityId ballId;

static Vector2 random_ball_velocity(float speed)
{
    float dir_x = GetRandomValue(0, 1) ? 1.0f : -1.0f;
    float dir_y = GetRandomValue(-1, 1) == 0 ? 0.5f : -0.5f;
    return Vector2Scale((Vector2){dir_x, dir_y}, speed);
}

void reset_game(void)
{
    const float paddle_x_inset = 30.0f;
    const float ball_speed = 350.0f;

    for (int i = 1; i < MAX_ENTITY; i++)
        entities[i].type = E_none;

    playerOneId = alloc_entity();
    entities[playerOneId].type = E_player_one;
    entities[playerOneId].position = (Vector2){paddle_x_inset, SCREEN_HEIGHT / 2.0f};
    entities[playerOneId].velocity = (Vector2){0.0f, 0.0f};

    playerTwoId = alloc_entity();
    entities[playerTwoId].type = E_player_two;
    entities[playerTwoId].position = (Vector2){SCREEN_WIDTH - paddle_x_inset, SCREEN_HEIGHT / 2.0f};
    entities[playerTwoId].velocity = Vector2Zero();

    ballId = alloc_entity();
    entities[ballId].type = E_ball;
    entities[ballId].position = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    entities[ballId].velocity = random_ball_velocity(ball_speed);
}

void update_player_one(float dt)
{
    Entity *player = &entities[playerOneId];
    const float speed = 400.0f;
    const float half_height = 50.0f;

    player->velocity = Vector2Zero();
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        player->velocity = (Vector2){0.0f, -speed};
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        player->velocity = (Vector2){0.0f, speed};

    player->position = Vector2Add(player->position, Vector2Scale(player->velocity, dt));
    player->position.y = Clamp(player->position.y, half_height, SCREEN_HEIGHT - half_height);
}

void draw_paddle(Entity *player)
{
    const float width = 15.0f;
    const float height = 100.0f;
    Rectangle rect = {
        player->position.x - width / 2.0f,
        player->position.y - height / 2.0f,
        width,
        height,
    };
    DrawRectangleRec(rect, WHITE);
}

void update_ball(Entity *ball, float dt)
{
    ball->position = Vector2Add(ball->position, Vector2Scale(ball->velocity, dt));
}

void draw_ball(Entity *ball)
{
    const float radius = 10.0f;
    DrawCircleV(ball->position, radius, WHITE);
}

void check_collisions(void)
{
    Entity *ball = &entities[ballId];
    const float ball_radius = 10.0f;
    const float paddle_width = 15.0f;
    const float paddle_height = 100.0f;
    const float ball_speed = 350.0f;

    if (ball->position.y - ball_radius <= 0.0f) {
        ball->position.y = ball_radius;
        ball->velocity.y = fabsf(ball->velocity.y);
    } else if (ball->position.y + ball_radius >= SCREEN_HEIGHT) {
        ball->position.y = SCREEN_HEIGHT - ball_radius;
        ball->velocity.y = -fabsf(ball->velocity.y);
    }

    Entity *paddles[] = {&entities[playerOneId], &entities[playerTwoId]};
    for (int i = 0; i < 2; i++) {
        Entity *paddle = paddles[i];
        Rectangle rect = {
            paddle->position.x - paddle_width / 2.0f,
            paddle->position.y - paddle_height / 2.0f,
            paddle_width,
            paddle_height,
        };

        if (!CheckCollisionCircleRec(ball->position, ball_radius, rect))
            continue;

        int moving_toward =
            (i == 0 && ball->velocity.x < 0.0f) || (i == 1 && ball->velocity.x > 0.0f);
        if (!moving_toward)
            continue;

        float hit_offset =
            (ball->position.y - paddle->position.y) / (paddle_height / 2.0f);
        hit_offset = Clamp(hit_offset, -1.0f, 1.0f);

        ball->velocity.x = (i == 0 ? 1.0f : -1.0f) * fabsf(ball->velocity.x);
        ball->velocity.y = hit_offset * ball_speed;
        ball->velocity = Vector2Scale(Vector2Normalize(ball->velocity), ball_speed);

        if (i == 0)
            ball->position.x = rect.x + rect.width + ball_radius;
        else
            ball->position.x = rect.x - ball_radius;
    }

    if (ball->position.x < -ball_radius || ball->position.x > SCREEN_WIDTH + ball_radius) {
        ball->position = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
        ball->velocity = random_ball_velocity(ball_speed);
    }
}

void update_player_two(Entity *player, Entity *ball, float dt)
{
    const float speed = 400.0f;
    const float half_height = 50.0f;
    const float deadzone = 5.0f;

    float target_y = ball->position.y;

    player->velocity = Vector2Zero();
    if (player->position.y < target_y - deadzone)
        player->velocity = (Vector2){0.0f, speed};
    else if (player->position.y > target_y + deadzone)
        player->velocity = (Vector2){0.0f, -speed};

    player->position = Vector2Add(player->position, Vector2Scale(player->velocity, dt));
    player->position.y = Clamp(player->position.y, half_height, SCREEN_HEIGHT - half_height);
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong");
    SetTargetFPS(60);

    reset_game();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        update_player_one(dt);
        update_player_two(&entities[playerTwoId], &entities[ballId], dt);
        update_ball(&entities[ballId], dt);

        check_collisions();

        BeginDrawing();
        ClearBackground(BLACK);
        draw_paddle(&entities[playerOneId]);
        draw_paddle(&entities[playerTwoId]);
        draw_ball(&entities[ballId]);
        DrawText("Pong", SCREEN_WIDTH - MeasureText("Pong", 20) - 20, 20, 20, WHITE);
        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
