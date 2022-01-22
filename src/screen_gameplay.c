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

const float player_radius = 18.0;

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
// static int framesCounter = 0;
static int finishScreen = 0;
static Vector2 position = { .x = 200, .y = 300 };
static Vector2 velocity = { 0 };
static Vector2 fireExtinguisherPosition = { 100, 100 };
static Texture fireExtinguisher;

static Rectangle obstacle = {
    .x = 400,
    .y = 400,
    .width = 100,
    .height = 200,
};
//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------


float clamp(float value, float min, float max) {
    if(value < min) {
        return min;
    }
    if(value > max) {
        return max;
    }
    return value;
}

// Project a onto b
Vector2 Vector2Project(Vector2 a, Vector2 b) {
    return Vector2Scale(Vector2Normalize(b), Vector2DotProduct(a, b)/Vector2Length(b));
}

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    fireExtinguisher = LoadTexture("resources/Fire Extinguisher.png");
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    float delta = GetFrameTime();
    Vector2 movement = {
        .x = (float)IsKeyDown(KEY_D) - (float)IsKeyDown(KEY_A),
        .y = (float)IsKeyDown(KEY_S) - (float)IsKeyDown(KEY_W),
    };

    movement = Vector2Normalize(movement);

    velocity = Vector2Lerp(velocity, Vector2Scale(movement, 400.0), delta*9.0);
    position = Vector2Add(position, Vector2Scale(velocity, delta));

    Vector2 normal = {0};
    Vector2 obstacleCenter = {.x = obstacle.x + obstacle.width/2.0, .y = obstacle.y + obstacle.height/2.0};
    Vector2 fromObstacleCenter = Vector2Subtract(position, obstacleCenter);
    fromObstacleCenter.x = clamp(fromObstacleCenter.x, -obstacle.width/2.0, obstacle.width/2.0);
    fromObstacleCenter.y = clamp(fromObstacleCenter.y, -obstacle.height/2.0, obstacle.height/2.0);
    Vector2 closestPointOnObstacle = Vector2Add(fromObstacleCenter, obstacleCenter);
    if(Vector2Distance(closestPointOnObstacle, position) < player_radius) {
        normal = Vector2Normalize(Vector2Subtract(position, closestPointOnObstacle));
        position = Vector2Add(closestPointOnObstacle, Vector2Scale(normal, player_radius));
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    DrawCircleV(position, player_radius, PINK);
    DrawRectanglePro(obstacle, (Vector2){0}, 0.0, GRAY);
    DrawTextureEx(fireExtinguisher, fireExtinguisherPosition, 0.0, 0.35, WHITE);
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