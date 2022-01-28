/**********************************************************************************************
 *
 *   raylib - Advance Game template
 *
 *   Gameplay Screen Functions Definitions (Init, Update, Draw, Unload)
 *
 *   Copyright (c) 2014-2022 Ramon Santamaria (@raysan5)
 *
 *   This software is provided "as-is", without any express or implied warranty. In no event
 *   will the authors be held liable for any damages arising from the use of this software.
 *
 *   Permission is granted to anyone to use this software for any purpose, including commercial
 *   applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 *     1. The origin of this software must not be misrepresented; you must not claim that you
 *     wrote the original software. If you use this software in a product, an acknowledgment
 *     in the product documentation would be appreciated but is not required.
 *
 *     2. Altered source versions must be plainly marked as such, and must not be misrepresented
 *     as being the original software.
 *
 *     3. This notice may not be removed or altered from any source distribution.
 *
 **********************************************************************************************/

#include "raylib.h"
#include "raymath.h"
#include "screens.h"
#include <stddef.h>
#include <stdio.h>

#define SCREEN_SIZE 900 // screen assumed to be square. Used for camera offset
const float player_radius = 18.0;

enum Type
{
    Obstacle,
    Ground,

    // UPDATE MAX_TYPE WHEN YOU CHANGE THIS
};
#define MAX_TYPE Ground
static const char *TypeNames[] = {
    "Obstacle",
    "Ground",
};

union Data
{
    Rectangle rect;
};

typedef struct Entity
{
    enum Type type;
    union Data data;
} Entity;

static bool editing = true;
static Entity currentEntity;
static bool creatingEntity = false;
static enum Type currentType = Obstacle;

static int finishScreen = 0;
static Vector2 position = {.x = 200, .y = 300};
static Camera2D camera;
static Vector2 velocity = {0};
static Vector2 fireExtinguisherPosition = {100, 100};
static Texture fireExtinguisher;

static Entity entities[100];
static size_t entities_len = 0;
//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// returns whichever has greater magnitude
float absmax(float a, float b)
{
    if (fabs(a) > fabs(b))
    {
        return a;
    }
    return b;
}

void RemoveEntity(int index)
{
    for (int i = index; i < entities_len - 1; i++)
    {
        entities[i] = entities[i + 1];
    }
    entities_len--;
}

void SaveEntities(const char *path)
{
    SaveFileData(path, (void *)entities, entities_len * sizeof(Entity));
}
void LoadEntities(const char *path)
{
    unsigned int bytesRead;
    unsigned char *data = LoadFileData(path, &bytesRead);
    for (int i = 0; i < bytesRead / sizeof(Entity); i++)
    {
        entities[i] = ((Entity *)data)[i];
    }
    entities_len = bytesRead / sizeof(Entity);
    UnloadFileText(data);

    // for debugging stuff, also put any programmatic level modifications here
    /*
    for(int i = 0; i < entities_len; i++) {
        Rectangle r = entities[i].data.rect;
        if(fabs(r.width) < 1.0) {
            RemoveEntity(i);
            break;
        }
        printf("%f %f %f %f\n", r.x, r.y, r.width, r.height);
    }*/
}

float clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

// Project a onto b
Vector2 Vector2Project(Vector2 a, Vector2 b)
{
    return Vector2Scale(Vector2Normalize(b), Vector2DotProduct(a, b) / Vector2Length(b));
}

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    camera = (Camera2D){
        .offset = (Vector2){.x = SCREEN_SIZE / 2.0f, .y = SCREEN_SIZE / 2.0f},
        .target = position,
        .rotation = 0.0,
        .zoom = 1.0,
    };
    if (FileExists("resources/saved.level"))
    {
        LoadEntities("resources/saved.level");
    }
    else
    {
        entities[0] = (Entity){
            .type = Obstacle,
            .data = (Rectangle){
                .x = 400,
                .y = 400,
                .width = 100,
                .height = 200,
            },
        };
        entities[1] = (Entity){
            .type = Obstacle,
            .data = (Rectangle){
                .x = 100,
                .y = 400,
                .width = 100,
                .height = 200,
            },
        };
        entities_len = 2;
    }

    fireExtinguisher = LoadTexture("resources/Fire Extinguisher.png");
}

Rectangle FixNegativeRect(Rectangle rect)
{
    if (rect.width < 0.0)
    {
        rect.x += rect.width;
        rect.width *= -1.0;
    }
    if (rect.height < 0.0)
    {
        rect.y += rect.height;
        rect.height *= -1.0;
    }
    return rect;
}

// unlike the raylib function for this this one works on negative width and height
bool RectHasPoint(Rectangle rect, Vector2 point)
{
    rect = FixNegativeRect(rect);
    return CheckCollisionPointRec(point, rect);
}

void ProcessEntity(Entity *entities, size_t i)
{
    switch (entities[i].type)
    {
    case Obstacle:
    {
        Rectangle obstacle = FixNegativeRect(entities[i].data.rect);
        Vector2 normal = {0};
        Vector2 newPosition = position;
        {
            Vector2 obstacleCenter = {.x = obstacle.x + obstacle.width / 2.0f, .y = obstacle.y + obstacle.height / 2.0f};
            Vector2 fromObstacleCenter = Vector2Subtract(position, obstacleCenter);
            fromObstacleCenter.x = clamp(fromObstacleCenter.x, -obstacle.width / 2.0f, obstacle.width / 2.0f);
            fromObstacleCenter.y = clamp(fromObstacleCenter.y, -obstacle.height / 2.0f, obstacle.height / 2.0f);
            Vector2 closestPointOnObstacle = Vector2Add(fromObstacleCenter, obstacleCenter);
            if (Vector2Distance(closestPointOnObstacle, position) < player_radius)
            {
                normal = Vector2Normalize(Vector2Subtract(position, closestPointOnObstacle));
                newPosition = Vector2Add(closestPointOnObstacle, Vector2Scale(normal, player_radius));
            }
        }
        position = newPosition;
        velocity = Vector2Reflect(velocity, normal);
        break;
    }
    }
}

void DrawEntity(Entity *entities, size_t i)
{
    Entity e = entities[i];
    switch (e.type)
    {
    case Obstacle:
    {
        DrawRectanglePro(FixNegativeRect(e.data.rect), (Vector2){0}, 0.0, GRAY);
        break;
    }
    case Ground:
    {
        DrawRectanglePro(FixNegativeRect(e.data.rect), (Vector2){0}, 0.0, GREEN);
        break;
    }
    }
}

void UpdateGameplayScreen(void)
{
    if (IsKeyPressed(KEY_TAB))
        editing = !editing;

    camera.target = Vector2Lerp(camera.target, position, GetFrameTime() * 5.0);
    float delta = GetFrameTime();
    Vector2 movement = {
        .x = (float)IsKeyDown(KEY_D) - (float)IsKeyDown(KEY_A),
        .y = (float)IsKeyDown(KEY_S) - (float)IsKeyDown(KEY_W),
    };

    movement = Vector2Normalize(movement);

    bool onGround = false;
    for (int i = 0; i < entities_len; i++)
    {
        if (entities[i].type == Ground && RectHasPoint(entities[i].data.rect, position))
            onGround = true;
        ProcessEntity(entities, i);
    }
    if (onGround)
        velocity = Vector2Lerp(velocity, Vector2Scale(movement, 400.0f), delta * 9.0f);
    position = Vector2Add(position, Vector2Scale(velocity, delta));

    // separate loops for gameplay and editing so editing can break early (deleting
    // multiple entities per loop I can't be bothered to implement)
    if (editing)
    {
        currentType += (int)GetMouseWheelMove();
        currentType %= MAX_TYPE + 1;
        if (currentType < 0)
            currentType = MAX_TYPE;

        if (IsKeyPressed(KEY_F1))
        {
            SaveEntities("resources/saved.level");
            LoadEntities("resources/saved.level");
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
            position = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            creatingEntity = true;
            currentEntity.type = currentType;
            currentEntity.data.rect.x = GetMousePosition().x;
            currentEntity.data.rect.y = GetMousePosition().y;
        }
        currentEntity.data.rect.width = GetMousePosition().x - currentEntity.data.rect.x;
        currentEntity.data.rect.height = GetMousePosition().y - currentEntity.data.rect.y;
        currentEntity.data.rect.width = absmax(3.0, currentEntity.data.rect.width);
        currentEntity.data.rect.height = absmax(3.0, currentEntity.data.rect.height);
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            entities[entities_len] = currentEntity;
            entities_len += 1;
            creatingEntity = false;
        }

        for (int i = 0; i < entities_len; i++)
        {
            switch (entities[i].type)
            {
            case Obstacle:
            case Ground:
            {
                if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && RectHasPoint(entities[i].data.rect, GetMousePosition()))
                {
                    RemoveEntity(i);
                    break;
                }
                break;
            }
            }
        }
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){17, 17, 17, 255});
    if (editing)
    {
        DrawText("Editing", 0, 0, 16, RED);
        DrawText(TypeNames[currentType], 100, 0, 16, RED);
    }
    BeginMode2D(camera);

    for (int i = 0; i < entities_len; i++)
    {
        DrawEntity(entities, i);
    }
    if (editing && creatingEntity)
    {
        DrawEntity(&currentEntity, 0);
    }
    DrawCircleV(position, player_radius, PINK);
    DrawTextureEx(fireExtinguisher, fireExtinguisherPosition, 0.0f, 0.35f, WHITE);
    EndMode2D();
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    // TODO: Unload GAMEPLAY screen variables here!
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}