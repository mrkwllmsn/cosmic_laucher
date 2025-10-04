#pragma once

#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <vector>
#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

// Simple 3D raycasting game inspired by Doom
class DoomGame : public GameBase {
private:
    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 32;
    static constexpr int MAP_SIZE = 8;
    static constexpr float TILE_SIZE = 64.0f;
    static constexpr int RAY_COUNT = 32;  // One ray per column
    static constexpr float FOV = 1.047f;  // 60 degrees in radians
    static constexpr float PLAYER_SPEED = 1.5f;
    static constexpr float ROTATION_SPEED = 0.03f;

    // Map layout (1 = wall, 0 = empty)
    int map[MAP_SIZE][MAP_SIZE] = {
        {1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,1},
        {1,0,1,0,0,1,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,0,1,1,0,0,1},
        {1,0,0,0,0,0,0,1},
        {1,0,1,0,0,1,0,1},
        {1,1,1,1,1,1,1,1}
    };

    // Player state
    float playerX = TILE_SIZE * 1.5f;
    float playerY = TILE_SIZE * 1.5f;
    float playerAngle = 0.0f;

    // AI state for auto-play
    uint32_t moveTimer = 0;
    uint32_t rotateTimer = 0;
    uint32_t shootTimer = 0;
    float targetAngle = 0.0f;
    bool isRotating = false;

    // Bullet system
    struct Bullet {
        float x, y;
        float angle;
        float distance;
        bool active;
        uint32_t spawnTime;
    };
    static constexpr int MAX_BULLETS = 3;
    Bullet bullets[MAX_BULLETS];
    int muzzleFlash = 0;  // Muzzle flash animation counter
    int gunRecoil = 0;    // Gun recoil animation counter

    // Enemy system
    struct Enemy {
        float x, y;
        bool active;
        int health;
        uint32_t animFrame;
        uint32_t lastAnimTime;
    };
    static constexpr int MAX_ENEMIES = 5;
    Enemy enemies[MAX_ENEMIES];
    uint32_t lastSpawnTime = 0;
    static constexpr uint32_t SPAWN_INTERVAL = 3000;  // 3 seconds

    // Particle system for explosions
    struct Particle {
        float x, y;
        float vx, vy;
        uint8_t r, g, b;
        uint32_t lifetime;
        uint32_t spawnTime;
        bool active;
    };
    static constexpr int MAX_PARTICLES = 30;
    Particle particles[MAX_PARTICLES];

    struct Ray {
        float distance;
        bool isVertical;
        int x, y;  // Hit position
    };

    // Cast a single ray
    Ray castRay(float angle) {
        float sinA = sinf(angle);
        float cosA = cosf(angle);

        float maxDepth = 500.0f;
        float stepSize = 2.0f;

        for (float depth = 0; depth < maxDepth; depth += stepSize) {
            float rayX = playerX + cosA * depth;
            float rayY = playerY + sinA * depth;

            int gridX = (int)(rayX / TILE_SIZE);
            int gridY = (int)(rayY / TILE_SIZE);

            if (gridY < 0 || gridY >= MAP_SIZE || gridX < 0 || gridX >= MAP_SIZE) {
                return {maxDepth, false, 0, 0};
            }

            if (map[gridY][gridX] == 1) {
                // Determine if vertical or horizontal wall
                float dx = fabsf(fmodf(rayX, TILE_SIZE));
                bool isVert = dx < 5.0f || dx > TILE_SIZE - 5.0f;

                return {depth, isVert, (int)rayX, (int)rayY};
            }
        }

        return {maxDepth, false, 0, 0};
    }

    // Check if there's a wall between two points
    bool hasWallBetween(float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dist = sqrtf(dx * dx + dy * dy);

        float stepSize = 2.0f;
        int steps = (int)(dist / stepSize);
        if (steps == 0) return false;

        for (int i = 0; i <= steps; i++) {
            float t = (float)i / steps;
            float checkX = x1 + dx * t;
            float checkY = y1 + dy * t;

            int gridX = (int)(checkX / TILE_SIZE);
            int gridY = (int)(checkY / TILE_SIZE);

            if (gridY < 0 || gridY >= MAP_SIZE || gridX < 0 || gridX >= MAP_SIZE) {
                return true;
            }

            if (map[gridY][gridX] == 1) {
                return true;
            }
        }

        return false;
    }

    // Check if position is valid (not in wall)
    bool isValidPosition(float x, float y) {
        int gridX = (int)(x / TILE_SIZE);
        int gridY = (int)(y / TILE_SIZE);

        if (gridY < 0 || gridY >= MAP_SIZE || gridX < 0 || gridX >= MAP_SIZE) {
            return false;
        }

        return map[gridY][gridX] == 0;
    }

    // Shoot a bullet
    void shoot() {
        // Find an inactive bullet slot
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (!bullets[i].active) {
                bullets[i].x = playerX;
                bullets[i].y = playerY;
                bullets[i].angle = playerAngle;
                bullets[i].distance = 0;
                bullets[i].active = true;
                bullets[i].spawnTime = time_us_32() / 1000;
                muzzleFlash = 8;
                gunRecoil = 4;
                break;
            }
        }
    }

    // Update bullets
    void updateBullets() {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Move bullet forward
                bullets[i].distance += 5.0f;
                bullets[i].x += cosf(bullets[i].angle) * 5.0f;
                bullets[i].y += sinf(bullets[i].angle) * 5.0f;

                // Check if bullet hit a wall or traveled too far
                int gridX = (int)(bullets[i].x / TILE_SIZE);
                int gridY = (int)(bullets[i].y / TILE_SIZE);

                if (gridY < 0 || gridY >= MAP_SIZE || gridX < 0 || gridX >= MAP_SIZE ||
                    map[gridY][gridX] == 1 || bullets[i].distance > 300.0f) {
                    bullets[i].active = false;
                }
            }
        }

        // Update animations
        if (muzzleFlash > 0) muzzleFlash--;
        if (gunRecoil > 0) gunRecoil--;
    }

    // AI movement logic
    void updateAI() {
        uint32_t currentTime = time_us_32() / 1000;  // Convert to ms

        // Find the closest visible enemy anywhere on the map
        float closestEnemyDist = 10000.0f;
        float closestEnemyAngle = 0.0f;
        bool foundEnemy = false;

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                float dx = enemies[i].x - playerX;
                float dy = enemies[i].y - playerY;
                float dist = sqrtf(dx * dx + dy * dy);

                // Check if enemy is visible (no wall between) - hunt ANY visible enemy
                if (!hasWallBetween(playerX, playerY, enemies[i].x, enemies[i].y)) {
                    if (dist < closestEnemyDist) {
                        closestEnemyDist = dist;
                        closestEnemyAngle = atan2f(dy, dx);
                        foundEnemy = true;
                    }
                }
            }
        }

        // Rotation logic
        if (isRotating) {
            float angleDiff = targetAngle - playerAngle;

            // Normalize angle difference
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;

            if (fabsf(angleDiff) < 0.1f) {
                playerAngle = targetAngle;
                isRotating = false;
            } else {
                // Rotate toward target
                playerAngle += (angleDiff > 0 ? 1 : -1) * ROTATION_SPEED * 4.0f;
            }
        }

        // Movement logic - ALWAYS try to move forward
        if (currentTime - moveTimer > 50) {  // Faster movement updates
            // Check if we can move forward
            float lookAheadDist = PLAYER_SPEED * 10.0f;
            float lookX = playerX + cosf(playerAngle) * lookAheadDist;
            float lookY = playerY + sinf(playerAngle) * lookAheadDist;

            bool pathBlocked = !isValidPosition(lookX, lookY);

            // If path is blocked, find a new direction
            if (pathBlocked) {
                // Try to find an open direction
                float testAngleRight = playerAngle + 1.57f;
                float testXRight = playerX + cosf(testAngleRight) * lookAheadDist;
                float testYRight = playerY + sinf(testAngleRight) * lookAheadDist;

                float testAngleLeft = playerAngle - 1.57f;
                float testXLeft = playerX + cosf(testAngleLeft) * lookAheadDist;
                float testYLeft = playerY + sinf(testAngleLeft) * lookAheadDist;

                // Choose direction with most space
                if (isValidPosition(testXRight, testYRight)) {
                    targetAngle = testAngleRight;
                    isRotating = true;
                } else if (isValidPosition(testXLeft, testYLeft)) {
                    targetAngle = testAngleLeft;
                    isRotating = true;
                } else {
                    // Both blocked, turn around completely
                    targetAngle = playerAngle + 3.14159f;
                    isRotating = true;
                }
            }
            // If we found an enemy and can see it, hunt it!
            else if (foundEnemy) {
                float angleDiff = closestEnemyAngle - playerAngle;

                // Normalize angle difference
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // If enemy is roughly in front, move toward it and shoot
                if (fabsf(angleDiff) < 0.4f) {
                    // Move toward enemy
                    float newX = playerX + cosf(playerAngle) * PLAYER_SPEED;
                    float newY = playerY + sinf(playerAngle) * PLAYER_SPEED;

                    if (isValidPosition(newX, newY)) {
                        playerX = newX;
                        playerY = newY;
                    }

                    // Shoot at enemy
                    if (currentTime - shootTimer > (uint32_t)(300 + (rand() % 500))) {
                        shoot();
                        shootTimer = currentTime;
                    }
                } else if (!isRotating) {
                    // Turn to face enemy
                    targetAngle = closestEnemyAngle;
                    isRotating = true;
                }
            }
            // No enemy visible, just explore
            else {
                // Keep moving forward
                float newX = playerX + cosf(playerAngle) * PLAYER_SPEED;
                float newY = playerY + sinf(playerAngle) * PLAYER_SPEED;

                if (isValidPosition(newX, newY)) {
                    playerX = newX;
                    playerY = newY;
                }

                // Random shooting while exploring
                if (currentTime - shootTimer > (uint32_t)(1500 + (rand() % 2000))) {
                    shoot();
                    shootTimer = currentTime;
                }

                // Occasionally pick a new random direction
                if (!isRotating && currentTime - rotateTimer > (uint32_t)(2000 + (rand() % 2000))) {
                    targetAngle = ((rand() % 360) * 3.14159f) / 180.0f;
                    isRotating = true;
                    rotateTimer = currentTime;
                }
            }

            moveTimer = currentTime;
        }
    }

    // Spawn an enemy at a random location away from player
    void spawnEnemy() {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) {
                // Find a valid spawn location away from player
                int attempts = 0;
                while (attempts < 20) {
                    int gridX = 1 + (rand() % (MAP_SIZE - 2));
                    int gridY = 1 + (rand() % (MAP_SIZE - 2));

                    float spawnX = gridX * TILE_SIZE + TILE_SIZE / 2;
                    float spawnY = gridY * TILE_SIZE + TILE_SIZE / 2;

                    float dx = spawnX - playerX;
                    float dy = spawnY - playerY;
                    float dist = sqrtf(dx * dx + dy * dy);

                    // Must be in empty space and far from player
                    if (map[gridY][gridX] == 0 && dist > TILE_SIZE * 3) {
                        enemies[i].x = spawnX;
                        enemies[i].y = spawnY;
                        enemies[i].active = true;
                        enemies[i].health = 2;
                        enemies[i].animFrame = 0;
                        enemies[i].lastAnimTime = time_us_32() / 1000;
                        break;
                    }
                    attempts++;
                }
                break;
            }
        }
    }

    // Update enemies - move toward player
    void updateEnemies() {
        uint32_t currentTime = time_us_32() / 1000;

        // Spawn enemies periodically
        if (currentTime - lastSpawnTime > SPAWN_INTERVAL) {
            spawnEnemy();
            lastSpawnTime = currentTime;
        }

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                // Update animation
                if (currentTime - enemies[i].lastAnimTime > 200) {
                    enemies[i].animFrame = (enemies[i].animFrame + 1) % 4;
                    enemies[i].lastAnimTime = currentTime;
                }

                // Move toward player
                float dx = playerX - enemies[i].x;
                float dy = playerY - enemies[i].y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist > 10.0f) {
                    float moveSpeed = 0.3f;
                    float newX = enemies[i].x + (dx / dist) * moveSpeed;
                    float newY = enemies[i].y + (dy / dist) * moveSpeed;

                    // Only move if position is valid
                    if (isValidPosition(newX, newY)) {
                        enemies[i].x = newX;
                        enemies[i].y = newY;
                    }
                }
            }
        }
    }

    // Check bullet-enemy collisions
    void checkCollisions() {
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) continue;

            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;

                float dx = bullets[b].x - enemies[e].x;
                float dy = bullets[b].y - enemies[e].y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < 15.0f) {  // Hit!
                    bullets[b].active = false;
                    enemies[e].health--;

                    if (enemies[e].health <= 0) {
                        // Enemy dies - create explosion
                        spawnExplosion(enemies[e].x, enemies[e].y);
                        enemies[e].active = false;
                    }
                    break;
                }
            }
        }
    }

    // Create particle explosion
    void spawnExplosion(float x, float y) {
        uint32_t currentTime = time_us_32() / 1000;

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                // Create particle with random velocity
                float angle = (rand() % 360) * 0.0174533f;
                float speed = 0.5f + (rand() % 100) / 50.0f;

                particles[i].x = x;
                particles[i].y = y;
                particles[i].vx = cosf(angle) * speed;
                particles[i].vy = sinf(angle) * speed;

                // Random colors - red, orange, yellow
                int colorChoice = rand() % 3;
                if (colorChoice == 0) {
                    particles[i].r = 255; particles[i].g = 0; particles[i].b = 0;
                } else if (colorChoice == 1) {
                    particles[i].r = 255; particles[i].g = 128; particles[i].b = 0;
                } else {
                    particles[i].r = 255; particles[i].g = 255; particles[i].b = 0;
                }

                particles[i].lifetime = 500 + (rand() % 500);
                particles[i].spawnTime = currentTime;
                particles[i].active = true;

                // Only create about 15-20 particles per explosion
                static int particleCount = 0;
                particleCount++;
                if (particleCount >= 18) {
                    particleCount = 0;
                    break;
                }
            }
        }
    }

    // Update particles
    void updateParticles() {
        uint32_t currentTime = time_us_32() / 1000;

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                // Update position
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;

                // Apply slight drag
                particles[i].vx *= 0.95f;
                particles[i].vy *= 0.95f;

                // Check lifetime
                if (currentTime - particles[i].spawnTime > particles[i].lifetime) {
                    particles[i].active = false;
                }
            }
        }
    }

    void drawSkyAndFloor(PicoGraphics_PenRGB888& gfx) {
        // Sky (top half) - dark red to blue gradient
        for (int y = 0; y < HEIGHT / 2; y++) {
            int shade = (y * 255) / (HEIGHT / 2);
            Pen skyPen = gfx.create_pen(shade / 3, 0, 100 + shade / 2);
            gfx.set_pen(skyPen);
            gfx.line(Point(0, y), Point(WIDTH - 1, y));
        }

        // Floor (bottom half) - dark gradient
        for (int y = HEIGHT / 2; y < HEIGHT; y++) {
            int shade = ((y - HEIGHT / 2) * 80) / (HEIGHT / 2);
            Pen floorPen = gfx.create_pen(shade / 4, shade / 3, shade / 2);
            gfx.set_pen(floorPen);
            gfx.line(Point(0, y), Point(WIDTH - 1, y));
        }
    }

    void drawWalls(PicoGraphics_PenRGB888& gfx) {
        float startAngle = playerAngle - FOV / 2.0f;
        float angleStep = FOV / RAY_COUNT;

        for (int i = 0; i < RAY_COUNT; i++) {
            float rayAngle = startAngle + i * angleStep;
            Ray ray = castRay(rayAngle);

            // Fisheye correction
            float correctedDist = ray.distance * cosf(rayAngle - playerAngle);

            // Calculate wall height based on distance
            if (correctedDist < 1.0f) correctedDist = 1.0f;
            int wallHeight = (int)((HEIGHT * 20.0f) / correctedDist);
            if (wallHeight > HEIGHT * 2) wallHeight = HEIGHT * 2;

            int wallTop = (HEIGHT - wallHeight) / 2;
            int wallBottom = wallTop + wallHeight;

            if (wallTop < 0) wallTop = 0;
            if (wallBottom > HEIGHT) wallBottom = HEIGHT;

            // Calculate shading based on distance - more aggressive falloff
            int shade = 255 - (int)((correctedDist / 250.0f) * 235.0f);
            if (shade < 10) shade = 10;
            if (shade > 255) shade = 255;

            // Darker for vertical walls (Doom-style)
            if (ray.isVertical) {
                shade = shade * 2 / 3;
            }

            // Create wall color with distance-based darkening
            int r = 0;
            int g = shade / 4;  // More subtle green
            int b = shade;       // Dominant blue

            Pen wallPen = gfx.create_pen(r, g, b);
            gfx.set_pen(wallPen);

            // Draw wall column
            gfx.line(Point(i, wallTop), Point(i, wallBottom));
        }
    }

    // Draw bullets in 3D view
    void drawBullets(PicoGraphics_PenRGB888& gfx) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Calculate bullet position relative to player
                float dx = bullets[i].x - playerX;
                float dy = bullets[i].y - playerY;
                float bulletDist = sqrtf(dx * dx + dy * dy);

                // Calculate angle relative to player view
                float bulletAngle = atan2f(dy, dx);
                float angleDiff = bulletAngle - playerAngle;

                // Normalize angle
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // Only draw if bullet is in front of player and within FOV
                if (bulletDist > 1.0f && fabsf(angleDiff) < FOV / 2.0f) {
                    // Calculate screen X position
                    int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));

                    // Calculate screen Y position based on distance
                    int bulletSize = (int)(100.0f / bulletDist);
                    if (bulletSize < 1) bulletSize = 1;
                    if (bulletSize > 4) bulletSize = 4;

                    int screenY = HEIGHT / 2;

                    // Draw bullet as a bright yellow/orange projectile
                    Pen bulletPen = gfx.create_pen(255, 200, 0);
                    gfx.set_pen(bulletPen);

                    for (int py = -bulletSize; py <= bulletSize; py++) {
                        for (int px = -bulletSize; px <= bulletSize; px++) {
                            int x = screenX + px;
                            int y = screenY + py;
                            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                                gfx.pixel(Point(x, y));
                            }
                        }
                    }
                }
            }
        }
    }

    // Draw enemies in 3D view with sprites
    void drawEnemies(PicoGraphics_PenRGB888& gfx) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                // Calculate enemy position relative to player
                float dx = enemies[i].x - playerX;
                float dy = enemies[i].y - playerY;
                float enemyDist = sqrtf(dx * dx + dy * dy);

                // Calculate angle relative to player view
                float enemyAngle = atan2f(dy, dx);
                float angleDiff = enemyAngle - playerAngle;

                // Normalize angle
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // Only draw if enemy is in front of player, within FOV, and not blocked by a wall
                if (enemyDist > 5.0f && fabsf(angleDiff) < FOV / 2.0f + 0.5f) {
                    // Check if there's a wall between player and enemy
                    if (hasWallBetween(playerX, playerY, enemies[i].x, enemies[i].y)) {
                        continue;  // Skip this enemy, it's behind a wall
                    }

                    // Calculate screen X position
                    int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));

                    // Calculate size based on distance - but don't make too small
                    int spriteSize = (int)(400.0f / enemyDist);
                    if (spriteSize < 4) spriteSize = 4;   // Minimum size for detail
                    if (spriteSize > 20) spriteSize = 20;

                    int screenY = HEIGHT / 2;

                    // Draw enemy sprite - floating skull/demon design
                    // Animation affects the "pulsing" of the sprite
                    int pulse = (enemies[i].animFrame % 2 == 0) ? 0 : 1;

                    // Body color - dark red/purple demon
                    uint8_t bodyR = 150 + pulse * 20;
                    uint8_t bodyG = 0;
                    uint8_t bodyB = 50 + pulse * 30;

                    // Eyes - bright glowing effect
                    uint8_t eyeR = 255;
                    uint8_t eyeG = 50 + pulse * 100;
                    uint8_t eyeB = 0;

                    Pen bodyPen = gfx.create_pen(bodyR, bodyG, bodyB);
                    Pen eyePen = gfx.create_pen(eyeR, eyeG, eyeB);
                    Pen darkPen = gfx.create_pen(bodyR / 2, 0, bodyB / 2);

                    // Draw body (rounded shape)
                    gfx.set_pen(bodyPen);
                    for (int py = -spriteSize/2; py <= spriteSize/2; py++) {
                        for (int px = -spriteSize/2; px <= spriteSize/2; px++) {
                            // Circular body shape
                            if (px*px + py*py <= (spriteSize/2)*(spriteSize/2)) {
                                int x = screenX + px;
                                int y = screenY + py;
                                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                                    gfx.pixel(Point(x, y));
                                }
                            }
                        }
                    }

                    // Draw eyes (always visible, with detail even when small)
                    gfx.set_pen(eyePen);
                    int eyeOffset = spriteSize / 4;
                    if (eyeOffset < 1) eyeOffset = 1;
                    int eyeY = screenY - spriteSize / 6;

                    // Left eye
                    int leftEyeX = screenX - eyeOffset;
                    gfx.pixel(Point(leftEyeX, eyeY));
                    if (spriteSize > 6) {
                        gfx.pixel(Point(leftEyeX - 1, eyeY));
                        gfx.pixel(Point(leftEyeX, eyeY - 1));
                    }

                    // Right eye
                    int rightEyeX = screenX + eyeOffset;
                    gfx.pixel(Point(rightEyeX, eyeY));
                    if (spriteSize > 6) {
                        gfx.pixel(Point(rightEyeX + 1, eyeY));
                        gfx.pixel(Point(rightEyeX, eyeY - 1));
                    }

                    // Mouth/teeth (if big enough)
                    if (spriteSize > 8) {
                        gfx.set_pen(darkPen);
                        int mouthY = screenY + spriteSize / 4;
                        for (int mx = -spriteSize/5; mx <= spriteSize/5; mx++) {
                            int x = screenX + mx;
                            if (x >= 0 && x < WIDTH && mouthY >= 0 && mouthY < HEIGHT) {
                                gfx.pixel(Point(x, mouthY));
                            }
                        }
                    }

                    // Horns/spikes on top (if big enough)
                    if (spriteSize > 10) {
                        gfx.set_pen(bodyPen);
                        int hornY = screenY - spriteSize/2 - 1;
                        gfx.pixel(Point(screenX - spriteSize/3, hornY));
                        gfx.pixel(Point(screenX + spriteSize/3, hornY));
                        if (spriteSize > 14) {
                            gfx.pixel(Point(screenX - spriteSize/3, hornY - 1));
                            gfx.pixel(Point(screenX + spriteSize/3, hornY - 1));
                        }
                    }
                }
            }
        }
    }

    // Draw particles in 3D view
    void drawParticles(PicoGraphics_PenRGB888& gfx) {
        uint32_t currentTime = time_us_32() / 1000;

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                // Calculate particle position relative to player
                float dx = particles[i].x - playerX;
                float dy = particles[i].y - playerY;
                float particleDist = sqrtf(dx * dx + dy * dy);

                // Calculate angle relative to player view
                float particleAngle = atan2f(dy, dx);
                float angleDiff = particleAngle - playerAngle;

                // Normalize angle
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // Only draw if particle is in front of player and within FOV
                if (particleDist > 1.0f && fabsf(angleDiff) < FOV / 2.0f + 0.5f) {
                    // Calculate screen X position
                    int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));

                    // Calculate screen Y position based on distance (with slight upward float)
                    float ageRatio = (float)(currentTime - particles[i].spawnTime) / particles[i].lifetime;
                    int screenY = HEIGHT / 2 - (int)(ageRatio * 10.0f);  // Rise up as they age

                    // Fade out over time
                    uint8_t alpha = (uint8_t)(255 * (1.0f - ageRatio));
                    uint8_t r = (particles[i].r * alpha) / 255;
                    uint8_t g = (particles[i].g * alpha) / 255;
                    uint8_t b = (particles[i].b * alpha) / 255;

                    Pen particlePen = gfx.create_pen(r, g, b);
                    gfx.set_pen(particlePen);

                    // Draw particle
                    if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
                        gfx.pixel(Point(screenX, screenY));

                        // Larger particles when close
                        if (particleDist < 100.0f) {
                            if (screenX > 0) gfx.pixel(Point(screenX - 1, screenY));
                            if (screenX < WIDTH - 1) gfx.pixel(Point(screenX + 1, screenY));
                        }
                    }
                }
            }
        }
    }

    void drawHUD(PicoGraphics_PenRGB888& gfx) {
        // Crosshair - proper + shape with center pixel missing
        Pen crosshairPen = gfx.create_pen(200, 200, 200);
        gfx.set_pen(crosshairPen);
        int centerX = WIDTH / 2;
        int centerY = HEIGHT / 2;

        // Horizontal line (left and right of center)
        gfx.pixel(Point(centerX - 2, centerY));
        gfx.pixel(Point(centerX - 1, centerY));
        gfx.pixel(Point(centerX + 1, centerY));
        gfx.pixel(Point(centerX + 2, centerY));

        // Vertical line (top and bottom of center)
        gfx.pixel(Point(centerX, centerY - 2));
        gfx.pixel(Point(centerX, centerY - 1));
        gfx.pixel(Point(centerX, centerY + 1));
        gfx.pixel(Point(centerX, centerY + 2));

        // Draw gun sprite at bottom center
        int gunX = WIDTH / 2;
        int gunY = HEIGHT - 6 + (gunRecoil / 2);  // Recoil moves gun up

        Pen gunPen = gfx.create_pen(60, 60, 60);
        gfx.set_pen(gunPen);

        // Gun barrel (simple rectangular design)
        gfx.line(Point(gunX - 1, gunY), Point(gunX - 1, gunY + 3));
        gfx.line(Point(gunX, gunY), Point(gunX, gunY + 3));
        gfx.line(Point(gunX + 1, gunY), Point(gunX + 1, gunY + 3));

        // Gun body/grip
        gfx.pixel(Point(gunX - 2, gunY + 3));
        gfx.pixel(Point(gunX + 2, gunY + 3));
        gfx.pixel(Point(gunX - 2, gunY + 4));
        gfx.pixel(Point(gunX - 1, gunY + 4));
        gfx.pixel(Point(gunX, gunY + 4));
        gfx.pixel(Point(gunX + 1, gunY + 4));
        gfx.pixel(Point(gunX + 2, gunY + 4));

        // Fill gaps with black (row 4: center 3 pixels, bottom row: all 5 pixels)
        Pen blackPen = gfx.create_pen(0, 0, 0);
        gfx.set_pen(blackPen);

        // Row 4 (gunY + 4) - fill center 3 pixels where gun body doesn't reach
        // (gun body only has pixels at -2, -1, 0, +1, +2, but we need to fill any gaps)
        // Actually row 4 is already filled by gun body, so fill row 5 completely
        gfx.pixel(Point(gunX - 2, gunY + 5));
        gfx.pixel(Point(gunX - 1, gunY + 5));
        gfx.pixel(Point(gunX, gunY + 5));
        gfx.pixel(Point(gunX + 1, gunY + 5));
        gfx.pixel(Point(gunX + 2, gunY + 5));

        // Muzzle flash when shooting
        if (muzzleFlash > 0) {
            int flashBrightness = (muzzleFlash * 30);
            Pen flashPen = gfx.create_pen(255, 200 + flashBrightness / 5, 0);
            gfx.set_pen(flashPen);

            // Flash coming out of barrel
            int flashY = gunY - 1 - (8 - muzzleFlash);
            for (int fx = -2; fx <= 2; fx++) {
                for (int fy = 0; fy < 3; fy++) {
                    int x = gunX + fx;
                    int y = flashY - fy;
                    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                        if (abs(fx) + fy < 3 + muzzleFlash / 2) {
                            gfx.pixel(Point(x, y));
                        }
                    }
                }
            }
        }
    }

    bool shouldExit = false;

public:
    DoomGame() {
        srand(time_us_32());
    }

    const char* getName() const override { return "DOOM"; }
    const char* getDescription() const override { return "Retro 3D raycasting shooter"; }

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;

        // Reset player position
        playerX = TILE_SIZE * 1.5f;
        playerY = TILE_SIZE * 1.5f;
        playerAngle = 0.0f;
        moveTimer = time_us_32() / 1000;
        rotateTimer = moveTimer;
        shootTimer = moveTimer;
        shouldExit = false;

        // Initialize bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            bullets[i].active = false;
        }
        muzzleFlash = 0;
        gunRecoil = 0;

        // Initialize enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            enemies[i].active = false;
        }
        lastSpawnTime = time_us_32() / 1000;

        // Initialize particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles[i].active = false;
        }
    }

    bool update() override {
        updateAI();
        updateBullets();
        updateEnemies();
        updateParticles();
        checkCollisions();
        return !shouldExit;  // Return false to exit
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Draw from back to front (painter's algorithm)
        drawSkyAndFloor(graphics);
        drawWalls(graphics);
        drawParticles(graphics);  // Draw explosion particles
        drawEnemies(graphics);     // Draw enemies in 3D view
        drawBullets(graphics);     // Draw bullets in 3D view
        drawHUD(graphics);
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                     bool button_vol_up, bool button_vol_down,
                     bool button_bright_up, bool button_bright_down) override {
        // Only exit on B button to avoid interfering with game
        if (button_b) {
            shouldExit = true;
        }
    }
};
