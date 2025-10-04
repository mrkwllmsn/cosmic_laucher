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

        // Shooting logic - shoot periodically
        if (currentTime - shootTimer > (uint32_t)(800 + (rand() % 1500))) {
            shoot();
            shootTimer = currentTime;
        }

        // Rotation logic - do this first
        if (isRotating) {
            float angleDiff = targetAngle - playerAngle;

            // Normalize angle difference
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;

            if (fabsf(angleDiff) < 0.1f) {
                playerAngle = targetAngle;
                isRotating = false;
            } else {
                // Faster rotation when avoiding walls
                playerAngle += (angleDiff > 0 ? 1 : -1) * ROTATION_SPEED * 3.0f;
            }
        }

        // Movement logic
        if (currentTime - moveTimer > 100) {
            // Look ahead to detect walls before hitting them
            float lookAheadDist = PLAYER_SPEED * 8.0f;
            float lookX = playerX + cosf(playerAngle) * lookAheadDist;
            float lookY = playerY + sinf(playerAngle) * lookAheadDist;

            // If we're about to hit a wall, turn immediately
            if (!isValidPosition(lookX, lookY) && !isRotating) {
                // Try turning right first
                float testAngleRight = playerAngle + 1.57f;
                float testXRight = playerX + cosf(testAngleRight) * lookAheadDist;
                float testYRight = playerY + sinf(testAngleRight) * lookAheadDist;

                // Try turning left
                float testAngleLeft = playerAngle - 1.57f;
                float testXLeft = playerX + cosf(testAngleLeft) * lookAheadDist;
                float testYLeft = playerY + sinf(testAngleLeft) * lookAheadDist;

                // Choose the direction that's more open
                if (isValidPosition(testXRight, testYRight)) {
                    targetAngle = testAngleRight;
                } else if (isValidPosition(testXLeft, testYLeft)) {
                    targetAngle = testAngleLeft;
                } else {
                    // Both blocked, turn around
                    targetAngle = playerAngle + 3.14159f;
                }
                isRotating = true;
            }

            // Try to move forward only if not rotating
            if (!isRotating) {
                float newX = playerX + cosf(playerAngle) * PLAYER_SPEED;
                float newY = playerY + sinf(playerAngle) * PLAYER_SPEED;

                if (isValidPosition(newX, newY)) {
                    playerX = newX;
                    playerY = newY;
                }
            }

            moveTimer = currentTime;
        }

        // Random rotation occasionally (when not already rotating)
        if (!isRotating && currentTime - rotateTimer > (uint32_t)(3000 + (rand() % 3000))) {
            targetAngle = ((rand() % 360) * 3.14159f) / 180.0f;
            isRotating = true;
            rotateTimer = currentTime;
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
    }

    bool update() override {
        updateAI();
        updateBullets();
        return !shouldExit;  // Return false to exit
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Draw from back to front
        drawSkyAndFloor(graphics);
        drawWalls(graphics);
        drawBullets(graphics);  // Draw bullets in 3D view
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
