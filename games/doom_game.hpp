#pragma once

#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <vector>
#include "pico/stdlib.h"
#include "../game_base.hpp"
#include "../effects/lightning.hpp"

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
    int playerHealth = 100;
    static constexpr int MAX_HEALTH = 100;
    bool isDead = false;
    uint32_t deathTime = 0;

    // AI state for auto-play
    uint32_t moveTimer = 0;
    uint32_t rotateTimer = 0;
    uint32_t shootTimer = 0;
    float targetAngle = 0.0f;
    bool isRotating = false;

    // Weapon system
    int weaponType = 0;  // 0=single, 1=spread, 2=rapid, 3=plasma, 4=lightning
    Lightning lightning;
    bool lightningFiring = false;
    uint32_t lightningFireTime = 0;

    // Bullet system
    struct Bullet {
        float x, y;
        float angle;
        float distance;
        bool active;
        uint32_t spawnTime;
        int type;  // Weapon type that fired this bullet
        float speed;  // Bullet speed (pixels per frame)
        struct TrailPoint {
            float x, y;
            bool active;
        };
        TrailPoint trail[5];  // Trail points for tracer effect
        int trailIndex;
    };
    static constexpr int MAX_BULLETS = 8;  // Reduced since we only fire single shots now
    Bullet bullets[MAX_BULLETS];
    int muzzleFlash = 0;  // Muzzle flash animation counter
    int gunRecoil = 0;    // Gun recoil animation counter
    float currentBrightness = 0.7f;  // Current display brightness
    float targetBrightness = 0.7f;   // Target brightness to fade to

    // Power-up system
    struct PowerUp {
        float x, y;
        bool active;
        uint32_t spawnTime;
        float animPhase;
        int type;  // 0=weapon upgrade, 1=health pack
    };
    static constexpr int MAX_POWERUPS = 4;
    PowerUp powerups[MAX_POWERUPS];

    // HSV to RGB conversion for fancy power-up colors
    void hsv_to_rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
        float c = v * s;
        float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r_prime, g_prime, b_prime;

        if (h < 60) {
            r_prime = c; g_prime = x; b_prime = 0;
        } else if (h < 120) {
            r_prime = x; g_prime = c; b_prime = 0;
        } else if (h < 180) {
            r_prime = 0; g_prime = c; b_prime = x;
        } else if (h < 240) {
            r_prime = 0; g_prime = x; b_prime = c;
        } else if (h < 300) {
            r_prime = x; g_prime = 0; b_prime = c;
        } else {
            r_prime = c; g_prime = 0; b_prime = x;
        }

        r = (uint8_t)((r_prime + m) * 255);
        g = (uint8_t)((g_prime + m) * 255);
        b = (uint8_t)((b_prime + m) * 255);
    }

    // Enemy system
    struct Enemy {
        float x, y;
        bool active;
        int health;
        uint32_t animFrame;
        uint32_t lastAnimTime;
        uint32_t lastAttackTime;
    };
    static constexpr int MAX_ENEMIES = 8;
    Enemy enemies[MAX_ENEMIES];
    uint32_t lastSpawnTime = 0;
    static constexpr uint32_t SPAWN_INTERVAL = 1500;  // 1.5 seconds

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

    // Theme system
    enum class Theme {
        HELL,
        SPACE,
        CYBER,
        TOXIC,
        ICE,
        DESERT,
        INDUSTRIAL,
        FOREST,
        COUNT
    };
    Theme currentTheme = Theme::HELL;
    uint32_t lastThemeChange = 0;
    static constexpr uint32_t THEME_CHANGE_INTERVAL = 20000;  // 20 seconds
    float skyOffset = 0.0f;  // For parallax scrolling

    // Theme transition state
    enum class TransitionState {
        NORMAL,
        FADING_OUT,
        FADING_IN
    };
    TransitionState transitionState = TransitionState::NORMAL;
    uint32_t transitionStartTime = 0;
    static constexpr uint32_t FADE_DURATION = 500;  // 500ms fade duration
    Theme nextTheme = Theme::HELL;

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

    // Shoot a bullet based on weapon type
    void shoot() {
        // Don't show muzzle flash/recoil for lightning weapon
        if (weaponType != 4) {
            muzzleFlash = 8;
            gunRecoil = 4;
        }

        // Trigger brightness flash
        currentBrightness = 1.0f;

        switch (weaponType) {
            case 0:   // Single shot - standard speed, yellow
            case 1:   // Spread shot - now just fast green tracer
            case 2:   // Rapid fire - now just very fast cyan tracer
            case 3: { // Plasma - now just slow purple tracer
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i].x = playerX;
                        bullets[i].y = playerY;
                        bullets[i].angle = playerAngle;
                        bullets[i].distance = 0;
                        bullets[i].active = true;
                        bullets[i].spawnTime = time_us_32() / 1000;
                        bullets[i].type = weaponType;
                        bullets[i].trailIndex = 0;

                        // Set speed based on weapon type
                        switch (weaponType) {
                            case 0: bullets[i].speed = 10.0f; break;  // Standard
                            case 1: bullets[i].speed = 15.0f; break;  // Fast
                            case 2: bullets[i].speed = 20.0f; break;  // Very fast
                            case 3: bullets[i].speed = 7.0f; break;   // Slow
                        }

                        // Initialize trail
                        for (int t = 0; t < 5; t++) {
                            bullets[i].trail[t].active = false;
                        }
                        break;
                    }
                }
                break;
            }

            case 4: {  // Lightning (fired from gun tip)
                // Trigger lightning effect
                lightningFiring = true;
                lightningFireTime = time_us_32() / 1000;

                // Fire 3 lightning strikes from gun position
                // Gun is at bottom center of screen
                float startScreenX = WIDTH / 2.0f;
                float startScreenY = HEIGHT - 2.0f;  // Near bottom

                // Fire 3 strikes with slight variations
                for (int strike = 0; strike < 3; strike++) {
                    // Vary the end position slightly for each strike
                    float endScreenX = WIDTH / 2.0f + (strike - 1) * 2.0f;  // Spread strikes horizontally
                    float endScreenY = HEIGHT / 2.0f - 5.0f + (strike - 1) * 1.0f;  // Slight vertical variation

                    lightning.triggerStrike(startScreenX, startScreenY, endScreenX, endScreenY);
                }
                break;
            }
        }
    }

    // Update bullets
    void updateBullets() {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Store current position in trail before moving
                bullets[i].trail[bullets[i].trailIndex].x = bullets[i].x;
                bullets[i].trail[bullets[i].trailIndex].y = bullets[i].y;
                bullets[i].trail[bullets[i].trailIndex].active = true;
                bullets[i].trailIndex = (bullets[i].trailIndex + 1) % 5;

                // Move bullet forward using its speed
                bullets[i].distance += bullets[i].speed;
                bullets[i].x += cosf(bullets[i].angle) * bullets[i].speed;
                bullets[i].y += sinf(bullets[i].angle) * bullets[i].speed;

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
                        enemies[i].lastAttackTime = time_us_32() / 1000;
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

                // Move toward player and attack if close
                float dx = playerX - enemies[i].x;
                float dy = playerY - enemies[i].y;
                float dist = sqrtf(dx * dx + dy * dy);

                // Attack player if close enough (within 30 units)
                if (dist < 30.0f && !isDead) {
                    if (currentTime - enemies[i].lastAttackTime > 1000) {  // Attack every 1 second
                        playerHealth -= 10;
                        enemies[i].lastAttackTime = currentTime;
                        if (playerHealth <= 0) {
                            playerHealth = 0;
                            isDead = true;
                            deathTime = currentTime;
                            // Player death explosion
                            spawnExplosion(playerX, playerY);
                        }
                    }
                } else if (dist > 10.0f) {
                    // Move toward player if not in attack range
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
        // Regular bullet collisions
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

                        // 50% chance to drop a power-up
                        if (rand() % 2 == 0) {
                            spawnPowerUp(enemies[e].x, enemies[e].y);
                        }

                        enemies[e].active = false;
                    }
                    break;
                }
            }
        }

        // Lightning weapon damage (hits all enemies in cone)
        if (lightningFiring) {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;

                // Check if enemy is in front of player (in lightning cone)
                float dx = enemies[e].x - playerX;
                float dy = enemies[e].y - playerY;
                float dist = sqrtf(dx * dx + dy * dy);

                float enemyAngle = atan2f(dy, dx);
                float angleDiff = enemyAngle - playerAngle;

                // Normalize angle
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // Lightning hits in a cone (30 degrees, 200 units range)
                if (dist < 200.0f && fabsf(angleDiff) < 0.26f) {  // ~30 degree cone
                    enemies[e].health -= 2;  // Lightning does more damage

                    if (enemies[e].health <= 0) {
                        spawnExplosion(enemies[e].x, enemies[e].y);

                        if (rand() % 2 == 0) {
                            spawnPowerUp(enemies[e].x, enemies[e].y);
                        }

                        enemies[e].active = false;
                    }
                }
            }
        }
    }

    // Spawn a power-up
    void spawnPowerUp(float x, float y) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (!powerups[i].active) {
                powerups[i].x = x;
                powerups[i].y = y;
                powerups[i].active = true;
                powerups[i].spawnTime = time_us_32() / 1000;
                powerups[i].animPhase = 0.0f;
                // 50% chance of med pack, 50% chance of weapon upgrade
                powerups[i].type = (rand() % 2);
                break;
            }
        }
    }

    // Update power-ups
    void updatePowerUps() {
        uint32_t currentTime = time_us_32() / 1000;

        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerups[i].active) {
                // Update animation phase
                powerups[i].animPhase += 0.1f;
                if (powerups[i].animPhase > 360.0f) {
                    powerups[i].animPhase -= 360.0f;
                }

                // Check if player picks up the power-up
                float dx = playerX - powerups[i].x;
                float dy = playerY - powerups[i].y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < 20.0f) {
                    // Pick up power-up
                    if (powerups[i].type == 0) {
                        // Weapon upgrade
                        weaponType = (weaponType + 1) % 5;  // 5 weapons (0-4)
                    } else {
                        // Health pack - restore 25 health
                        playerHealth = (playerHealth + 25 > MAX_HEALTH) ? MAX_HEALTH : playerHealth + 25;
                    }
                    powerups[i].active = false;
                }

                // Despawn after 10 seconds
                if (currentTime - powerups[i].spawnTime > 10000) {
                    powerups[i].active = false;
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
        uint32_t currentTime = time_us_32() / 1000;

        // Calculate parallax offset based on player rotation
        skyOffset = fmodf(playerAngle * 5.0f, 64.0f);  // Parallax factor

        switch(currentTheme) {
            case Theme::HELL: {
                // Hell theme - burning sky with floating embers
                for (int y = 0; y < HEIGHT / 2; y++) {
                    int shade = (y * 255) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(100 + shade / 2, shade / 8, 0);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Floating embers with parallax
                for (int i = 0; i < 8; i++) {
                    int x = (int)(i * 8 + skyOffset) % WIDTH;
                    int y = ((currentTime / 100 + i * 20) % 100) / 7;  // Slow rise
                    if (y < HEIGHT / 2) {
                        Pen emberPen = gfx.create_pen(255, 100 + (i * 20) % 100, 0);
                        gfx.set_pen(emberPen);
                        gfx.pixel(Point(x, y));
                    }
                }

                // Lava floor - much darker
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 60) / (HEIGHT / 2);
                    int flicker = ((currentTime + y * 10) % 20) - 10;
                    Pen floorPen = gfx.create_pen(40 + shade + flicker, shade / 4, 0);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                break;
            }

            case Theme::SPACE: {
                // Space theme - star field with nebula
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Deep space gradient with purple nebula
                    int shade = (y * 100) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(shade / 4, 0, 40 + shade / 2);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Stars with parallax (different layers)
                for (int layer = 0; layer < 3; layer++) {
                    float layerSpeed = 1.0f + layer * 0.5f;
                    for (int i = 0; i < 12; i++) {
                        int x = (int)(i * 7 + skyOffset * layerSpeed + layer * 3) % WIDTH;
                        int y = ((i * 11 + layer * 5) % (HEIGHT / 2));
                        int brightness = 150 + (i * 30) % 100;
                        Pen starPen = gfx.create_pen(brightness, brightness, 200 + brightness / 3);
                        gfx.set_pen(starPen);
                        gfx.pixel(Point(x, y));
                        if (layer == 2 && i % 3 == 0) {  // Some stars twinkle
                            if ((currentTime / 200 + i) % 3 == 0) {
                                gfx.pixel(Point(x + 1, y));
                            }
                        }
                    }
                }

                // Metal floor with grid
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 60) / (HEIGHT / 2);
                    Pen floorPen = gfx.create_pen(shade / 2, shade / 2, shade);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Grid lines on floor
                for (int x = 0; x < WIDTH; x += 4) {
                    if ((x + (int)skyOffset) % 8 == 0) {
                        gfx.set_pen(gfx.create_pen(0, 100, 150));
                        gfx.pixel(Point(x, HEIGHT / 2 + 2));
                    }
                }
                break;
            }

            case Theme::CYBER: {
                // Cyber/neon city theme
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Pink/cyan gradient sky
                    int shade = (y * 255) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(shade / 3, shade / 2, 100 + shade / 2);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Neon city skyline with parallax
                for (int i = 0; i < 6; i++) {
                    int x = (int)(i * 10 + skyOffset * 0.7f) % (WIDTH + 10) - 5;
                    int buildingHeight = 3 + (i % 3) * 2;
                    int buildingWidth = 3 + (i % 2);

                    for (int bx = 0; bx < buildingWidth; bx++) {
                        for (int by = 0; by < buildingHeight; by++) {
                            int px = x + bx;
                            int py = HEIGHT / 2 - by - 1;
                            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT / 2) {
                                // Building body
                                Pen buildPen = gfx.create_pen(20, 20, 40);
                                gfx.set_pen(buildPen);
                                gfx.pixel(Point(px, py));

                                // Neon windows
                                if (by % 2 == 0 && bx % 2 == 1) {
                                    int neonChoice = i % 3;
                                    if (neonChoice == 0) {
                                        gfx.set_pen(gfx.create_pen(255, 0, 150));
                                    } else if (neonChoice == 1) {
                                        gfx.set_pen(gfx.create_pen(0, 255, 255));
                                    } else {
                                        gfx.set_pen(gfx.create_pen(255, 255, 0));
                                    }
                                    gfx.pixel(Point(px, py));
                                }
                            }
                        }
                    }
                }

                // Neon grid floor
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 80) / (HEIGHT / 2);
                    Pen floorPen = gfx.create_pen(shade / 4, 0, shade / 2);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Animated grid
                for (int x = 0; x < WIDTH; x += 3) {
                    if ((x + (int)skyOffset + (int)(currentTime / 100)) % 6 == 0) {
                        gfx.set_pen(gfx.create_pen(255, 0, 255));
                        gfx.pixel(Point(x, HEIGHT / 2 + 1));
                    }
                }
                break;
            }

            case Theme::TOXIC: {
                // Toxic wasteland theme
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Sickly green sky with pollution
                    int shade = (y * 200) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(shade / 4, 80 + shade / 2, shade / 8);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Toxic clouds drifting with parallax
                for (int i = 0; i < 10; i++) {
                    int x = (int)(i * 6 + skyOffset * 0.3f + (currentTime / 50)) % (WIDTH + 8) - 4;
                    int y = (i * 3) % (HEIGHT / 2 - 2);
                    int cloudSize = 2 + (i % 2);

                    for (int cx = 0; cx < cloudSize; cx++) {
                        int px = x + cx;
                        if (px >= 0 && px < WIDTH) {
                            Pen cloudPen = gfx.create_pen(100, 150 + (i * 10) % 50, 20);
                            gfx.set_pen(cloudPen);
                            gfx.pixel(Point(px, y));
                        }
                    }
                }

                // Toxic sludge floor with bubbles - much darker
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 50) / (HEIGHT / 2);
                    int pulse = (int)(sinf((currentTime / 100.0f) + y * 0.1f) * 10);
                    Pen floorPen = gfx.create_pen(shade / 5, 20 + shade / 2 + pulse, 0);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Bubbles rising
                for (int i = 0; i < 6; i++) {
                    int x = (i * 11) % WIDTH;
                    int y = HEIGHT / 2 + ((currentTime / 150 + i * 40) % 100) / 6;
                    if (y < HEIGHT) {
                        gfx.set_pen(gfx.create_pen(150, 200, 50));
                        gfx.pixel(Point(x, y));
                    }
                }
                break;
            }

            case Theme::ICE: {
                // Ice/Frozen theme - aurora sky with snowflakes
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Gradient from deep blue to cyan with aurora shimmer
                    int shade = (y * 200) / (HEIGHT / 2);
                    int shimmer = (int)(sinf((currentTime / 300.0f) + y * 0.3f) * 20);
                    Pen skyPen = gfx.create_pen(shade / 6 + shimmer, 100 + shade / 3, 150 + shade / 4);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Aurora bands with parallax
                for (int band = 0; band < 2; band++) {
                    for (int x = 0; x < WIDTH; x++) {
                        int waveY = HEIGHT / 4 + band * 4 + (int)(sinf((x + skyOffset * 0.5f + band * 10) * 0.3f) * 2);
                        if (waveY >= 0 && waveY < HEIGHT / 2) {
                            int brightness = 100 + (int)(sinf((currentTime / 200.0f) + x * 0.2f) * 50);
                            Pen auroraPen = gfx.create_pen(brightness / 3, brightness, brightness);
                            gfx.set_pen(auroraPen);
                            gfx.pixel(Point(x, waveY));
                        }
                    }
                }

                // Falling snowflakes with parallax layers
                for (int i = 0; i < 15; i++) {
                    int x = (int)(i * 7 + skyOffset * 0.2f) % WIDTH;
                    int y = ((currentTime / 50 + i * 15) % 120) / 4;  // Slow fall
                    if (y < HEIGHT / 2) {
                        gfx.set_pen(gfx.create_pen(200 + (i % 2) * 55, 200 + (i % 2) * 55, 255));
                        gfx.pixel(Point(x, y));
                    }
                }

                // Icy floor with cracks
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 60) / (HEIGHT / 2);
                    int sparkle = ((currentTime / 100 + y * 3) % 20) - 10;
                    Pen floorPen = gfx.create_pen(80 + shade / 2 + sparkle / 2, 100 + shade / 2 + sparkle / 2, 120 + shade / 2);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Ice cracks
                for (int x = 0; x < WIDTH; x += 7) {
                    if ((x + (int)skyOffset) % 11 == 0) {
                        int crackY = HEIGHT / 2 + 3 + (x % 3);
                        if (crackY < HEIGHT) {
                            gfx.set_pen(gfx.create_pen(100, 120, 150));
                            gfx.pixel(Point(x, crackY));
                        }
                    }
                }
                break;
            }

            case Theme::DESERT: {
                // Desert/Canyon theme - hot orange sky with sun
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Orange to yellow gradient
                    int shade = (y * 255) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(200 + shade / 5, 100 + shade / 2, 30 + shade / 8);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Scorching sun with parallax (moves slowly)
                int sunX = (int)(WIDTH / 2 + skyOffset * 0.1f);
                int sunY = HEIGHT / 6;
                for (int sy = -3; sy <= 3; sy++) {
                    for (int sx = -3; sx <= 3; sx++) {
                        if (sx*sx + sy*sy <= 9) {
                            int x = sunX + sx;
                            int y = sunY + sy;
                            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT / 2) {
                                gfx.set_pen(gfx.create_pen(255, 220, 100));
                                gfx.pixel(Point(x, y));
                            }
                        }
                    }
                }

                // Heat shimmer particles rising
                for (int i = 0; i < 8; i++) {
                    int x = (i * 9 + (int)(skyOffset * 0.3f)) % WIDTH;
                    int y = HEIGHT / 2 - 1 - ((currentTime / 100 + i * 20) % 80) / 5;
                    if (y >= 0 && y < HEIGHT / 2) {
                        int opacity = 100 + (i * 15) % 100;
                        gfx.set_pen(gfx.create_pen(255, 180 + opacity / 3, opacity / 2));
                        gfx.pixel(Point(x, y));
                    }
                }

                // Sandy floor with dunes pattern
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 80) / (HEIGHT / 2);
                    int dunePattern = (int)(sinf((y + currentTime / 100.0f) * 0.5f) * 15);
                    Pen floorPen = gfx.create_pen(180 + shade / 2 + dunePattern / 2, 140 + shade / 2 + dunePattern / 3, 60 + shade / 4);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Sand ripples
                for (int x = 0; x < WIDTH; x += 4) {
                    if ((x + (int)skyOffset + (int)(currentTime / 200)) % 8 == 0) {
                        gfx.set_pen(gfx.create_pen(140, 100, 40));
                        gfx.pixel(Point(x, HEIGHT / 2 + 2));
                    }
                }
                break;
            }

            case Theme::INDUSTRIAL: {
                // Industrial/Factory theme - polluted sky with smokestacks
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Dark grey/brown polluted sky
                    int shade = (y * 150) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(60 + shade / 3, 50 + shade / 4, 40 + shade / 5);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Smoke plumes with parallax
                for (int stack = 0; stack < 3; stack++) {
                    int stackX = (int)(stack * 12 + skyOffset * 0.6f) % (WIDTH + 8) - 4;
                    // Smoke rising and expanding
                    for (int puff = 0; puff < 5; puff++) {
                        int puffY = HEIGHT / 2 - 3 - puff * 2 - ((currentTime / 80 + stack * 10) % 30) / 3;
                        int puffSize = 1 + puff / 2;
                        for (int px = 0; px < puffSize; px++) {
                            int x = stackX + px - puffSize / 2 + (puff % 2);
                            if (x >= 0 && x < WIDTH && puffY >= 0 && puffY < HEIGHT / 2) {
                                int smokeShade = 80 + puff * 10;
                                gfx.set_pen(gfx.create_pen(smokeShade, smokeShade - 10, smokeShade - 20));
                                gfx.pixel(Point(x, puffY));
                            }
                        }
                    }
                }

                // Industrial skyline with parallax
                for (int i = 0; i < 8; i++) {
                    int x = (int)(i * 8 + skyOffset * 0.8f) % (WIDTH + 6) - 3;
                    int structureHeight = 2 + (i % 4);
                    for (int h = 0; h < structureHeight; h++) {
                        int px = x;
                        int py = HEIGHT / 2 - h - 1;
                        if (px >= 0 && px < WIDTH && py >= 0) {
                            gfx.set_pen(gfx.create_pen(30, 30, 30));
                            gfx.pixel(Point(px, py));
                            // Warning lights on top
                            if (h == structureHeight - 1 && (currentTime / 400 + i) % 2 == 0) {
                                gfx.set_pen(gfx.create_pen(255, 0, 0));
                                gfx.pixel(Point(px, py));
                            }
                        }
                    }
                }

                // Metal grating floor with rust
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 60) / (HEIGHT / 2);
                    int rust = ((y + (int)(currentTime / 100)) % 5 == 0) ? 15 : 0;
                    Pen floorPen = gfx.create_pen(40 + shade / 2 + rust, 40 + shade / 2, 40 + shade / 2);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Grating lines
                for (int x = 0; x < WIDTH; x += 2) {
                    if ((x + (int)skyOffset) % 4 == 0) {
                        gfx.set_pen(gfx.create_pen(80, 80, 80));
                        gfx.pixel(Point(x, HEIGHT / 2 + 1));
                    }
                }
                // Rust spots
                for (int i = 0; i < 6; i++) {
                    int x = (i * 11) % WIDTH;
                    int y = HEIGHT / 2 + 4 + (i % 3);
                    if (y < HEIGHT) {
                        gfx.set_pen(gfx.create_pen(120, 60, 20));
                        gfx.pixel(Point(x, y));
                    }
                }
                break;
            }

            case Theme::FOREST: {
                // Forest/Jungle theme - green canopy with fireflies
                for (int y = 0; y < HEIGHT / 2; y++) {
                    // Dark green gradient (seen through canopy)
                    int shade = (y * 120) / (HEIGHT / 2);
                    Pen skyPen = gfx.create_pen(shade / 6, 40 + shade / 2, shade / 8);
                    gfx.set_pen(skyPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }

                // Canopy leaves with parallax
                for (int layer = 0; layer < 3; layer++) {
                    float layerSpeed = 0.3f + layer * 0.2f;
                    for (int i = 0; i < 10; i++) {
                        int x = (int)(i * 6 + skyOffset * layerSpeed + layer * 4) % WIDTH;
                        int y = (i * 5 + layer * 3) % (HEIGHT / 2);
                        int leafSize = 2 - layer / 2;

                        for (int lx = 0; lx < leafSize; lx++) {
                            int px = x + lx;
                            if (px >= 0 && px < WIDTH && y >= 0 && y < HEIGHT / 2) {
                                int greenShade = 100 + (layer * 30) - (i * 5);
                                gfx.set_pen(gfx.create_pen(greenShade / 5, greenShade, greenShade / 4));
                                gfx.pixel(Point(px, y));
                            }
                        }
                    }
                }

                // Glowing fireflies
                for (int i = 0; i < 12; i++) {
                    int x = (int)(i * 7 + sinf((currentTime / 300.0f) + i) * 3) % WIDTH;
                    int y = (int)(i * 3 + cosf((currentTime / 250.0f) + i * 2) * 2) % (HEIGHT / 2);
                    // Pulsing glow
                    int glow = (int)(sinf((currentTime / 150.0f) + i * 0.5f) * 100 + 155);
                    if ((currentTime / 100 + i) % 5 != 0) {  // Some flicker
                        gfx.set_pen(gfx.create_pen(glow, glow, 100 + glow / 2));
                        gfx.pixel(Point(x, y));
                    }
                }

                // Mossy ground floor
                for (int y = HEIGHT / 2; y < HEIGHT; y++) {
                    int shade = ((y - HEIGHT / 2) * 80) / (HEIGHT / 2);
                    int moss = (int)(sinf((y + currentTime / 100.0f) * 0.7f) * 10);
                    Pen floorPen = gfx.create_pen(shade / 5 + moss / 2, 60 + shade / 2 + moss, shade / 6);
                    gfx.set_pen(floorPen);
                    for (int x = 0; x < WIDTH; x++) {
                        gfx.pixel(Point(x, y));
                    }
                }
                // Roots and vines pattern
                for (int x = 0; x < WIDTH; x += 5) {
                    if ((x + (int)skyOffset + (int)(currentTime / 150)) % 9 == 0) {
                        int rootY = HEIGHT / 2 + 2 + (x % 4);
                        if (rootY < HEIGHT) {
                            gfx.set_pen(gfx.create_pen(40, 30, 20));
                            gfx.pixel(Point(x, rootY));
                        }
                    }
                }
                // Mushrooms glowing on floor
                for (int i = 0; i < 5; i++) {
                    int x = (i * 13) % WIDTH;
                    int y = HEIGHT - 2 - (i % 2);
                    if ((currentTime / 300 + i) % 4 < 2) {  // Pulsing
                        gfx.set_pen(gfx.create_pen(150, 100 + (i * 30) % 100, 200));
                        gfx.pixel(Point(x, y));
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    void drawWalls(PicoGraphics_PenRGB888& gfx) {
        float startAngle = playerAngle - FOV / 2.0f;
        float angleStep = FOV / (RAY_COUNT - 1);  // Divide by RAY_COUNT-1 to ensure we hit both edges

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

            // Create wall color based on theme
            int r, g, b;
            switch(currentTheme) {
                case Theme::HELL:
                    // Dark red stone walls
                    r = shade * 3 / 4;
                    g = shade / 8;
                    b = shade / 6;
                    break;

                case Theme::SPACE:
                    // Metallic silver/blue walls
                    r = shade / 3;
                    g = shade / 3;
                    b = shade;
                    break;

                case Theme::CYBER:
                    // Purple/magenta neon walls
                    r = shade * 2 / 3;
                    g = shade / 6;
                    b = shade;
                    break;

                case Theme::TOXIC:
                    // Sickly yellow-green walls
                    r = shade / 4;
                    g = shade * 3 / 4;
                    b = shade / 8;
                    break;

                case Theme::ICE:
                    // Crystalline ice walls - bright blue-white
                    r = shade * 3 / 4;
                    g = shade * 4 / 5;
                    b = shade;
                    break;

                case Theme::DESERT:
                    // Sandstone canyon walls - orange/tan
                    r = shade;
                    g = shade * 2 / 3;
                    b = shade / 4;
                    break;

                case Theme::INDUSTRIAL:
                    // Rusted metal walls - dark grey with rust
                    r = shade / 2 + 20;
                    g = shade / 2;
                    b = shade / 2;
                    break;

                case Theme::FOREST:
                    // Ancient stone covered in moss - dark green-grey
                    r = shade / 4;
                    g = shade / 2;
                    b = shade / 6;
                    break;

                default:
                    r = 0;
                    g = shade / 4;
                    b = shade;
                    break;
            }

            Pen wallPen = gfx.create_pen(r, g, b);
            gfx.set_pen(wallPen);

            // Draw wall column
            gfx.line(Point(i, wallTop), Point(i, wallBottom));
        }
    }

    // Draw bullets in 3D view with trail effect
    void drawBullets(PicoGraphics_PenRGB888& gfx) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {
                // Get base color for this weapon type
                uint8_t base_r, base_g, base_b;
                switch (bullets[i].type) {
                    case 0:  // Single - yellow/orange
                        base_r = 255; base_g = 200; base_b = 0;
                        break;
                    case 1:  // Spread - green
                        base_r = 100; base_g = 255; base_b = 100;
                        break;
                    case 2:  // Rapid - cyan
                        base_r = 100; base_g = 255; base_b = 255;
                        break;
                    case 3:  // Plasma - magenta/purple
                        base_r = 255; base_g = 100; base_b = 255;
                        break;
                    default:
                        base_r = 255; base_g = 200; base_b = 0;
                        break;
                }

                // Draw trail points (oldest to newest, fading)
                for (int t = 0; t < 5; t++) {
                    int trailIdx = (bullets[i].trailIndex + t) % 5;
                    if (bullets[i].trail[trailIdx].active) {
                        float dx = bullets[i].trail[trailIdx].x - playerX;
                        float dy = bullets[i].trail[trailIdx].y - playerY;
                        float trailDist = sqrtf(dx * dx + dy * dy);

                        float trailAngle = atan2f(dy, dx);
                        float angleDiff = trailAngle - playerAngle;
                        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                        if (trailDist > 1.0f && fabsf(angleDiff) < FOV / 2.0f) {
                            int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));
                            int screenY = HEIGHT / 2;

                            // Fade based on age (newer = brighter)
                            float fade = (float)(t + 1) / 5.0f;
                            uint8_t r = (uint8_t)(base_r * fade);
                            uint8_t g = (uint8_t)(base_g * fade);
                            uint8_t b = (uint8_t)(base_b * fade);

                            gfx.set_pen(gfx.create_pen(r, g, b));
                            if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
                                gfx.pixel(Point(screenX, screenY));
                            }
                        }
                    }
                }

                // Draw main bullet (bright)
                float dx = bullets[i].x - playerX;
                float dy = bullets[i].y - playerY;
                float bulletDist = sqrtf(dx * dx + dy * dy);

                float bulletAngle = atan2f(dy, dx);
                float angleDiff = bulletAngle - playerAngle;
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                if (bulletDist > 1.0f && fabsf(angleDiff) < FOV / 2.0f) {
                    int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));
                    int screenY = HEIGHT / 2;

                    // Draw bright bullet head
                    gfx.set_pen(gfx.create_pen(base_r, base_g, base_b));
                    if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
                        gfx.pixel(Point(screenX, screenY));
                    }

                    // Add bright center glow
                    gfx.set_pen(gfx.create_pen(255, 255, 255));
                    if (screenX >= 0 && screenX < WIDTH && screenY >= 0 && screenY < HEIGHT) {
                        gfx.pixel(Point(screenX, screenY));
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
        // Health bar at bottom left
        int healthBarWidth = 10;
        int healthBarHeight = 2;
        int healthBarX = 1;
        int healthBarY = HEIGHT - 3;

        // Background (black)
        Pen bgPen = gfx.create_pen(0, 0, 0);
        gfx.set_pen(bgPen);
        for (int y = healthBarY; y < healthBarY + healthBarHeight; y++) {
            for (int x = healthBarX; x < healthBarX + healthBarWidth; x++) {
                gfx.pixel(Point(x, y));
            }
        }

        // Health fill (color based on health percentage)
        float healthPercent = (float)playerHealth / MAX_HEALTH;
        int fillWidth = (int)(healthBarWidth * healthPercent);

        Pen healthPen;
        if (healthPercent > 0.6f) {
            // Green when healthy
            healthPen = gfx.create_pen(0, 255, 0);
        } else if (healthPercent > 0.3f) {
            // Amber when damaged
            healthPen = gfx.create_pen(255, 150, 0);
        } else {
            // Red when critical
            healthPen = gfx.create_pen(255, 0, 0);
        }

        gfx.set_pen(healthPen);
        for (int y = healthBarY; y < healthBarY + healthBarHeight; y++) {
            for (int x = healthBarX; x < healthBarX + fillWidth; x++) {
                gfx.pixel(Point(x, y));
            }
        }

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

    // Draw power-ups in 3D view
    void drawPowerUps(PicoGraphics_PenRGB888& gfx) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            if (powerups[i].active) {
                // Calculate power-up position relative to player
                float dx = powerups[i].x - playerX;
                float dy = powerups[i].y - playerY;
                float powerupDist = sqrtf(dx * dx + dy * dy);

                // Calculate angle relative to player view
                float powerupAngle = atan2f(dy, dx);
                float angleDiff = powerupAngle - playerAngle;

                // Normalize angle
                while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
                while (angleDiff < -3.14159f) angleDiff += 6.28318f;

                // Only draw if power-up is in front of player and within FOV
                if (powerupDist > 1.0f && fabsf(angleDiff) < FOV / 2.0f + 0.5f) {
                    // Calculate screen X position
                    int screenX = WIDTH / 2 + (int)((angleDiff / (FOV / 2.0f)) * (WIDTH / 2));

                    // Calculate size based on distance
                    int spriteSize = (int)(300.0f / powerupDist);
                    if (spriteSize < 3) spriteSize = 3;
                    if (spriteSize > 8) spriteSize = 8;

                    int screenY = HEIGHT / 2;

                    // Pulsing animation using sine wave
                    float pulse = sinf(powerups[i].animPhase * 0.0174533f) * 0.5f + 0.5f;  // 0.0-1.0

                    uint8_t r, g, b;
                    if (powerups[i].type == 0) {
                        // Weapon upgrade - Rainbow color cycling (HSV)
                        float hue = fmodf(powerups[i].animPhase * 2.0f, 360.0f);  // Cycle through hues
                        hsv_to_rgb(hue, 1.0f, pulse * 0.7f + 0.3f, r, g, b);  // Pulse brightness
                    } else {
                        // Health pack - Red/white pulsing
                        int brightness = (int)(pulse * 200.0f + 55.0f);
                        r = brightness;
                        g = brightness / 4;
                        b = brightness / 4;
                    }

                    // Draw main power-up body as a rotating diamond/star
                    Pen powerupPen = gfx.create_pen(r, g, b);
                    gfx.set_pen(powerupPen);

                    for (int py = -spriteSize/2; py <= spriteSize/2; py++) {
                        for (int px = -spriteSize/2; px <= spriteSize/2; px++) {
                            if (abs(px) + abs(py) <= spriteSize/2) {
                                int x = screenX + px;
                                int y = screenY + py;
                                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                                    gfx.pixel(Point(x, y));
                                }
                            }
                        }
                    }

                    // Add white sparkle effect in cross pattern
                    if ((int)(powerups[i].animPhase / 12.0f) % 3 == 0 && spriteSize >= 4) {
                        gfx.set_pen(gfx.create_pen(255, 255, 255));
                        int sparkleSize = spriteSize / 2 + 1;

                        // Horizontal sparkle
                        if (screenX + sparkleSize < WIDTH) {
                            gfx.pixel(Point(screenX + sparkleSize, screenY));
                        }
                        if (screenX - sparkleSize >= 0) {
                            gfx.pixel(Point(screenX - sparkleSize, screenY));
                        }

                        // Vertical sparkle
                        if (screenY + sparkleSize < HEIGHT) {
                            gfx.pixel(Point(screenX, screenY + sparkleSize));
                        }
                        if (screenY - sparkleSize >= 0) {
                            gfx.pixel(Point(screenX, screenY - sparkleSize));
                        }
                    }

                    // Add bright center pixel for extra pop
                    if (pulse > 0.7f) {
                        gfx.set_pen(gfx.create_pen(255, 255, 255));
                        gfx.pixel(Point(screenX, screenY));
                    }
                }
            }
        }
    }

    bool shouldExit = false;
    bool buttonDPressed = false;

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

        // Initialize theme system
        currentTheme = Theme::HELL;
        lastThemeChange = time_us_32() / 1000;
        skyOffset = 0.0f;

        // Initialize health system
        playerHealth = MAX_HEALTH;
        isDead = false;
        deathTime = 0;

        // Initialize weapon and power-ups
        weaponType = 0;
        for (int i = 0; i < MAX_POWERUPS; i++) {
            powerups[i].active = false;
        }

        // Initialize lightning effect
        lightning.init();
        lightning.enableAutoSpawn(false);  // Disable random lightning, only use for gun
        lightningFiring = false;

        // Set initial brightness
        currentBrightness = 0.7f;
        targetBrightness = 0.7f;
        cosmic->set_brightness(currentBrightness);
    }

    void resetGame() {
        // Reset player
        playerX = TILE_SIZE * 1.5f;
        playerY = TILE_SIZE * 1.5f;
        playerAngle = 0.0f;
        playerHealth = MAX_HEALTH;
        isDead = false;

        // Clear enemies
        for (int i = 0; i < MAX_ENEMIES; i++) {
            enemies[i].active = false;
        }

        // Clear bullets
        for (int i = 0; i < MAX_BULLETS; i++) {
            bullets[i].active = false;
        }

        // Clear particles
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles[i].active = false;
        }

        // Clear power-ups
        for (int i = 0; i < MAX_POWERUPS; i++) {
            powerups[i].active = false;
        }
    }

    bool update() override {
        uint32_t currentTime = time_us_32() / 1000;

        // Handle death and respawn
        if (isDead) {
            // Wait 2 seconds after death before respawning
            if (currentTime - deathTime > 2000) {
                resetGame();
            }
            // Update particles during death (for explosion)
            updateParticles();
            return !shouldExit;
        }

        updateAI();
        updateBullets();
        updateEnemies();
        updateParticles();
        updatePowerUps();
        checkCollisions();

        // Update lightning effect
        float dt = 0.05f;  // 50ms per frame
        lightning.update(dt);

        // Reset lightning firing flag after duration
        if (lightningFiring && currentTime - lightningFireTime > 200) {
            lightningFiring = false;
        }

        // Auto-change themes periodically with fade transition
        if (transitionState == TransitionState::NORMAL) {
            if (currentTime - lastThemeChange > THEME_CHANGE_INTERVAL) {
                // Start fade out
                transitionState = TransitionState::FADING_OUT;
                transitionStartTime = currentTime;
                nextTheme = (Theme)(((int)currentTheme + 1) % (int)Theme::COUNT);
            }
        } else if (transitionState == TransitionState::FADING_OUT) {
            uint32_t elapsed = currentTime - transitionStartTime;
            if (elapsed >= FADE_DURATION) {
                // Fade out complete, switch theme and start fade in
                currentTheme = nextTheme;
                transitionState = TransitionState::FADING_IN;
                transitionStartTime = currentTime;
                lastThemeChange = currentTime;
            } else {
                // Fade out: reduce brightness
                targetBrightness = 0.0f;
            }
        } else if (transitionState == TransitionState::FADING_IN) {
            uint32_t elapsed = currentTime - transitionStartTime;
            if (elapsed >= FADE_DURATION) {
                // Fade in complete
                transitionState = TransitionState::NORMAL;
                targetBrightness = 0.7f;
            } else {
                // Fade in: increase brightness
                targetBrightness = 0.7f;
            }
        }

        // Smooth brightness transitions
        if (transitionState == TransitionState::FADING_OUT) {
            // Fast fade to black
            float fadeProgress = (float)(currentTime - transitionStartTime) / FADE_DURATION;
            currentBrightness = 0.7f * (1.0f - fadeProgress);
            if (currentBrightness < 0.0f) currentBrightness = 0.0f;
        } else if (transitionState == TransitionState::FADING_IN) {
            // Fast fade from black
            float fadeProgress = (float)(currentTime - transitionStartTime) / FADE_DURATION;
            currentBrightness = 0.7f * fadeProgress;
            if (currentBrightness > 0.7f) currentBrightness = 0.7f;
        } else {
            // Normal smooth fade (e.g., after muzzle flash)
            if (currentBrightness > targetBrightness) {
                currentBrightness -= 0.05f;
                if (currentBrightness < targetBrightness) {
                    currentBrightness = targetBrightness;
                }
            } else if (currentBrightness < targetBrightness) {
                currentBrightness += 0.05f;
                if (currentBrightness > targetBrightness) {
                    currentBrightness = targetBrightness;
                }
            }
        }

        // Update cosmic unicorn brightness
        if (cosmic) {
            cosmic->set_brightness(currentBrightness);
        }

        return !shouldExit;  // Return false to exit
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Draw from back to front (painter's algorithm)
        drawSkyAndFloor(graphics);
        drawWalls(graphics);
        drawParticles(graphics);  // Draw explosion particles
        drawPowerUps(graphics);    // Draw power-ups in 3D view
        drawEnemies(graphics);     // Draw enemies in 3D view
        drawBullets(graphics);     // Draw bullets in 3D view
        drawHUD(graphics);

        // Draw lightning effect on top of everything except HUD
        lightning.render(&graphics);
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                     bool button_vol_up, bool button_vol_down,
                     bool button_bright_up, bool button_bright_down) override {
        // Only exit on B button to avoid interfering with game
        if (button_b) {
            shouldExit = true;
        }

        // Button D to cycle weapons (for testing)
        if (button_d && !buttonDPressed) {
            weaponType = (weaponType + 1) % 5;  // 5 weapons (0-4)
            buttonDPressed = true;
        } else if (!button_d) {
            buttonDPressed = false;
        }
    }
};
