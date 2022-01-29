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

typedef struct KinematicInfo
{
    Vector2 vel;
    Vector2 pos;
    bool onGround;
} KinematicInfo;

// All the entity datas
typedef struct PlayerData
{
    KinematicInfo k;
    int grabbedEntity;
} PlayerData;
typedef Rectangle ObstacleData;
typedef Rectangle GroundData;
typedef struct ExtinguisherData
{
    KinematicInfo info;
    float amountLeft;
} ExtinguisherData;

// Entity types
enum Type
{
    Player,
    Obstacle,
    Ground,
    Extinguisher,

    // UPDATE MAX_TYPE WHEN YOU CHANGE THIS
};
#define MAX_TYPE Extinguisher
static const char *TypeNames[] = {
    "Player",
    "Obstacle",
    "Ground",
    "Extinguisher",
};

typedef struct Entity
{
    int id;
    enum Type type;
    union
    {
        PlayerData player;
        ObstacleData obstacle;
        GroundData ground;
        ExtinguisherData extinguisher;
        int empty_data[64]; // so I can add new fields without it breaking
    };
} Entity;

// editor state
static bool editing = true;
static int currentType = 0;
static Entity *currentEntity = NULL;

// game state
static int finishScreen = 0;
static Camera2D camera;

// entity stuff
typedef int ID;
static Entity entities[100];
static size_t entitiesLen = 0;
static ID curNextEntityID = 0;

int GetEntityIndex(ID id)
{
    for (int i = 0; i < entitiesLen; i++)
    {
        if (entities[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

// null if entity doesn't exist
Entity *GetEntity(ID id)
{
    int index = GetEntityIndex(id);
    if (index == -1)
    {
        return NULL;
    }
    return &entities[index];
}

// null if player not in entities
Entity *GetPlayerEntity()
{
    for (int i = 0; i < entitiesLen; i++)
    {
        if (entities[i].type == Player)
        {
            return &entities[i];
        }
    }
    return NULL;
}

void DeleteEntityIndex(int index)
{
    for (int i = index; i < entitiesLen - 1; i++)
    {
        entities[i] = entities[i + 1];
    }
    entitiesLen -= 1;
}

void DeleteEntity(ID id)
{
    int index = GetEntityIndex(id);
    if (index != -1)
    {
        DeleteEntityIndex(index);
    }
}

Entity *AddEntity(Entity e)
{
    e.id = curNextEntityID;
    curNextEntityID += 1;
    entities[entitiesLen] = e;
    entitiesLen += 1;
    return &entities[entitiesLen - 1];
}

void SaveEntities(const char *path)
{
    SaveFileData(path, (void *)entities, entitiesLen * sizeof(Entity));
}
void LoadEntities(const char *path)
{
    unsigned int bytesRead;
    unsigned char *data = LoadFileData(path, &bytesRead);
    for (int i = 0; i < bytesRead / sizeof(Entity); i++)
    {
        entities[i] = ((Entity *)data)[i];
        curNextEntityID = max(curNextEntityID, entities[i].id);
    }
    curNextEntityID = curNextEntityID + 1;
    entitiesLen = bytesRead / sizeof(Entity);
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

int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

// returns whichever has greater magnitude
float absmax(float a, float b)
{
    if (fabs(a) > fabs(b))
    {
        return a;
    }
    return b;
}

void DrawTexCentered(Texture t, Vector2 pos, float scale)
{
    DrawTextureEx(t, Vector2Add(pos, Vector2Scale((Vector2){.x = t.width, .y = t.height}, -scale * 0.5)), 0.0f, scale, WHITE);
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

Vector2 WorldMousePos()
{
    return Vector2Add(GetMousePosition(), Vector2Subtract(camera.target, camera.offset));
}

// Project a onto b
Vector2 Vector2Project(Vector2 a, Vector2 b)
{
    return Vector2Scale(Vector2Normalize(b), Vector2DotProduct(a, b) / Vector2Length(b));
}

enum Textures
{
    EXTINGUISHER_TEXTURE,
};

static Texture textures[] = {
    {0},
};

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    textures[EXTINGUISHER_TEXTURE] = LoadTexture("resources/Extinguisher.png");
    camera = (Camera2D){
        .offset = (Vector2){.x = SCREEN_SIZE / 2.0f, .y = SCREEN_SIZE / 2.0f},
        .target = (Vector2){0},
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
            .type = Player,
            .player = {
                .k = {
                    .pos = (Vector2){200, 300},
                    .vel = (Vector2){0},
                },
                .grabbedEntity = -1,
            },
        };
        entities[1] = (Entity){
            .type = Obstacle,
            .obstacle = {
                .x = 100,
                .y = 400,
                .width = 100,
                .height = 200,
            },
        };
        entitiesLen = 2;
    }
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

KinematicInfo GlideAndBounce(KinematicInfo k)
{
    k.onGround = false;
    for (int i = 0; i < entitiesLen; i++)
    {
        if (entities[i].type == Obstacle)
        {
            Rectangle obstacle = FixNegativeRect(entities[i].obstacle);
            Vector2 normal = {0};
            {
                Vector2 obstacleCenter = {.x = obstacle.x + obstacle.width / 2.0f, .y = obstacle.y + obstacle.height / 2.0f};
                Vector2 fromObstacleCenter = Vector2Subtract(k.pos, obstacleCenter);
                fromObstacleCenter.x = clamp(fromObstacleCenter.x, -obstacle.width / 2.0f, obstacle.width / 2.0f);
                fromObstacleCenter.y = clamp(fromObstacleCenter.y, -obstacle.height / 2.0f, obstacle.height / 2.0f);
                Vector2 closestPointOnObstacle = Vector2Add(fromObstacleCenter, obstacleCenter);
                if (Vector2Distance(closestPointOnObstacle, k.pos) < player_radius)
                {
                    normal = Vector2Normalize(Vector2Subtract(k.pos, closestPointOnObstacle));
                    k.pos = Vector2Add(closestPointOnObstacle, Vector2Scale(normal, player_radius));
                }
            }
            k.vel = Vector2Reflect(k.vel, normal);
        }
        else if (entities[i].type == Ground && RectHasPoint(entities[i].obstacle, k.pos))
        {
            k.onGround = true;
        }
    }
    return k;
}

void ProcessEntity(Entity *e)
{
    switch (e->type)
    {
    case Player:
    {
        camera.target = Vector2Lerp(camera.target, e->player.k.pos, GetFrameTime() * 5.0);
        float delta = GetFrameTime();
        Vector2 movement = {
            .x = (float)IsKeyDown(KEY_D) - (float)IsKeyDown(KEY_A),
            .y = (float)IsKeyDown(KEY_S) - (float)IsKeyDown(KEY_W),
        };

        movement = Vector2Normalize(movement);
        e->player.k = GlideAndBounce(e->player.k);
        if (e->player.k.onGround)
            e->player.k.vel = Vector2Lerp(e->player.k.vel, Vector2Scale(movement, 50.0f), delta * 4.0f);
        e->player.k.pos = Vector2Add(e->player.k.pos, Vector2Scale(e->player.k.vel, delta));
        break;
    }
    case Extinguisher:
    {
        if (GetPlayerEntity()->player.grabbedEntity != e->id)
        {
            e->extinguisher.info = GlideAndBounce(e->extinguisher.info);
            break;
        }
    }
    }
}

void DrawEntity(Entity e)
{
    switch (e.type)
    {
    case Player:
    {
        DrawCircleV(e.player.k.pos, player_radius, PINK);
        break;
    }
    case Obstacle:
    {
        DrawRectanglePro(FixNegativeRect(e.obstacle), (Vector2){0}, 0.0, GRAY);
        break;
    }
    case Ground:
    {
        printf("Drawing %f %f\n", e.ground.x, e.ground.width);
        DrawRectanglePro(FixNegativeRect(e.ground), (Vector2){0}, 0.0, GREEN);
        break;
    }
    case Extinguisher:
    {
        DrawTexCentered(textures[EXTINGUISHER_TEXTURE], e.extinguisher.info.pos, 0.35f);
        break;
    }
    }
}

void UpdateGameplayScreen(void)
{
    if (IsKeyPressed(KEY_TAB))
        editing = !editing;

    for (int i = 0; i < entitiesLen; i++)
    {
        ProcessEntity(entities, i);
    }

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
            GetPlayerEntity()->player.k.pos = WorldMousePos();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (currentType == Player)
            {
            }
            else
            {
                Entity toAdd = {0};
                toAdd.type = currentType;
                if (currentType == Ground || currentType == Obstacle)
                {
                    toAdd.ground.x = WorldMousePos().x;
                    toAdd.ground.y = WorldMousePos().y;
                }
                else
                {
                    toAdd.extinguisher.info.pos = WorldMousePos();
                    toAdd.extinguisher.info.vel = (Vector2){0};
                }
                currentEntity = AddEntity(toAdd);
            }
        }

        if (currentEntity != NULL && (currentEntity->type == Ground || currentEntity->type == Obstacle))
        {
            currentEntity->ground.width = WorldMousePos().x - currentEntity->ground.x;
            currentEntity->ground.height = WorldMousePos().y - currentEntity->ground.y;
            currentEntity->ground.width = absmax(3.0, currentEntity->ground.width);
            currentEntity->ground.height = absmax(3.0, currentEntity->ground.height);
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            currentEntity = NULL;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            for (int i = 0; i < entitiesLen; i++)
            {
                switch (entities[i].type)
                {
                case Obstacle:
                case Ground:
                {
                    if (RectHasPoint(entities[i].ground, WorldMousePos()))
                    {
                        DeleteEntityIndex(i);
                        break;
                    }
                    break;
                }
                case Extinguisher:
                {
                    if (Vector2Distance(entities[i].extinguisher.info.pos, WorldMousePos()) < 15.0f)
                    {
                        DeleteEntityIndex(i);
                        break;
                    }
                    break;
                }
                }
            }
        }
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){17, 17, 17, 255});
    BeginMode2D(camera);
    for (int i = 0; i < entitiesLen; i++)
    {
        if(entities[i].type == Player)
            continue;
        DrawEntity(entities[i]);
    }
    DrawEntity(*GetPlayerEntity());
    EndMode2D();

    if (editing)
    {
        DrawText("Editing", 0, 0, 16, RED);
        DrawText(TypeNames[currentType], 100, 0, 16, RED);
    }
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