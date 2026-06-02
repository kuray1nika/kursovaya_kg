#include "Game.h"
#include "Camera.h"
#include <algorithm>

extern Camera camera;

Game::Game() : playerRadius(0.4)
{
}

void Game::init()
{
    walls.clear();

    // Стены для стартовой локации (лес по краям)
    // Квадрат от -15 до 15 по X и Z
    // Северная стена (Z = -15)
    walls.push_back(AABB(-16, -2, -15.5, 16, 5, -14.5));
    // Южная стена (Z = 15)
    walls.push_back(AABB(-16, -2, 14.5, 16, 5, 15.5));
    // Западная стена (X = -15)
    walls.push_back(AABB(-15.5, -2, -16, -14.5, 5, 16));
    // Восточная стена (X = 15)
    walls.push_back(AABB(14.5, -2, -16, 15.5, 5, 16));
}

void Game::update(double deltaTime)
{
    // Пока пусто, добавим позже
}

void Game::checkCollisions()
{
    double px = camera.x();
    double py = camera.y();
    double pz = camera.z();

    for (size_t i = 0; i < walls.size(); ++i)
    {
        const AABB& wall = walls[i];

        // Проверка столкновения сферы игрока с AABB стены
        double closestX = wall.minX;
        if (px < wall.minX) closestX = wall.minX;
        else if (px > wall.maxX) closestX = wall.maxX;
        else closestX = px;

        double closestY = wall.minY;
        if (py < wall.minY) closestY = wall.minY;
        else if (py > wall.maxY) closestY = wall.maxY;
        else closestY = py;

        double closestZ = wall.minZ;
        if (pz < wall.minZ) closestZ = wall.minZ;
        else if (pz > wall.maxZ) closestZ = wall.maxZ;
        else closestZ = pz;

        double dx = px - closestX;
        double dy = py - closestY;
        double dz = pz - closestZ;

        double distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < playerRadius * playerRadius)
        {
            // Выталкиваем игрока
            double dist = sqrt(distSq);
            if (dist > 0.001)
            {
                camera.setPosition(
                    px + dx / dist * (playerRadius - dist),
                    py + dy / dist * (playerRadius - dist),
                    pz + dz / dist * (playerRadius - dist)
                );
            }
        }
    }
}

bool Game::isColliding(const AABB& box)
{
    double px = camera.x();
    double py = camera.y();
    double pz = camera.z();

    double closestX = box.minX;
    if (px < box.minX) closestX = box.minX;
    else if (px > box.maxX) closestX = box.maxX;
    else closestX = px;

    double closestY = box.minY;
    if (py < box.minY) closestY = box.minY;
    else if (py > box.maxY) closestY = box.maxY;
    else closestY = py;

    double closestZ = box.minZ;
    if (pz < box.minZ) closestZ = box.minZ;
    else if (pz > box.maxZ) closestZ = box.maxZ;
    else closestZ = pz;

    double dx = px - closestX;
    double dy = py - closestY;
    double dz = pz - closestZ;

    return (dx * dx + dy * dy + dz * dz) < playerRadius * playerRadius;
}