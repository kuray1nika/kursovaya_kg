#pragma once
#include <vector>

struct AABB {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    AABB(double x1, double y1, double z1, double x2, double y2, double z2)
        : minX(x1), minY(y1), minZ(z1), maxX(x2), maxY(y2), maxZ(z2) {}
};

class Game {
public:
    Game();

    void init();
    void update(double deltaTime);
    void checkCollisions();
    bool isColliding(const AABB& box);

    const std::vector<AABB>& getWalls() const { return walls; }

private:
    std::vector<AABB> walls;
    double playerRadius;
};