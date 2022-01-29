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
const float player_grab_radius = 50.0;
const char *level_name = "resources/saved.level";

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
    float health;
} PlayerData;
typedef Rectangle ObstacleData;
typedef Rectangle GroundData;
typedef struct FireData
{
    Rectangle rect;
    float fireLeft;
    float fireParticleTimer;
} FireData;
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
    Fire,

    // UPDATE MAX_TYPE WHEN YOU CHANGE THIS
};
#define MAX_TYPE Fire
static const char *TypeNames[] = {
    "Player",
    "Obstacle",
    "Ground",
    "Extinguisher",
    "Fire",
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
        FireData fire;
        int empty_data[64]; // so I can add new fields without it breaking
    };
} Entity;


enum ParticleType {
    RetardantParticle,
    FireParticle,
};
typedef struct Particle
{
    Vector2 pos;
    Vector2 vel;
    float lifetime;
    float max_lifetime; // used to compute color alpha
    Color color;
    enum ParticleType type;
} Particle;

// editor state
static bool editing = true;
static int currentType = 0;
static Entity *currentEntity = NULL;

// game state
static int finishScreen = 0;
static Vector2 spawnPoint = {0};
static Camera2D camera;
static int frameID = 0;

// entity stuff
typedef int ID;
static Entity entities[100];
static size_t entitiesLen = 0;
static ID curNextEntityID = 0;

// particles
#define MAX_PARTICLES 1000
#define PARTICLE_RADIUS 17.0
static Particle particles[MAX_PARTICLES];
static int curParticleIndex = 0;

void SpawnParticle(Particle p)
{
    int newParticleIndex = (curParticleIndex + 1) % MAX_PARTICLES;
    particles[newParticleIndex] = p;
    curParticleIndex = newParticleIndex;
}

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
void LoadEntities(const char *path, bool setSpawnPoint)
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

    if (setSpawnPoint)
    {
        spawnPoint = GetPlayerEntity()->player.k.pos;
    }

    GetPlayerEntity()->player.k.pos = spawnPoint;

    camera.target = GetPlayerEntity()->player.k.pos;

    // for debugging stuff, also put any programmatic level modifications here
    for (int i = 0; i < entitiesLen; i++)
    {
        printf("%s %d\n", TypeNames[entities[i].type], entities[i].id);
    }
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

#include <stdlib.h>
float RandFloat(float min, float max)
{
    return ((max - min) * ((float)rand() / RAND_MAX)) + min;
}

Color ColorLerp(Color from, Color to, float factor)
{
    return (Color){
        .r = (unsigned char)Lerp((float)from.r, (float)to.r, factor),
        .g = (unsigned char)Lerp((float)from.b, (float)to.g, factor),
        .b = (unsigned char)Lerp((float)from.g, (float)to.b, factor),
        .a = (unsigned char)Lerp((float)from.a, (float)to.a, factor),
    };
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
    if (FileExists(level_name))
    {
        LoadEntities(level_name, true);
    }
    else
    {
        entities[0] = (Entity){
            .id = 0,
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
            .id = 1,
            .type = Obstacle,
            .obstacle = {
                .x = 100,
                .y = 400,
                .width = 100,
                .height = 200,
            },
        };
        curNextEntityID = 2;
        entitiesLen = 2;
        spawnPoint = GetPlayerEntity()->player.k.pos;
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

KinematicInfo GlideAndBounce(KinematicInfo k, float bounceFactor)
{
    k.onGround = false;
    for (int i = 0; i < entitiesLen; i++)
    {
        if (entities[i].type == Obstacle)
        {
            Rectangle obstacle = FixNegativeRect(entities[i].obstacle);
            Vector2 normal = {0};
            bool bounced = false;
            {
                Vector2 obstacleCenter = {.x = obstacle.x + obstacle.width / 2.0f, .y = obstacle.y + obstacle.height / 2.0f};
                Vector2 fromObstacleCenter = Vector2Subtract(k.pos, obstacleCenter);
                fromObstacleCenter.x = clamp(fromObstacleCenter.x, -obstacle.width / 2.0f, obstacle.width / 2.0f);
                fromObstacleCenter.y = clamp(fromObstacleCenter.y, -obstacle.height / 2.0f, obstacle.height / 2.0f);
                Vector2 closestPointOnObstacle = Vector2Add(fromObstacleCenter, obstacleCenter);
                if (Vector2Distance(closestPointOnObstacle, k.pos) < player_radius)
                {
                    bounced = true;
                    normal = Vector2Normalize(Vector2Subtract(k.pos, closestPointOnObstacle));
                    k.pos = Vector2Add(closestPointOnObstacle, Vector2Scale(normal, player_radius));
                }
            }
            if (bounced)
                k.vel = Vector2Scale(Vector2Reflect(k.vel, normal), bounceFactor);
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
        bool onGroundBefore = e->player.k.onGround;
        e->player.k = GlideAndBounce(e->player.k, 1.0f);
        if (!onGroundBefore && e->player.k.onGround)
        {
            spawnPoint = e->player.k.pos;
        }
        if (e->player.k.onGround)
            e->player.k.vel = Vector2Lerp(e->player.k.vel, Vector2Scale(movement, 400.0f), delta * 9.0f);
        e->player.k.pos = Vector2Add(e->player.k.pos, Vector2Scale(e->player.k.vel, delta));

        bool inFire = false;
        float fireLeft = 0.0;
        for(int i = 0; i < entitiesLen; i++) {
            if(entities[i].type == Fire && RectHasPoint(entities[i].fire.rect, e->player.k.pos)) {
                inFire = true;
                fireLeft = entities[i].fire.fireLeft;
                break;
            }
        }

        if(inFire) {
            e->player.health -= Lerp(GetFrameTime() / 0.5, GetFrameTime() / 2.5, 1.0 - fireLeft);
        } else if (!e->player.k.onGround)
        {
            e->player.health -= GetFrameTime() / 3.0;
        }
        else
        {
            e->player.health += GetFrameTime() / 0.5;
        }

        e->player.health = clamp(e->player.health, 0.0, 1.0);

        if (e->player.grabbedEntity == -1)
        {
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
            {
                for (int i = 0; i < entitiesLen; i++)
                {
                    if (entities[i].type == Extinguisher && Vector2Distance(e->player.k.pos, entities[i].extinguisher.info.pos) < player_grab_radius)
                    {
                        e->player.grabbedEntity = entities[i].id;
                        break;
                    }
                }
            }
        }
        else
        {
            GetEntity(e->player.grabbedEntity)->extinguisher.info.pos = e->player.k.pos;
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
            {
                Vector2 extraVelocity = Vector2Scale(Vector2Normalize(Vector2Subtract(WorldMousePos(), e->player.k.pos)), 250.0);
                GetEntity(e->player.grabbedEntity)->extinguisher.info.vel = Vector2Add(e->player.k.vel, extraVelocity);
                e->player.k.vel = Vector2Add(e->player.k.vel, Vector2Scale(extraVelocity, -1.0));
                e->player.grabbedEntity = -1;
            }
        }

        break;
    }
    case Extinguisher:
    {
        if (GetPlayerEntity()->player.grabbedEntity != e->id)
        {
            e->extinguisher.info = GlideAndBounce(e->extinguisher.info, 0.5f);
            if (e->extinguisher.info.onGround)
                e->extinguisher.info.vel = Vector2Lerp(e->extinguisher.info.vel, (Vector2){0}, GetFrameTime() * 4.0f);
            e->extinguisher.info.pos = Vector2Add(e->extinguisher.info.pos, Vector2Scale(e->extinguisher.info.vel, GetFrameTime()));
            break;
        }
        else
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                Vector2 toMouse = Vector2Subtract(WorldMousePos(), e->extinguisher.info.pos);
                Vector2 solidVelocity = Vector2Scale(Vector2Normalize(toMouse), 200.0);

                GetPlayerEntity()->player.k.vel = Vector2Add(GetPlayerEntity()->player.k.vel, Vector2Scale(toMouse, -GetFrameTime() * 3.0));
                SpawnParticle((Particle){
                    .pos = e->extinguisher.info.pos,
                    .vel = Vector2Rotate(solidVelocity, (float)GetRandomValue(-50, 50) / 100.0),
                    .color = (Color){255, 255, 255, 255},
                    .lifetime = 3.0,
                    .max_lifetime = 3.0,
                    .type = RetardantParticle,
                });
            }
        }
        break;
    }
    case Fire:
    {
        e->fire.fireParticleTimer += GetFrameTime();
        e->fire.fireLeft = clamp(e->fire.fireLeft, 0.0, 1.0);
        printf("%f\n", e->fire.fireLeft);
        if (e->fire.fireParticleTimer > Lerp(0.05, 0.5, 1.0 - e->fire.fireLeft))
        {
            SpawnParticle((Particle){
                .pos = (Vector2){
                    .x = RandFloat(e->fire.rect.x, e->fire.rect.x + e->fire.rect.width),
                    .y = RandFloat(e->fire.rect.y, e->fire.rect.y + e->fire.rect.height),
                },
                .vel = Vector2Rotate((Vector2){.x = 20.0, .y = 0.0}, RandFloat(-2.0 * PI, 2.0 * PI)),
                .color = (Color){255, 0, 0, 255},
                .lifetime = 4.0,
                .max_lifetime = 10.0,
                .type = FireParticle,
            });
            e->fire.fireParticleTimer = 0.0;
        }
        break;
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
        DrawRectanglePro(FixNegativeRect(e.ground), (Vector2){0}, 0.0, GREEN);
        break;
    }
    case Fire:
    {
        DrawRectanglePro(FixNegativeRect(e.fire.rect), (Vector2){0}, 0.0,ColorLerp((Color){230, 41, 55, 50}, (Color){50, 41, 255, 80}, 1.0 - e.fire.fireLeft));
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
    frameID += 1;
    if (IsKeyPressed(KEY_TAB))
        editing = !editing;

    if (IsKeyPressed(KEY_R))
        LoadEntities(level_name, false);

    for (int i = 0; i < entitiesLen; i++)
    {
        ProcessEntity(&entities[i]);
    }

    // worried about calling load entities from within the entity processing loop
    // so I put it here
    if (GetPlayerEntity()->player.health <= 0.0)
    {
        LoadEntities(level_name, false);
        GetPlayerEntity()->player.health = 1.0;
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
            SaveEntities(level_name);
            LoadEntities(level_name, false);
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
                if(currentType == Fire) {
                    toAdd.fire.fireLeft = 1.0;
                }
                if (currentType == Ground || currentType == Obstacle || currentType == Fire)
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

        if (currentEntity != NULL && (currentEntity->type == Ground || currentEntity->type == Obstacle || currentEntity->type == Fire))
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
                case Fire:
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

    // process particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].lifetime <= 0.0)
        {
            continue;
        }
        for (int ii = 0; ii < entitiesLen; ii++)
        {
            if (entities[ii].type == Obstacle && RectHasPoint(entities[ii].obstacle, particles[i].pos))
            {
                particles[i].vel = (Vector2){0};
                break;
            } else if(particles[i].type == RetardantParticle && entities[ii].type == Fire && RectHasPoint(entities[ii].fire.rect, particles[i].pos)) {
                particles[i].vel = (Vector2){0};
                particles[i].lifetime /= 2.0;
                entities[ii].fire.fireLeft -= 0.001;
                entities[ii].fire.fireLeft = clamp(entities[ii].fire.fireLeft, 0.0, 1.0);
            }
        }
        particles[i].lifetime -= GetFrameTime();
        particles[i].pos = Vector2Add(particles[i].pos, Vector2Scale(particles[i].vel, GetFrameTime()));
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // background color not moved by camera
    Color bg = ColorLerp((Color){17, 17, 17, 255}, (Color){205, 50, 75, 255}, 1.0 - GetPlayerEntity()->player.health);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), bg);

    BeginMode2D(camera);

    // draw entities
    for (int i = 0; i < entitiesLen; i++)
    {
        // other stuff on top
        if (entities[i].type == Player)
            continue;
        if (entities[i].type == Extinguisher)
            continue;
        DrawEntity(entities[i]);
    }
    for (int i = 0; i < entitiesLen; i++)
    {
        if (entities[i].type != Extinguisher)
            continue;
        DrawEntity(entities[i]);
    }

    DrawEntity(*GetPlayerEntity());

    // particles
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].lifetime <= 0.0)
            continue;
        Color toDraw = particles[i].color;
        toDraw.a = (unsigned char)((particles[i].lifetime / particles[i].max_lifetime) * 255);
        DrawCircleV(particles[i].pos, PARTICLE_RADIUS, toDraw);
    }

    EndMode2D();

    if (editing)
    {
        DrawText("Editing Mode\nScroll to change target\nClick to place\nRight click to delete\nMiddle click to teleport\nIt saves in browser storage or something idk", 0, 0, 16, RED);
        DrawText(TypeNames[currentType], 200, 0, 16, RED);
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