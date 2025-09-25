#pragma once

#include "../game_base.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace pimoroni;

class SideScrollerGame : public GameBase {
private:
    static constexpr int DISPLAY_WIDTH = 32;
    static constexpr int DISPLAY_HEIGHT = 32;
    static constexpr int MAX_BULLETS = 20;
    static constexpr int MAX_ENEMY_BULLETS = 15;
    static constexpr int MAX_ENEMIES = 8;
    static constexpr int MAX_SWARM_ENEMIES = 16;
    static constexpr int MAX_PARTICLES = 50;
    static constexpr int MAX_POWERUPS = 3;
    
    // Perlin noise implementation for terrain
    class PerlinNoise {
    private:
        static const int PERM_SIZE = 256;
        int perm[PERM_SIZE * 2];
        
        float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
        float lerp(float t, float a, float b) { return a + t * (b - a); }
        float grad(int hash, float x, float y) {
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }
        
    public:
        PerlinNoise() {
            // Initialize permutation table
            for (int i = 0; i < PERM_SIZE; i++) {
                perm[i] = i;
            }
            // Shuffle using simple LCG random
            for (int i = 0; i < PERM_SIZE; i++) {
                int j = (i * 17 + 19) % PERM_SIZE;
                int temp = perm[i];
                perm[i] = perm[j];
                perm[j] = temp;
            }
            // Duplicate for easy wrap-around
            for (int i = 0; i < PERM_SIZE; i++) {
                perm[PERM_SIZE + i] = perm[i];
            }
        }
        
        float noise(float x, float y) {
            int X = (int)floor(x) & 255;
            int Y = (int)floor(y) & 255;
            
            x -= floor(x);
            y -= floor(y);
            
            float u = fade(x);
            float v = fade(y);
            
            int A = perm[X] + Y;
            int AA = perm[A];
            int AB = perm[A + 1];
            int B = perm[X + 1] + Y;
            int BA = perm[B];
            int BB = perm[B + 1];
            
            return lerp(v, lerp(u, grad(perm[AA], x, y), grad(perm[BA], x - 1, y)),
                          lerp(u, grad(perm[AB], x, y - 1), grad(perm[BB], x - 1, y - 1)));
        }
    };
    
    // Game objects
    struct Player {
        float x = 4.0f, y = 16.0f;
        int health = 100;
        int weapon_type = 0; // 0=single, 1=triple, 2=missile, 3=quad_directional
        uint32_t last_shot = 0;
        uint32_t invulnerable_until = 0;
        bool alive = true;
        
        // Weapon stats
        int single_shot_delay = 150;  // ms
        int triple_shot_delay = 200;  // ms  
        int missile_shot_delay = 400; // ms
        int quad_shot_delay = 300;    // ms
    } player;
    
    struct Bullet {
        float x, y, vx, vy;
        float prev_x, prev_y; // Previous position for line drawing
        int type; // 0=normal, 1=triple, 2=missile, 3=quad_directional
        bool active = false;
        uint32_t created_time;
        
        // Enhanced trail effect - store more positions for better visibility
        static const int MAX_TRAIL_LENGTH = 12;
        float trail_x[MAX_TRAIL_LENGTH], trail_y[MAX_TRAIL_LENGTH];
        int trail_length = 0;
        
        void addTrailPoint(float new_x, float new_y) {
            // Shift existing trail points
            for (int i = trail_length - 1; i > 0; i--) {
                trail_x[i] = trail_x[i-1];
                trail_y[i] = trail_y[i-1];
            }
            
            // Add new point
            trail_x[0] = new_x;
            trail_y[0] = new_y;
            
            if (trail_length < MAX_TRAIL_LENGTH) {
                trail_length++;
            }
        }
    };
    
    struct Enemy {
        float x, y, vx, vy;
        int health;
        int type; // 0=basic, 1=fast, 2=tank, 3=shooter
        bool active = false;
        uint32_t last_shot = 0;
        uint32_t ai_timer = 0;
        float ai_phase = 0;
    };
    
    // Swarm Enemy with flocking behavior
    struct SwarmEnemy {
        float x, y;           // Position
        float vx, vy;         // Velocity
        float max_speed;      // Maximum speed
        float max_force;      // Maximum steering force
        int health;
        int type;             // 0=drone, 1=defensive, 2=aggressive
        bool active = false;
        uint32_t last_shot = 0;
        uint32_t ai_timer = 0;
        float ai_phase = 0;
        int swarm_id;         // Which swarm group this belongs to
        float wing_phase;     // For animation effects
        
        SwarmEnemy() : x(0), y(0), vx(0), vy(0), max_speed(1.2f), max_force(0.05f),
                      health(1), type(0), swarm_id(0), wing_phase(0) {}
    };
    
    struct EnemyBullet {
        float x, y, vx, vy;
        bool active = false;
    };
    
    struct Particle {
        float x, y, vx, vy;
        float life, max_life;
        uint8_t r, g, b;
        int type; // 0=explosion, 1=engine_exhaust, 2=spark
        bool active = false;
    };
    
    struct PowerUp {
        float x, y;
        int type; // 0=weapon, 1=health, 2=speed
        bool active = false;
        float anim_phase = 0;
    };
    
    // Game arrays
    Bullet bullets[MAX_BULLETS];
    EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    SwarmEnemy swarm_enemies[MAX_SWARM_ENEMIES];
    Particle particles[MAX_PARTICLES];
    PowerUp powerups[MAX_POWERUPS];
    
    // Theme system
    enum Theme {
        SPACE_BLUE = 0,
        VOLCANIC_RED,
        FOREST_GREEN,
        ICE_CYAN,
        DESERT_ORANGE,
        PURPLE_NEBULA,
        THEME_COUNT
    };
    
    struct ThemeColors {
        // Nebula background colors
        uint8_t nebula_r1, nebula_g1, nebula_b1;
        uint8_t nebula_r2, nebula_g2, nebula_b2;
        uint8_t nebula_r3, nebula_g3, nebula_b3;
        // Terrain colors
        uint8_t floor_r, floor_g, floor_b;
        uint8_t ceiling_r, ceiling_g, ceiling_b;
        uint8_t highlight_r, highlight_g, highlight_b;
        // Terrain shape parameters
        float terrain_amplitude;    // Height variation multiplier
        float terrain_frequency;    // Frequency of terrain features
        float terrain_roughness;    // How jagged the terrain is
        float floor_bias;          // Base floor height adjustment
        float ceiling_bias;        // Base ceiling height adjustment
        // Distance required for this theme
        float distance_threshold;
    };
    
    ThemeColors themes[THEME_COUNT] = {
        // SPACE_BLUE - Standard rolling hills
        {60, 40, 120, 80, 60, 180, 120, 80, 200, 120, 80, 60, 100, 100, 120, 220, 180, 120, 
         3.0f, 0.1f, 1.0f, 0.0f, 0.0f, 0.0f},
        
        // VOLCANIC_RED - Jagged volcanic peaks
        {120, 40, 40, 180, 60, 40, 200, 80, 60, 140, 70, 30, 120, 60, 40, 255, 150, 80, 
         5.0f, 0.15f, 2.0f, 1.0f, -1.0f, 1000.0f},
        
        // FOREST_GREEN - Gentle rolling hills
        {40, 120, 40, 60, 160, 80, 80, 180, 100, 60, 100, 40, 80, 120, 60, 150, 220, 120, 
         2.0f, 0.08f, 0.5f, -0.5f, 0.5f, 2000.0f},
        
        // ICE_CYAN - Sharp crystalline terrain
        {60, 120, 180, 80, 160, 220, 100, 180, 255, 120, 140, 160, 140, 160, 180, 200, 240, 255, 
         4.0f, 0.2f, 1.8f, 0.5f, -0.5f, 3000.0f},
        
        // DESERT_ORANGE - Large smooth dunes
        {180, 120, 60, 220, 160, 80, 255, 180, 100, 160, 120, 80, 140, 100, 60, 255, 220, 150, 
         6.0f, 0.05f, 0.3f, 2.0f, 1.0f, 4000.0f},
        
        // PURPLE_NEBULA - Dramatic spiky terrain
        {120, 60, 160, 160, 80, 200, 200, 100, 240, 80, 40, 120, 100, 60, 140, 180, 120, 255, 
         7.0f, 0.25f, 2.5f, 1.5f, -1.5f, 5000.0f}
    };
    
    // Game state
    float scroll_x = 0;
    float total_distance = 0; // Track total distance traveled
    Theme current_theme = SPACE_BLUE;
    uint32_t game_time = 0;
    uint32_t last_enemy_spawn = 0;
    uint32_t last_swarm_spawn = 0;
    int next_swarm_id = 0;
    uint32_t score = 0;
    bool game_over = false;
    uint32_t game_over_time = 0;
    bool demo_mode = true;
    bool button_d_pressed = false;
    
    // Demo AI state
    float demo_target_y = 16.0f;
    uint32_t demo_weapon_change_time = 0;
    uint32_t demo_last_dodge = 0;
    uint32_t mode_switch_time = 0;
    
    // Terrain system
    PerlinNoise noise;
    float terrain_offset = 0;
    
    // Visual effects
    float screen_shake = 0;
    uint32_t last_update_time = 0;
    
    void hsv_to_rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
        float c = v * s;
        float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
        float m = v - c;
        
        float r_prime, g_prime, b_prime;
        
        if (h >= 0 && h < 60) {
            r_prime = c; g_prime = x; b_prime = 0;
        } else if (h >= 60 && h < 120) {
            r_prime = x; g_prime = c; b_prime = 0;
        } else if (h >= 120 && h < 180) {
            r_prime = 0; g_prime = c; b_prime = x;
        } else if (h >= 180 && h < 240) {
            r_prime = 0; g_prime = x; b_prime = c;
        } else if (h >= 240 && h < 300) {
            r_prime = x; g_prime = 0; b_prime = c;
        } else {
            r_prime = c; g_prime = 0; b_prime = x;
        }
        
        r = (uint8_t)((r_prime + m) * 255);
        g = (uint8_t)((g_prime + m) * 255);
        b = (uint8_t)((b_prime + m) * 255);
    }
    
    void createExplosion(float x, float y, int intensity = 10) {
        screen_shake = 0.0f;
        for (int i = 0; i < intensity; i++) {
            for (int p = 0; p < MAX_PARTICLES; p++) {
                if (!particles[p].active) {
                    particles[p].x = x + (rand() % 6 - 3);
                    particles[p].y = y + (rand() % 6 - 3);
                    particles[p].vx = (rand() % 200 - 100) / 50.0f;
                    particles[p].vy = (rand() % 200 - 100) / 50.0f;
                    particles[p].life = (rand() % 300 + 200) / 1000.0f;
                    particles[p].max_life = particles[p].life;
                    particles[p].type = 0; // explosion
                    
                    // Explosion colors - reds, oranges, yellows
                    float hue = rand() % 60; // 0-60 degrees for red-orange-yellow
                    hsv_to_rgb(hue, 1.0f, 1.0f, particles[p].r, particles[p].g, particles[p].b);
                    particles[p].active = true;
                    break;
                }
            }
        }
    }
    
    void createEngineExhaust() {
        // Create engine particles behind player
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (!particles[p].active) {
                particles[p].x = player.x - 1 - rand() % 2;
                particles[p].y = player.y + (rand() % 3 - 1);
                particles[p].vx = -(rand() % 100 + 50) / 50.0f;
                particles[p].vy = (rand() % 60 - 30) / 100.0f;
                particles[p].life = (rand() % 150 + 100) / 1000.0f;
                particles[p].max_life = particles[p].life;
                particles[p].type = 1; // engine exhaust
                
                // Brighter blue exhaust colors
                particles[p].r = 80 + rand() % 70;
                particles[p].g = 130 + rand() % 70;
                particles[p].b = 220 + rand() % 35;
                particles[p].active = true;
                break;
            }
        }
    }
    
    void fireBullet(float x, float y, float vx, float vy, int type = 0) {
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) {
                bullets[b].x = x;
                bullets[b].y = y;
                bullets[b].prev_x = x;
                bullets[b].prev_y = y;
                bullets[b].vx = vx;
                bullets[b].vy = vy;
                bullets[b].type = type;
                bullets[b].active = true;
                bullets[b].created_time = game_time;
                bullets[b].trail_length = 0;
                break;
            }
        }
    }
    
    void fireEnemyBullet(float x, float y, float vx, float vy) {
        for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
            if (!enemy_bullets[b].active) {
                enemy_bullets[b].x = x;
                enemy_bullets[b].y = y;
                enemy_bullets[b].vx = vx;
                enemy_bullets[b].vy = vy;
                enemy_bullets[b].active = true;
                break;
            }
        }
    }
    
    // Swarm flocking behavior functions (based on Dan Shiffman's boids)
    std::pair<float, float> swarmSeparate(const SwarmEnemy& swarm_enemy) {
        float desired_separation = 2.5f;
        float steer_x = 0, steer_y = 0;
        int count = 0;
        
        for (int i = 0; i < MAX_SWARM_ENEMIES; i++) {
            if (!swarm_enemies[i].active) continue;
            if (&swarm_enemies[i] == &swarm_enemy) continue;
            
            float dx = swarm_enemy.x - swarm_enemies[i].x;
            float dy = swarm_enemy.y - swarm_enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist > 0 && dist < desired_separation) {
                dx /= dist;
                dy /= dist;
                dx /= dist; // Weight by distance
                dy /= dist;
                
                steer_x += dx;
                steer_y += dy;
                count++;
            }
        }
        
        if (count > 0) {
            steer_x /= count;
            steer_y /= count;
            
            // Normalize and scale to max force
            float mag = sqrt(steer_x * steer_x + steer_y * steer_y);
            if (mag > 0) {
                steer_x = (steer_x / mag) * swarm_enemy.max_speed;
                steer_y = (steer_y / mag) * swarm_enemy.max_speed;
                
                // Steering = Desired - Velocity
                steer_x -= swarm_enemy.vx;
                steer_y -= swarm_enemy.vy;
                
                // Limit to max force
                mag = sqrt(steer_x * steer_x + steer_y * steer_y);
                if (mag > swarm_enemy.max_force) {
                    steer_x = (steer_x / mag) * swarm_enemy.max_force;
                    steer_y = (steer_y / mag) * swarm_enemy.max_force;
                }
            }
        }
        
        return {steer_x, steer_y};
    }
    
    std::pair<float, float> swarmAlign(const SwarmEnemy& swarm_enemy) {
        float neighbor_dist = 6.0f;
        float sum_vx = 0, sum_vy = 0;
        int count = 0;
        
        for (int i = 0; i < MAX_SWARM_ENEMIES; i++) {
            if (!swarm_enemies[i].active) continue;
            if (&swarm_enemies[i] == &swarm_enemy) continue;
            if (swarm_enemies[i].swarm_id != swarm_enemy.swarm_id) continue; // Only align with same swarm
            
            float dx = swarm_enemy.x - swarm_enemies[i].x;
            float dy = swarm_enemy.y - swarm_enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist > 0 && dist < neighbor_dist) {
                sum_vx += swarm_enemies[i].vx;
                sum_vy += swarm_enemies[i].vy;
                count++;
            }
        }
        
        if (count > 0) {
            sum_vx /= count;
            sum_vy /= count;
            
            // Normalize and scale to max speed
            float mag = sqrt(sum_vx * sum_vx + sum_vy * sum_vy);
            if (mag > 0) {
                sum_vx = (sum_vx / mag) * swarm_enemy.max_speed;
                sum_vy = (sum_vy / mag) * swarm_enemy.max_speed;
                
                // Steering = Desired - Velocity
                sum_vx -= swarm_enemy.vx;
                sum_vy -= swarm_enemy.vy;
                
                // Limit to max force
                mag = sqrt(sum_vx * sum_vx + sum_vy * sum_vy);
                if (mag > swarm_enemy.max_force) {
                    sum_vx = (sum_vx / mag) * swarm_enemy.max_force;
                    sum_vy = (sum_vy / mag) * swarm_enemy.max_force;
                }
            }
        }
        
        return {sum_vx, sum_vy};
    }
    
    std::pair<float, float> swarmCohesion(const SwarmEnemy& swarm_enemy) {
        float neighbor_dist = 6.0f;
        float sum_x = 0, sum_y = 0;
        int count = 0;
        
        for (int i = 0; i < MAX_SWARM_ENEMIES; i++) {
            if (!swarm_enemies[i].active) continue;
            if (&swarm_enemies[i] == &swarm_enemy) continue;
            if (swarm_enemies[i].swarm_id != swarm_enemy.swarm_id) continue; // Only cohere with same swarm
            
            float dx = swarm_enemy.x - swarm_enemies[i].x;
            float dy = swarm_enemy.y - swarm_enemies[i].y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist > 0 && dist < neighbor_dist) {
                sum_x += swarm_enemies[i].x;
                sum_y += swarm_enemies[i].y;
                count++;
            }
        }
        
        if (count > 0) {
            sum_x /= count;
            sum_y /= count;
            
            return swarmSeek(swarm_enemy, sum_x, sum_y);
        }
        
        return {0, 0};
    }
    
    std::pair<float, float> swarmSeek(const SwarmEnemy& swarm_enemy, float target_x, float target_y) {
        float dx = target_x - swarm_enemy.x;
        float dy = target_y - swarm_enemy.y;
        float dist = sqrt(dx * dx + dy * dy);
        
        if (dist > 0) {
            dx = (dx / dist) * swarm_enemy.max_speed;
            dy = (dy / dist) * swarm_enemy.max_speed;
            
            // Steering = Desired - Velocity
            dx -= swarm_enemy.vx;
            dy -= swarm_enemy.vy;
            
            // Limit to max force
            float mag = sqrt(dx * dx + dy * dy);
            if (mag > swarm_enemy.max_force) {
                dx = (dx / mag) * swarm_enemy.max_force;
                dy = (dy / mag) * swarm_enemy.max_force;
            }
        }
        
        return {dx, dy};
    }
    
    std::pair<float, float> swarmBoundaryForce(const SwarmEnemy& swarm_enemy) {
        float force_x = 0, force_y = 0;
        float boundary_distance = 4.0f;
        
        // Left boundary
        if (swarm_enemy.x < boundary_distance) {
            force_x += (boundary_distance - swarm_enemy.x) * 0.1f;
        }
        // Right boundary
        if (swarm_enemy.x > DISPLAY_WIDTH - boundary_distance) {
            force_x -= (swarm_enemy.x - (DISPLAY_WIDTH - boundary_distance)) * 0.1f;
        }
        // Top boundary
        if (swarm_enemy.y < boundary_distance) {
            force_y += (boundary_distance - swarm_enemy.y) * 0.1f;
        }
        // Bottom boundary
        if (swarm_enemy.y > DISPLAY_HEIGHT - boundary_distance) {
            force_y -= (swarm_enemy.y - (DISPLAY_HEIGHT - boundary_distance)) * 0.1f;
        }
        
        return {force_x, force_y};
    }
    
    void spawnSwarm(int count, int type, int swarm_id, float spawn_x, float spawn_y) {
        for (int i = 0; i < count && i < MAX_SWARM_ENEMIES; i++) {
            for (int s = 0; s < MAX_SWARM_ENEMIES; s++) {
                if (!swarm_enemies[s].active) {
                    swarm_enemies[s].x = spawn_x + (rand() % 6 - 3);
                    swarm_enemies[s].y = spawn_y + (rand() % 6 - 3);
                    swarm_enemies[s].vx = -0.5f + (rand() % 100 - 50) / 100.0f;
                    swarm_enemies[s].vy = (rand() % 100 - 50) / 100.0f;
                    swarm_enemies[s].type = type;
                    swarm_enemies[s].swarm_id = swarm_id;
                    swarm_enemies[s].ai_phase = rand() % 628 / 100.0f;
                    swarm_enemies[s].wing_phase = rand() % 628 / 100.0f;
                    swarm_enemies[s].active = true;
                    
                    switch (type) {
                        case 0: // Drone swarm
                            swarm_enemies[s].health = 1;
                            swarm_enemies[s].max_speed = 1.0f;
                            swarm_enemies[s].max_force = 0.04f;
                            break;
                        case 1: // Defensive swarm
                            swarm_enemies[s].health = 1;
                            swarm_enemies[s].max_speed = 0.8f;
                            swarm_enemies[s].max_force = 0.06f;
                            break;
                        case 2: // Aggressive swarm
                            swarm_enemies[s].health = 1;
                            swarm_enemies[s].max_speed = 1.4f;
                            swarm_enemies[s].max_force = 0.05f;
                            break;
                    }
                    break;
                }
            }
        }
    }
    
    void spawnEnemy(int type = -1) {
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) {
                if (type == -1) type = rand() % 4;
                
                enemies[e].x = DISPLAY_WIDTH + 2;
                enemies[e].y = 3 + rand() % (DISPLAY_HEIGHT - 6);
                enemies[e].type = type;
                enemies[e].ai_phase = rand() % 628 / 100.0f;
                enemies[e].active = true;
                
                switch (type) {
                    case 0: // Basic enemy
                        enemies[e].health = 1;
                        enemies[e].vx = -1.0f;
                        enemies[e].vy = 0;
                        break;
                    case 1: // Fast enemy
                        enemies[e].health = 1;
                        enemies[e].vx = -2.0f;
                        enemies[e].vy = sin(enemies[e].ai_phase) * 0.5f;
                        break;
                    case 2: // Tank enemy
                        enemies[e].health = 3;
                        enemies[e].vx = -0.5f;
                        enemies[e].vy = 0;
                        break;
                    case 3: // Shooter enemy
                        enemies[e].health = 2;
                        enemies[e].vx = -0.8f;
                        enemies[e].vy = 0;
                        break;
                }
                break;
            }
        }
    }
    
    void spawnPowerUp(float x, float y) {
        for (int p = 0; p < MAX_POWERUPS; p++) {
            if (!powerups[p].active) {
                powerups[p].x = x;
                powerups[p].y = y;
                powerups[p].type = rand() % 3;
                powerups[p].active = true;
                powerups[p].anim_phase = 0;
                break;
            }
        }
    }
    
    void updateTheme() {
        // Check if we should advance to next theme based on distance
        for (int i = THEME_COUNT - 1; i >= 0; i--) {
            if (total_distance >= themes[i].distance_threshold) {
                current_theme = (Theme)i;
                break;
            }
        }
    }
    
    void updateTerrain() {
        terrain_offset += 0.02f; // Scroll speed
        
        // Track distance traveled - faster progression in demo mode
        if (demo_mode) {
            total_distance += 2.0f;  // 4x faster theme changes in demo mode
        } else {
            total_distance += 0.5f;  // Normal speed for player mode
        }
        
        updateTheme();
    }
    
    void drawTerrain() {
        const ThemeColors& theme = themes[current_theme];
        
        // Draw scrolling floor and ceiling with theme-specific Perlin noise variations
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            // Theme-based terrain calculations
            float noise_x = (x + terrain_offset * 50) * theme.terrain_frequency;
            
            // Base noise with theme-specific amplitude
            float floor_noise = noise.noise(noise_x, 0) * theme.terrain_amplitude;
            float ceiling_noise = noise.noise(noise_x, 10) * theme.terrain_amplitude;
            
            // Add roughness for jagged terrain effects
            if (theme.terrain_roughness > 1.0f) {
                floor_noise += noise.noise(noise_x * 2.0f, 0.5f) * (theme.terrain_roughness - 1.0f);
                ceiling_noise += noise.noise(noise_x * 2.0f, 10.5f) * (theme.terrain_roughness - 1.0f);
            }
            
            // Apply theme-specific bias and calculate final heights
            int floor_height = (int)(floor_noise + 3 + theme.floor_bias);
            int ceiling_height = (int)(ceiling_noise + 3 + theme.ceiling_bias);
            
            // Clamp heights to reasonable bounds
            floor_height = std::max(1, std::min(floor_height, 8));
            ceiling_height = std::max(1, std::min(ceiling_height, 8));
            
            // Draw floor with theme colors
            for (int y = DISPLAY_HEIGHT - floor_height; y < DISPLAY_HEIGHT; y++) {
                float depth = (float)(y - (DISPLAY_HEIGHT - floor_height)) / floor_height;
                float intensity = 0.6f + depth * 0.4f;
                
                uint8_t r = (uint8_t)(theme.floor_r * intensity);
                uint8_t g = (uint8_t)(theme.floor_g * intensity);
                uint8_t b = (uint8_t)(theme.floor_b * intensity);
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
            
            // Draw ceiling with theme colors
            for (int y = 0; y < ceiling_height; y++) {
                float depth = (float)(ceiling_height - y) / ceiling_height;
                float intensity = 0.6f + depth * 0.4f;
                
                uint8_t r = (uint8_t)(theme.ceiling_r * intensity);
                uint8_t g = (uint8_t)(theme.ceiling_g * intensity);
                uint8_t b = (uint8_t)(theme.ceiling_b * intensity);
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
        }
        
        // Add some texture/detail to walls with theme colors
        for (int x = 0; x < DISPLAY_WIDTH; x += 4) {
            float detail_noise = noise.noise((x + terrain_offset * 30) * theme.terrain_frequency * 2.0f, 5) * theme.terrain_roughness;
            if (detail_noise > 1.0f) {
                // Recalculate floor/ceiling positions for highlights using theme parameters
                float noise_x = (x + terrain_offset * 50) * theme.terrain_frequency;
                float floor_noise = noise.noise(noise_x, 0) * theme.terrain_amplitude;
                float ceiling_noise = noise.noise(noise_x, 10) * theme.terrain_amplitude;
                
                if (theme.terrain_roughness > 1.0f) {
                    floor_noise += noise.noise(noise_x * 2.0f, 0.5f) * (theme.terrain_roughness - 1.0f);
                    ceiling_noise += noise.noise(noise_x * 2.0f, 10.5f) * (theme.terrain_roughness - 1.0f);
                }
                
                int floor_base = DISPLAY_HEIGHT - (int)(floor_noise + 5 + theme.floor_bias);
                int ceiling_base = (int)(ceiling_noise + 5 + theme.ceiling_bias);
                
                // Theme-based highlights
                gfx->set_pen(theme.highlight_r, theme.highlight_g, theme.highlight_b);
                if (floor_base >= 0 && floor_base < DISPLAY_HEIGHT - 1) {
                    gfx->pixel(Point(x, floor_base));
                }
                if (ceiling_base >= 1 && ceiling_base < DISPLAY_HEIGHT) {
                    gfx->pixel(Point(x, ceiling_base));
                }
            }
        }
    }
    
    void drawNebulaBackground() {
        const ThemeColors& theme = themes[current_theme];
        // Multi-layered nebula with subtle animation
        float time = game_time * 0.0005f; // Very slow animation
        
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                // Create multiple layers of noise for nebula effect
                float noise1 = noise.noise((x + scroll_x * 0.1f) * 0.05f, y * 0.05f + time * 0.3f);
                float noise2 = noise.noise((x + scroll_x * 0.05f) * 0.08f, y * 0.08f - time * 0.2f);
                float noise3 = noise.noise((x + scroll_x * 0.02f) * 0.12f, y * 0.12f + time * 0.1f);
                
                // Combine noise layers with different weights
                float nebula = (noise1 * 0.5f + noise2 * 0.3f + noise3 * 0.2f) * 0.4f + 0.1f;
                nebula = nebula < 0 ? 0 : nebula;
                nebula = nebula > 0.5f ? 0.5f : nebula;
                
                if (nebula > 0.05f) {
                    // Create color variations across the nebula using theme colors
                    float color_shift = noise.noise(x * 0.03f, y * 0.03f + time * 0.1f);
                    
                    // Use theme-based nebula colors
                    uint8_t r, g, b;
                    if (color_shift > 0.3f) {
                        // Warmer regions - use first nebula color
                        r = (uint8_t)(nebula * theme.nebula_r1 / 255.0f * 255);
                        g = (uint8_t)(nebula * theme.nebula_g1 / 255.0f * 255);
                        b = (uint8_t)(nebula * theme.nebula_b1 / 255.0f * 255);
                    } else if (color_shift > -0.2f) {
                        // Cool regions - use second nebula color
                        r = (uint8_t)(nebula * theme.nebula_r2 / 255.0f * 255);
                        g = (uint8_t)(nebula * theme.nebula_g2 / 255.0f * 255);
                        b = (uint8_t)(nebula * theme.nebula_b2 / 255.0f * 255);
                    } else {
                        // Deep regions - use third nebula color
                        r = (uint8_t)(nebula * theme.nebula_r3 / 255.0f * 255);
                        g = (uint8_t)(nebula * theme.nebula_g3 / 255.0f * 255);
                        b = (uint8_t)(nebula * theme.nebula_b3 / 255.0f * 255);
                    }
                    
                    gfx->set_pen(r, g, b);
                    gfx->pixel(Point(x, y));
                }
            }
        }
    }
    
    void updateDemoAI(float dt) {
        if (!demo_mode || !player.alive) return;
        
        // Demo AI movement - direct position updates for smooth movement
        const float move_speed = 0.05f;
        
        // Vertical movement towards target
        float y_diff = demo_target_y - player.y;
        if (abs(y_diff) > 0.2f) {
            if (y_diff > 0) {
                player.y += move_speed;
            } else {
                player.y -= move_speed;
            }
        }
        
        // Horizontal movement - stay in left third but move around
        float target_x = 6.0f + sin(game_time * 0.001f) * 3.0f;
        float x_diff = target_x - player.x;
        if (abs(x_diff) > 0.2f) {
            if (x_diff > 0) {
                player.x += move_speed * 0.5f;
            } else {
                player.x -= move_speed * 0.5f;
            }
        }
        
        // Dodge enemy bullets
        for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
            if (enemy_bullets[b].active) {
                float bullet_dist = sqrt((enemy_bullets[b].x - player.x) * (enemy_bullets[b].x - player.x) + 
                                       (enemy_bullets[b].y - player.y) * (enemy_bullets[b].y - player.y));
                if (bullet_dist < 8 && enemy_bullets[b].x > player.x && game_time - demo_last_dodge > 1000) {
                    // Dodge up or down randomly
                    demo_target_y = (rand() % 2) ? 8.0f : 24.0f;
                    demo_last_dodge = game_time;
                }
            }
        }
        
        // Change target occasionally
        if (game_time % 3000 < 100) {
            demo_target_y = 8 + rand() % 16;
        }
        
        // Auto-shoot
        uint32_t shot_delay;
        switch (player.weapon_type) {
            case 1: shot_delay = player.triple_shot_delay; break;
            case 2: shot_delay = player.missile_shot_delay; break;
            case 3: shot_delay = player.quad_shot_delay; break;
            default: shot_delay = player.single_shot_delay; break;
        }
        
        if (game_time - player.last_shot > shot_delay) {
            switch (player.weapon_type) {
                case 0: // Single shot
                    fireBullet(player.x + 2, player.y, 4.0f, 0, 0);
                    break;
                case 1: // Triple shot
                    fireBullet(player.x + 2, player.y, 4.0f, 0, 1);
                    fireBullet(player.x + 2, player.y - 1, 4.0f, -0.5f, 1);
                    fireBullet(player.x + 2, player.y + 1, 4.0f, 0.5f, 1);
                    break;
                case 2: // Missiles
                    fireBullet(player.x + 2, player.y, 3.0f, 0, 2);
                    break;
                case 3: // Quad-directional
                    fireBullet(player.x + 2, player.y, 3.5f, 0, 3);       // East (right)
                    fireBullet(player.x, player.y, -2.5f, 0, 3);          // West (left)  
                    fireBullet(player.x, player.y - 1, 0, -3.0f, 3);      // North (up)
                    fireBullet(player.x, player.y + 1, 0, 3.0f, 3);       // South (down)
                    break;
            }
            player.last_shot = game_time;
        }
        
        // Change weapons periodically
        if (game_time - demo_weapon_change_time > 8000) {
            player.weapon_type = (player.weapon_type + 1) % 4;
            demo_weapon_change_time = game_time;
        }
    }

    void updatePlayer(float dt) {
        if (!player.alive) return;
        
        // Keep player on screen boundaries
        if (player.x < 1) player.x = 1;
        if (player.x > DISPLAY_WIDTH - 2) player.x = DISPLAY_WIDTH - 2;
        if (player.y < 1) player.y = 1;
        if (player.y > DISPLAY_HEIGHT - 2) player.y = DISPLAY_HEIGHT - 2;
        
        // Create engine exhaust particles
        if (rand() % 3 == 0) {
            createEngineExhaust();
        }
    }
    
    void updateBullets(float dt) {
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (bullets[b].active) {
                // Store previous position
                bullets[b].prev_x = bullets[b].x;
                bullets[b].prev_y = bullets[b].y;
                
                // Update position - slower for better trail visibility
                bullets[b].x += bullets[b].vx * dt * 5.0f;  // Reduced from 30.0f
                bullets[b].y += bullets[b].vy * dt * 5.0f;
                
                // Add current position to trail
                bullets[b].addTrailPoint(bullets[b].x, bullets[b].y);
                
                // Missile homing behavior
                if (bullets[b].type == 2) {
                    // Find nearest enemy
                    float nearest_dist = 1000;
                    int nearest_enemy = -1;
                    for (int e = 0; e < MAX_ENEMIES; e++) {
                        if (enemies[e].active) {
                            float dist = sqrt((enemies[e].x - bullets[b].x) * (enemies[e].x - bullets[b].x) + 
                                            (enemies[e].y - bullets[b].y) * (enemies[e].y - bullets[b].y));
                            if (dist < nearest_dist) {
                                nearest_dist = dist;
                                nearest_enemy = e;
                            }
                        }
                    }
                    
                    // Home towards nearest enemy
                    if (nearest_enemy != -1 && nearest_dist < 15) {
                        float dx = enemies[nearest_enemy].x - bullets[b].x;
                        float dy = enemies[nearest_enemy].y - bullets[b].y;
                        float len = sqrt(dx * dx + dy * dy);
                        if (len > 0) {
                            bullets[b].vx += (dx / len) * 2.0f * dt;
                            bullets[b].vy += (dy / len) * 2.0f * dt;
                        }
                    }
                }
                
                // Remove bullets that go off screen
                if (bullets[b].x > DISPLAY_WIDTH + 5 || bullets[b].x < -5 || 
                    bullets[b].y > DISPLAY_HEIGHT + 5 || bullets[b].y < -5) {
                    bullets[b].active = false;
                }
            }
        }
        
        // Update enemy bullets
        for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
            if (enemy_bullets[b].active) {
                enemy_bullets[b].x += enemy_bullets[b].vx * dt * 18.0f;  // Reduced from 25.0f
                enemy_bullets[b].y += enemy_bullets[b].vy * dt * 18.0f;
                
                if (enemy_bullets[b].x < -5 || enemy_bullets[b].x > DISPLAY_WIDTH + 5 ||
                    enemy_bullets[b].y < -5 || enemy_bullets[b].y > DISPLAY_HEIGHT + 5) {
                    enemy_bullets[b].active = false;
                }
            }
        }
    }
    
    void updateEnemies(float dt) {
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (enemies[e].active) {
                enemies[e].ai_timer += (uint32_t)(dt * 1000);
                enemies[e].ai_phase += dt * 2.0f;
                
                // Update movement based on type
                switch (enemies[e].type) {
                    case 1: // Fast zigzag
                        enemies[e].vy = sin(enemies[e].ai_phase) * 1.5f;
                        break;
                    case 3: // Shooter - shoot at player
                        if (enemies[e].ai_timer > 800 && abs(enemies[e].y - player.y) < 8) {
                            float dx = player.x - enemies[e].x;
                            float dy = player.y - enemies[e].y;
                            float len = sqrt(dx * dx + dy * dy);
                            if (len > 0) {
                                fireEnemyBullet(enemies[e].x - 1, enemies[e].y, 
                                              (dx / len) * -3.0f, (dy / len) * -3.0f);
                            }
                            enemies[e].ai_timer = 0;
                        }
                        break;
                }
                
                enemies[e].x += enemies[e].vx * dt * 20.0f;
                enemies[e].y += enemies[e].vy * dt * 20.0f;
                
                // Remove enemies that go off screen
                if (enemies[e].x < -5) {
                    enemies[e].active = false;
                }
            }
        }
    }
    
    void updateSwarmEnemies(float dt) {
        for (int s = 0; s < MAX_SWARM_ENEMIES; s++) {
            if (!swarm_enemies[s].active) continue;
            
            // Update animation phases
            swarm_enemies[s].ai_timer += (uint32_t)(dt * 1000);
            swarm_enemies[s].ai_phase += dt * 3.0f;
            swarm_enemies[s].wing_phase += dt * 8.0f;
            
            // Calculate flocking forces
            auto sep = swarmSeparate(swarm_enemies[s]);
            auto ali = swarmAlign(swarm_enemies[s]);
            auto coh = swarmCohesion(swarm_enemies[s]);
            auto bounds = swarmBoundaryForce(swarm_enemies[s]);
            auto seek_player = swarmSeek(swarm_enemies[s], player.x, player.y);
            
            // Weight forces based on enemy type
            float sep_weight, ali_weight, coh_weight, seek_weight;
            switch (swarm_enemies[s].type) {
                case 0: // Drone swarm - balanced flocking
                    sep_weight = 1.5f; ali_weight = 1.0f; coh_weight = 1.0f; seek_weight = 0.3f;
                    break;
                case 1: // Defensive swarm - stay together, avoid player
                    sep_weight = 1.0f; ali_weight = 1.5f; coh_weight = 2.0f; seek_weight = -0.5f;
                    break;
                case 2: // Aggressive swarm - seek player more aggressively
                    sep_weight = 1.2f; ali_weight = 0.8f; coh_weight = 0.8f; seek_weight = 0.8f;
                    break;
                default:
                    sep_weight = 1.0f; ali_weight = 1.0f; coh_weight = 1.0f; seek_weight = 0.5f;
                    break;
            }
            
            // Apply forces
            swarm_enemies[s].vx += sep.first * sep_weight + ali.first * ali_weight + 
                                  coh.first * coh_weight + bounds.first * 2.0f + 
                                  seek_player.first * seek_weight;
            swarm_enemies[s].vy += sep.second * sep_weight + ali.second * ali_weight + 
                                  coh.second * coh_weight + bounds.second * 2.0f + 
                                  seek_player.second * seek_weight;
            
            // Add gentle leftward drift for side-scrolling effect
            swarm_enemies[s].vx -= 0.3f;
            
            // Limit velocity
            float speed = sqrt(swarm_enemies[s].vx * swarm_enemies[s].vx + 
                              swarm_enemies[s].vy * swarm_enemies[s].vy);
            if (speed > swarm_enemies[s].max_speed) {
                swarm_enemies[s].vx = (swarm_enemies[s].vx / speed) * swarm_enemies[s].max_speed;
                swarm_enemies[s].vy = (swarm_enemies[s].vy / speed) * swarm_enemies[s].max_speed;
            }
            
            // Update position with slower movement for better visibility
            swarm_enemies[s].x += swarm_enemies[s].vx * dt * 15.0f;
            swarm_enemies[s].y += swarm_enemies[s].vy * dt * 15.0f;
            
            // Aggressive swarm shooting behavior
            if (swarm_enemies[s].type == 2 && swarm_enemies[s].ai_timer > 1200) {
                float dist_to_player = sqrt((swarm_enemies[s].x - player.x) * (swarm_enemies[s].x - player.x) + 
                                          (swarm_enemies[s].y - player.y) * (swarm_enemies[s].y - player.y));
                if (dist_to_player < 12 && rand() % 8 == 0) {
                    float dx = player.x - swarm_enemies[s].x;
                    float dy = player.y - swarm_enemies[s].y;
                    float len = sqrt(dx * dx + dy * dy);
                    if (len > 0) {
                        fireEnemyBullet(swarm_enemies[s].x, swarm_enemies[s].y, 
                                      (dx / len) * -2.5f, (dy / len) * -2.5f);
                    }
                    swarm_enemies[s].ai_timer = 0;
                }
            }
            
            // Remove enemies that go off screen
            if (swarm_enemies[s].x < -8) {
                swarm_enemies[s].active = false;
            }
        }
    }
    
    void updateParticles(float dt) {
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (particles[p].active) {
                particles[p].x += particles[p].vx * dt * 10.0f;
                particles[p].y += particles[p].vy * dt * 10.0f;
                particles[p].life -= dt;
                
                // Gravity for explosion particles
                if (particles[p].type == 0) {
                    particles[p].vy += dt * 5.0f; // gravity
                }
                
                if (particles[p].life <= 0) {
                    particles[p].active = false;
                }
            }
        }
    }
    
    void updatePowerUps(float dt) {
        for (int p = 0; p < MAX_POWERUPS; p++) {
            if (powerups[p].active) {
                powerups[p].x -= dt * 15.0f; // Scroll with terrain
                powerups[p].anim_phase += dt * 5.0f;
                
                if (powerups[p].x < -2) {
                    powerups[p].active = false;
                }
                
                // Check collision with player
                if (abs(powerups[p].x - player.x) < 2 && abs(powerups[p].y - player.y) < 2) {
                    switch (powerups[p].type) {
                        case 0: // Weapon upgrade
                            player.weapon_type = (player.weapon_type + 1) % 4;
                            break;
                        case 1: // Health
                            player.health = std::min(100, player.health + 25);
                            break;
                        case 2: // Speed boost (temporary)
                            // Could implement speed boost here
                            break;
                    }
                    powerups[p].active = false;
                    
                    // Create pickup effect
                    createExplosion(powerups[p].x, powerups[p].y, 5);
                }
            }
        }
    }
    
    void checkCollisions() {
        // Player bullets vs enemies
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) continue;
            
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;
                
                if (abs(bullets[b].x - enemies[e].x) < 2 && abs(bullets[b].y - enemies[e].y) < 2) {
                    // Hit!
                    bullets[b].active = false;
                    enemies[e].health--;
                    
                    createExplosion(enemies[e].x, enemies[e].y, 3);
                    
                    if (enemies[e].health <= 0) {
                        createExplosion(enemies[e].x, enemies[e].y, 8);
                        score += (enemies[e].type + 1) * 10;
                        
                        // Chance to drop power-up
                        if (rand() % 10 == 0) {
                            spawnPowerUp(enemies[e].x, enemies[e].y);
                        }
                        
                        enemies[e].active = false;
                    }
                }
            }
        }
        
        // Player bullets vs swarm enemies
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (!bullets[b].active) continue;
            
            for (int s = 0; s < MAX_SWARM_ENEMIES; s++) {
                if (!swarm_enemies[s].active) continue;
                
                if (abs(bullets[b].x - swarm_enemies[s].x) < 1.5f && abs(bullets[b].y - swarm_enemies[s].y) < 1.5f) {
                    // Hit!
                    bullets[b].active = false;
                    swarm_enemies[s].health--;
                    
                    createExplosion(swarm_enemies[s].x, swarm_enemies[s].y, 2);
                    
                    if (swarm_enemies[s].health <= 0) {
                        createExplosion(swarm_enemies[s].x, swarm_enemies[s].y, 4);
                        score += (swarm_enemies[s].type + 1) * 5; // Lower score than regular enemies
                        
                        // Small chance to drop power-up
                        if (rand() % 15 == 0) {
                            spawnPowerUp(swarm_enemies[s].x, swarm_enemies[s].y);
                        }
                        
                        swarm_enemies[s].active = false;
                    }
                }
            }
        }
        
        // Enemy bullets vs player
        if (player.alive && game_time > player.invulnerable_until) {
            for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
                if (!enemy_bullets[b].active) continue;
                
                if (abs(enemy_bullets[b].x - player.x) < 2 && abs(enemy_bullets[b].y - player.y) < 2) {
                    enemy_bullets[b].active = false;
                    player.health -= 10;
                    player.invulnerable_until = game_time + 500; // 500ms invulnerability
                    
                    createExplosion(player.x, player.y, 5);
                    
                    if (player.health <= 0) {
                        player.alive = false;
                        game_over = true;
                        game_over_time = 0; // Reset timer for new game over
                        createExplosion(player.x, player.y, 15);
                    }
                }
            }
        }
        
        // Enemies vs player
        if (player.alive && game_time > player.invulnerable_until) {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;
                
                if (abs(enemies[e].x - player.x) < 2 && abs(enemies[e].y - player.y) < 2) {
                    player.health -= 20;
                    player.invulnerable_until = game_time + 1000; // 1s invulnerability
                    
                    createExplosion(player.x, player.y, 8);
                    createExplosion(enemies[e].x, enemies[e].y, 5);
                    enemies[e].active = false;
                    
                    if (player.health <= 0) {
                        player.alive = false;
                        game_over = true;
                        game_over_time = 0; // Reset timer for new game over
                        createExplosion(player.x, player.y, 15);
                    }
                }
            }
        }
        
        // Swarm enemies vs player
        if (player.alive && game_time > player.invulnerable_until) {
            for (int s = 0; s < MAX_SWARM_ENEMIES; s++) {
                if (!swarm_enemies[s].active) continue;
                
                if (abs(swarm_enemies[s].x - player.x) < 1.8f && abs(swarm_enemies[s].y - player.y) < 1.8f) {
                    player.health -= 5; // Less damage than regular enemies
                    player.invulnerable_until = game_time + 300; // Shorter invulnerability
                    
                    createExplosion(player.x, player.y, 4);
                    createExplosion(swarm_enemies[s].x, swarm_enemies[s].y, 3);
                    swarm_enemies[s].active = false;
                    
                    if (player.health <= 0) {
                        player.alive = false;
                        game_over = true;
                        game_over_time = 0;
                        createExplosion(player.x, player.y, 15);
                    }
                }
            }
        }
    }
    
    void drawPlayer() {
        if (!player.alive) return;
        
        // Screen shake effect
        int shake_x = 0, shake_y = 0;
        if (screen_shake > 0) {
            shake_x = (rand() % (int)(screen_shake * 2)) - (int)screen_shake;
            shake_y = (rand() % (int)(screen_shake * 2)) - (int)screen_shake;
            screen_shake *= 0.9f;
        }
        
        int px = (int)player.x + shake_x;
        int py = (int)player.y + shake_y;
        
        // Player ship - right-facing triangle
        bool invulnerable = game_time < player.invulnerable_until;
        if (!invulnerable || (game_time / 100) % 2) {
            // Triangle shape: tip at (px+1, py), base vertical line at px
            
            // Triangle tip (nose of ship)
            gfx->set_pen(200, 240, 255);
            gfx->pixel(Point(px + 1, py));
            
            // Triangle body (base)
            gfx->set_pen(150, 200, 255);
            gfx->pixel(Point(px, py));        // Center
            gfx->pixel(Point(px, py - 1));    // Top
            gfx->pixel(Point(px, py + 1));    // Bottom
            
            // Weapon indicator - change the tip color
            switch (player.weapon_type) {
                case 1: // Triple shot - green tint
                    gfx->set_pen(150, 255, 150);
                    gfx->pixel(Point(px + 1, py));
                    break;
                case 2: // Missiles - red tint  
                    gfx->set_pen(255, 150, 150);
                    gfx->pixel(Point(px + 1, py));
                    break;
                case 3: // Quad-directional - purple/magenta tint
                    gfx->set_pen(255, 150, 255);
                    gfx->pixel(Point(px + 1, py));
                    // Add cross pattern to indicate 4-way firing
                    gfx->set_pen(200, 100, 200);
                    if (px - 1 >= 0) gfx->pixel(Point(px - 1, py));
                    if (py - 1 >= 0) gfx->pixel(Point(px, py - 1));
                    if (py + 1 < DISPLAY_HEIGHT) gfx->pixel(Point(px, py + 1));
                    break;
                default: // Single shot - bright tip
                    gfx->set_pen(255, 255, 200);
                    gfx->pixel(Point(px + 1, py));
                    break;
            }
        }
    }
    
    void drawLine(float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t b) {
        // Bresenham line algorithm for smooth bullet trails
        int ix1 = (int)x1, iy1 = (int)y1, ix2 = (int)x2, iy2 = (int)y2;
        
        int dx = abs(ix2 - ix1);
        int dy = abs(iy2 - iy1);
        int sx = (ix1 < ix2) ? 1 : -1;
        int sy = (iy1 < iy2) ? 1 : -1;
        int err = dx - dy;
        
        gfx->set_pen(r, g, b);
        
        while (true) {
            if (ix1 >= 0 && ix1 < DISPLAY_WIDTH && iy1 >= 0 && iy1 < DISPLAY_HEIGHT) {
                gfx->pixel(Point(ix1, iy1));
            }
            
            if (ix1 == ix2 && iy1 == iy2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                ix1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                iy1 += sy;
            }
        }
    }
    
    void drawBullets() {
        // Player bullets with enhanced trails
        for (int b = 0; b < MAX_BULLETS; b++) {
            if (bullets[b].active) {
                // Get bullet colors
                uint8_t main_r = 150, main_g = 255, main_b = 255; // Brighter default cyan
                uint8_t trail_r = 100, trail_g = 230, trail_b = 255; // Brighter cyan trail
                
                switch (bullets[b].type) {
                    case 0: // Normal bullets - bright cyan
                        main_r = 150; main_g = 255; main_b = 255;
                        trail_r = 100; trail_g = 230; trail_b = 255;
                        break;
                    case 1: // Triple shots - bright green
                        main_r = 150; main_g = 255; main_b = 150;
                        trail_r = 100; trail_g = 230; trail_b = 100;
                        break;
                    case 2: // Missiles - bright orange/red
                        main_r = 255; main_g = 180; main_b = 80;
                        trail_r = 230; trail_g = 130; trail_b = 50;
                        break;
                    case 3: // Quad-directional - bright magenta/purple
                        main_r = 255; main_g = 150; main_b = 255;
                        trail_r = 230; trail_g = 100; trail_b = 230;
                        break;
                }
                
                // Draw trail segments connecting each point
                for (int t = 1; t < bullets[b].trail_length; t++) {
                    float intensity = (float)(bullets[b].trail_length - t) / bullets[b].trail_length;
                    intensity *= 0.8f; // Make trail dimmer than main bullet
                    
                    uint8_t seg_r = (uint8_t)(trail_r * intensity);
                    uint8_t seg_g = (uint8_t)(trail_g * intensity);
                    uint8_t seg_b = (uint8_t)(trail_b * intensity);
                    
                    // Draw line segment between consecutive trail points
                    drawLine(bullets[b].trail_x[t-1], bullets[b].trail_y[t-1],
                            bullets[b].trail_x[t], bullets[b].trail_y[t],
                            seg_r, seg_g, seg_b);
                }
                
                // Draw the main bullet head (brighter)
                int bx = (int)bullets[b].x;
                int by = (int)bullets[b].y;
                if (bx >= 0 && bx < DISPLAY_WIDTH && by >= 0 && by < DISPLAY_HEIGHT) {
                    gfx->set_pen(main_r, main_g, main_b);
                    gfx->pixel(Point(bx, by));
                    
                    // Add a bright core for missiles
                    if (bullets[b].type == 2) {
                        gfx->set_pen(255, 255, 200);
                        gfx->pixel(Point(bx, by));
                    }
                }
                
                // Add extra glow effect for bullets
                if (bullets[b].type != 0) { // Triple shots, missiles, and quad shots get extra glow
                    gfx->set_pen((uint8_t)(main_r * 0.3f), (uint8_t)(main_g * 0.3f), (uint8_t)(main_b * 0.3f));
                    if (bx > 0) gfx->pixel(Point(bx-1, by));
                    if (bx < DISPLAY_WIDTH-1) gfx->pixel(Point(bx+1, by));
                    if (by > 0) gfx->pixel(Point(bx, by-1));
                    if (by < DISPLAY_HEIGHT-1) gfx->pixel(Point(bx, by+1));
                    
                    // Quad-directional bullets get extra bright glow
                    if (bullets[b].type == 3) {
                        gfx->set_pen((uint8_t)(main_r * 0.5f), (uint8_t)(main_g * 0.5f), (uint8_t)(main_b * 0.5f));
                        if (bx > 1) gfx->pixel(Point(bx-2, by));
                        if (bx < DISPLAY_WIDTH-2) gfx->pixel(Point(bx+2, by));
                        if (by > 1) gfx->pixel(Point(bx, by-2));
                        if (by < DISPLAY_HEIGHT-2) gfx->pixel(Point(bx, by+2));
                    }
                }
            }
        }
        
        // Enemy bullets with simple trail
        for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
            if (enemy_bullets[b].active) {
                int bx = (int)enemy_bullets[b].x;
                int by = (int)enemy_bullets[b].y;
                if (bx >= 0 && bx < DISPLAY_WIDTH && by >= 0 && by < DISPLAY_HEIGHT) {
                    gfx->set_pen(255, 130, 130);
                    gfx->pixel(Point(bx, by));
                    
                    // Brighter trail effect for enemy bullets
                    gfx->set_pen(200, 80, 80);
                    if (bx < DISPLAY_WIDTH-1) gfx->pixel(Point(bx+1, by));
                    if (bx < DISPLAY_WIDTH-2) {
                        gfx->set_pen(150, 60, 60);
                        gfx->pixel(Point(bx+2, by));
                    }
                }
            }
        }
    }
    
    void drawEnemies() {
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (enemies[e].active) {
                int ex = (int)enemies[e].x;
                int ey = (int)enemies[e].y;
                
                if (ex >= 0 && ex < DISPLAY_WIDTH && ey >= 0 && ey < DISPLAY_HEIGHT) {
                    switch (enemies[e].type) {
                        case 0: // Basic - bright red
                            gfx->set_pen(255, 80, 80);
                            gfx->pixel(Point(ex, ey));
                            break;
                        case 1: // Fast - bright purple
                            gfx->set_pen(255, 80, 255);
                            gfx->pixel(Point(ex, ey));
                            gfx->set_pen(180, 60, 180);
                            gfx->pixel(Point(ex - 1, ey));
                            break;
                        case 2: // Tank - bright orange
                            gfx->set_pen(255, 180, 80);
                            gfx->pixel(Point(ex, ey));
                            gfx->set_pen(230, 130, 60);
                            gfx->pixel(Point(ex - 1, ey));
                            gfx->pixel(Point(ex + 1, ey));
                            gfx->pixel(Point(ex, ey - 1));
                            gfx->pixel(Point(ex, ey + 1));
                            break;
                        case 3: // Shooter - bright yellow
                            gfx->set_pen(255, 255, 80);
                            gfx->pixel(Point(ex, ey));
                            gfx->set_pen(230, 230, 60);
                            gfx->pixel(Point(ex - 1, ey));
                            break;
                    }
                }
            }
        }
    }
    
    void drawSwarmEnemies() {
        for (int s = 0; s < MAX_SWARM_ENEMIES; s++) {
            if (!swarm_enemies[s].active) continue;
            
            int sx = (int)swarm_enemies[s].x;
            int sy = (int)swarm_enemies[s].y;
            
            if (sx >= -2 && sx < DISPLAY_WIDTH + 2 && sy >= -2 && sy < DISPLAY_HEIGHT + 2) {
                switch (swarm_enemies[s].type) {
                    case 0: // Drone swarm - small blue dots with animation
                        {
                            float pulse = sin(swarm_enemies[s].wing_phase) * 0.3f + 0.7f;
                            gfx->set_pen((uint8_t)(120 * pulse), (uint8_t)(200 * pulse), (uint8_t)(255 * pulse));
                            gfx->pixel(Point(sx, sy));
                            
                            // Add subtle wing effect
                            if (sin(swarm_enemies[s].wing_phase) > 0) {
                                gfx->set_pen((uint8_t)(60 * pulse), (uint8_t)(120 * pulse), (uint8_t)(180 * pulse));
                                if (sx - 1 >= 0) gfx->pixel(Point(sx - 1, sy));
                                if (sx + 1 < DISPLAY_WIDTH) gfx->pixel(Point(sx + 1, sy));
                            }
                        }
                        break;
                        
                    case 1: // Defensive swarm - green formation
                        {
                            float pulse = sin(swarm_enemies[s].wing_phase * 0.8f) * 0.2f + 0.8f;
                            gfx->set_pen((uint8_t)(120 * pulse), (uint8_t)(255 * pulse), (uint8_t)(120 * pulse));
                            gfx->pixel(Point(sx, sy));
                            
                            // Formation indicator - small cross pattern
                            gfx->set_pen((uint8_t)(80 * pulse), (uint8_t)(180 * pulse), (uint8_t)(80 * pulse));
                            if (sx > 0) gfx->pixel(Point(sx - 1, sy));
                            if (sx < DISPLAY_WIDTH - 1) gfx->pixel(Point(sx + 1, sy));
                            if (sy > 0) gfx->pixel(Point(sx, sy - 1));
                            if (sy < DISPLAY_HEIGHT - 1) gfx->pixel(Point(sx, sy + 1));
                        }
                        break;
                        
                    case 2: // Aggressive swarm - red with animated wings
                        {
                            float pulse = sin(swarm_enemies[s].wing_phase * 1.2f) * 0.4f + 0.6f;
                            gfx->set_pen((uint8_t)(255 * pulse), (uint8_t)(100 * pulse), (uint8_t)(100 * pulse));
                            gfx->pixel(Point(sx, sy));
                            
                            // Aggressive wing animation
                            bool wing_extended = sin(swarm_enemies[s].wing_phase) > 0;
                            if (wing_extended) {
                                gfx->set_pen((uint8_t)(200 * pulse), (uint8_t)(60 * pulse), (uint8_t)(60 * pulse));
                                if (sx - 1 >= 0 && sy - 1 >= 0) gfx->pixel(Point(sx - 1, sy - 1));
                                if (sx + 1 < DISPLAY_WIDTH && sy - 1 >= 0) gfx->pixel(Point(sx + 1, sy - 1));
                                if (sx - 1 >= 0 && sy + 1 < DISPLAY_HEIGHT) gfx->pixel(Point(sx - 1, sy + 1));
                                if (sx + 1 < DISPLAY_WIDTH && sy + 1 < DISPLAY_HEIGHT) gfx->pixel(Point(sx + 1, sy + 1));
                            } else {
                                gfx->set_pen((uint8_t)(180 * pulse), (uint8_t)(40 * pulse), (uint8_t)(40 * pulse));
                                if (sx > 0) gfx->pixel(Point(sx - 1, sy));
                                if (sx < DISPLAY_WIDTH - 1) gfx->pixel(Point(sx + 1, sy));
                            }
                        }
                        break;
                }
                
                // Add slight energy trail effect for all swarm types
                if (rand() % 6 == 0) {
                    gfx->set_pen(80, 80, 120);
                    if (sx + 1 < DISPLAY_WIDTH) gfx->pixel(Point(sx + 1, sy));
                }
            }
        }
    }
    
    void drawParticles() {
        for (int p = 0; p < MAX_PARTICLES; p++) {
            if (particles[p].active) {
                int px = (int)particles[p].x;
                int py = (int)particles[p].y;
                
                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    float life_ratio = particles[p].life / particles[p].max_life;
                    uint8_t r = (uint8_t)(particles[p].r * life_ratio);
                    uint8_t g = (uint8_t)(particles[p].g * life_ratio);
                    uint8_t b = (uint8_t)(particles[p].b * life_ratio);
                    
                    gfx->set_pen(r, g, b);
                    gfx->pixel(Point(px, py));
                }
            }
        }
    }
    
    void drawPowerUps() {
        for (int p = 0; p < MAX_POWERUPS; p++) {
            if (powerups[p].active) {
                int px = (int)powerups[p].x;
                int py = (int)powerups[p].y;
                
                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    // Pulsing animation
                    float pulse = sin(powerups[p].anim_phase) * 0.5f + 0.5f;
                    
                    switch (powerups[p].type) {
                        case 0: // Weapon - cycles colors
                            {
                                uint8_t r, g, b;
                                hsv_to_rgb(powerups[p].anim_phase * 60, 1.0f, pulse, r, g, b);
                                gfx->set_pen(r, g, b);
                            }
                            break;
                        case 1: // Health - bright red pulse
                            gfx->set_pen((uint8_t)(255 * pulse), (uint8_t)(80 * pulse), (uint8_t)(80 * pulse));
                            break;
                        case 2: // Speed - bright blue pulse
                            gfx->set_pen((uint8_t)(80 * pulse), (uint8_t)(80 * pulse), (uint8_t)(255 * pulse));
                            break;
                    }
                    gfx->pixel(Point(px, py));
                    
                    // Add sparkle effect
                    if ((int)(powerups[p].anim_phase * 10) % 3 == 0) {
                        gfx->set_pen(255, 255, 255);
                        gfx->pixel(Point(px + 1, py));
                        gfx->pixel(Point(px - 1, py));
                        gfx->pixel(Point(px, py + 1));
                        gfx->pixel(Point(px, py - 1));
                    }
                }
            }
        }
    }
    
    void drawHUD() {
        // Health bar at top
        int health_width = (player.health * 20) / 100;
        for (int i = 0; i < health_width; i++) {
            uint8_t green = (uint8_t)(255 * player.health / 100);
            uint8_t red = 255 - green;
            gfx->set_pen(red, green, 0);
            gfx->pixel(Point(i + 6, 1));
        }
        
        // Score display (simplified - just a few pixels representing score level)
        int score_level = score / 100;
        for (int i = 0; i < std::min(score_level, 8); i++) {
            gfx->set_pen(255, 255, 0);
            gfx->pixel(Point(28 + (i % 4), 1 + (i / 4)));
        }
        
        // Theme indicator - small dots at bottom showing current theme
        const ThemeColors& theme = themes[current_theme];
        for (int i = 0; i < (int)current_theme + 1; i++) {
            gfx->set_pen(theme.highlight_r / 2, theme.highlight_g / 2, theme.highlight_b / 2);
            gfx->pixel(Point(2 + i, 30));
        }
        
        // Distance progress indicator - shows progress to next theme
        if (current_theme < THEME_COUNT - 1) {
            float next_distance = themes[current_theme + 1].distance_threshold;
            float current_distance_in_theme = total_distance - themes[current_theme].distance_threshold;
            float theme_distance_span = next_distance - themes[current_theme].distance_threshold;
            
            if (theme_distance_span > 0) {
                int progress_pixels = (int)((current_distance_in_theme / theme_distance_span) * 8);
                progress_pixels = std::min(progress_pixels, 8);
                
                for (int i = 0; i < progress_pixels; i++) {
                    gfx->set_pen(100, 100, 100);
                    gfx->pixel(Point(12 + i, 30));
                }
            }
        }
    }
    
    void drawGameOverText() {
        // Draw "GAME OVER" in a simple pixel font
        // Flashing effect
        bool flash = (game_time / 300) % 2;
        uint8_t brightness = flash ? 255 : 150;
        gfx->set_pen(brightness, brightness/2, brightness/2);
        
        // G
        gfx->pixel(Point(4, 12));
        gfx->pixel(Point(4, 13));
        gfx->pixel(Point(4, 14));
        gfx->pixel(Point(4, 15));
        gfx->pixel(Point(5, 12));
        gfx->pixel(Point(5, 15));
        gfx->pixel(Point(6, 14));
        gfx->pixel(Point(6, 15));
        
        // A  
        gfx->pixel(Point(8, 12));
        gfx->pixel(Point(8, 13));
        gfx->pixel(Point(8, 14));
        gfx->pixel(Point(8, 15));
        gfx->pixel(Point(9, 12));
        gfx->pixel(Point(9, 14));
        gfx->pixel(Point(10, 12));
        gfx->pixel(Point(10, 13));
        gfx->pixel(Point(10, 14));
        gfx->pixel(Point(10, 15));
        
        // M
        gfx->pixel(Point(12, 12));
        gfx->pixel(Point(12, 13));
        gfx->pixel(Point(12, 14));
        gfx->pixel(Point(12, 15));
        gfx->pixel(Point(13, 12));
        gfx->pixel(Point(14, 13));
        gfx->pixel(Point(15, 12));
        gfx->pixel(Point(15, 13));
        gfx->pixel(Point(15, 14));
        gfx->pixel(Point(15, 15));
        
        // E
        gfx->pixel(Point(17, 12));
        gfx->pixel(Point(17, 13));
        gfx->pixel(Point(17, 14));
        gfx->pixel(Point(17, 15));
        gfx->pixel(Point(18, 12));
        gfx->pixel(Point(18, 14));
        gfx->pixel(Point(18, 15));
        gfx->pixel(Point(19, 12));
        gfx->pixel(Point(19, 14));
        gfx->pixel(Point(19, 15));
        
        // O (second row)
        gfx->pixel(Point(6, 18));
        gfx->pixel(Point(6, 19));
        gfx->pixel(Point(6, 20));
        gfx->pixel(Point(6, 21));
        gfx->pixel(Point(7, 18));
        gfx->pixel(Point(7, 21));
        gfx->pixel(Point(8, 18));
        gfx->pixel(Point(8, 19));
        gfx->pixel(Point(8, 20));
        gfx->pixel(Point(8, 21));
        
        // V
        gfx->pixel(Point(10, 18));
        gfx->pixel(Point(10, 19));
        gfx->pixel(Point(11, 20));
        gfx->pixel(Point(12, 21));
        gfx->pixel(Point(13, 20));
        gfx->pixel(Point(14, 18));
        gfx->pixel(Point(14, 19));
        
        // E
        gfx->pixel(Point(16, 18));
        gfx->pixel(Point(16, 19));
        gfx->pixel(Point(16, 20));
        gfx->pixel(Point(16, 21));
        gfx->pixel(Point(17, 18));
        gfx->pixel(Point(17, 20));
        gfx->pixel(Point(17, 21));
        gfx->pixel(Point(18, 18));
        gfx->pixel(Point(18, 20));
        gfx->pixel(Point(18, 21));
        
        // R
        gfx->pixel(Point(20, 18));
        gfx->pixel(Point(20, 19));
        gfx->pixel(Point(20, 20));
        gfx->pixel(Point(20, 21));
        gfx->pixel(Point(21, 18));
        gfx->pixel(Point(21, 20));
        gfx->pixel(Point(22, 18));
        gfx->pixel(Point(22, 19));
        gfx->pixel(Point(22, 21));
    }
    
public:
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        // Initialize game state
        player = Player();
        game_time = 0;
        last_enemy_spawn = 0;
        score = 0;
        game_over = false;
        game_over_time = 0;
        demo_mode = true;
        demo_target_y = 16.0f;
        demo_weapon_change_time = 0;
        demo_last_dodge = 0;
        mode_switch_time = 0;
        scroll_x = 0;
        total_distance = 0;
        current_theme = (Theme)(rand() % THEME_COUNT);  // Start with random theme
        terrain_offset = 0;
        screen_shake = 0;
        last_update_time = to_ms_since_boot(get_absolute_time());
        
        // Clear arrays
        for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemy_bullets[i].active = false;
        for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
        for (int i = 0; i < MAX_SWARM_ENEMIES; i++) swarm_enemies[i].active = false;
        for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;
        for (int i = 0; i < MAX_POWERUPS; i++) powerups[i].active = false;
    }
    
    bool update() override {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        float dt = (current_time - last_update_time) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f; // Cap delta time
        last_update_time = current_time;
        
        game_time = current_time;
        
        if (game_over) {
            // Game over state with 5-second display then auto-restart
            if (game_over_time == 0) {
                game_over_time = current_time; // Record when game over started
            }
            
            if (current_time - game_over_time > 5000) { // 5 seconds
                // Auto-restart in demo mode
                init(*gfx, *cosmic);
                demo_mode = true;
                return true;
            }
            
            return !checkExitCondition(button_d_pressed); // Check for exit during game over
        }
        
        updateTerrain();
        
        // Update demo AI or regular player
        if (demo_mode) {
            updateDemoAI(dt);
        }
        updatePlayer(dt);
        
        updateBullets(dt);
        updateEnemies(dt);
        updateSwarmEnemies(dt);
        updateParticles(dt);
        updatePowerUps(dt);
        checkCollisions();
        
        // Spawn enemies
        if (current_time - last_enemy_spawn > 1500) {
            spawnEnemy();
            last_enemy_spawn = current_time;
        }
        
        // Occasionally spawn tougher enemies
        if (current_time - last_enemy_spawn > 800 && rand() % 100 < 5) {
            spawnEnemy(2); // Tank
            last_enemy_spawn = current_time;
        }
        
        // Spawn swarm enemies periodically
        if (current_time - last_swarm_spawn > 4000) {
            int swarm_type = rand() % 3;
            int swarm_size = 3 + rand() % 4; // 3-6 enemies per swarm
            float spawn_y = 5 + rand() % (DISPLAY_HEIGHT - 10);
            
            spawnSwarm(swarm_size, swarm_type, next_swarm_id++, DISPLAY_WIDTH + 5, spawn_y);
            last_swarm_spawn = current_time;
        }
        
        // Occasionally spawn smaller aggressive swarms
        if (current_time - last_swarm_spawn > 2000 && rand() % 100 < 4) {
            int small_swarm = 2 + rand() % 3; // 2-4 enemies
            float spawn_y = 8 + rand() % (DISPLAY_HEIGHT - 16);
            
            spawnSwarm(small_swarm, 2, next_swarm_id++, DISPLAY_WIDTH + 3, spawn_y); // Always aggressive type
            last_swarm_spawn = current_time - 1500; // Reduce cooldown
        }
        
        return !checkExitCondition(button_d_pressed); // Check exit condition
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        gfx->set_pen(0, 0, 0);
        gfx->clear();
        
        drawNebulaBackground();
        drawTerrain();
        drawParticles();
        drawEnemies();
        drawSwarmEnemies();
        drawBullets();
        drawPowerUps();
        drawPlayer();
        drawHUD();
        
        if (game_over) {
            // Display "GAME OVER" text
            drawGameOverText();
        }
        
        // Display demo mode indicator
        if (demo_mode && !game_over) {
            // Small indicator that we're in demo mode
            gfx->set_pen(100, 100, 255);
            gfx->pixel(Point(0, 0));
            gfx->pixel(Point(1, 0));
            gfx->pixel(Point(0, 1));
        }
    }
    
    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down, 
                    bool button_bright_up, bool button_bright_down) override {
        
        // Store button d state for exit checking in update
        button_d_pressed = button_d;
        
        if (checkExitCondition(button_d)) {
            // Exit handled by base class
            return;
        }
        
        if (game_over) {
            // During game over, A button manually restarts
            if (button_a) {
                init(*gfx, *cosmic);
                demo_mode = true; // Restart in demo mode
            }
            return;
        }
        
        // A button switches from demo mode to player mode  
        if (demo_mode && button_a) {
            demo_mode = false;
            mode_switch_time = game_time;
            return; // Don't process other inputs this frame
        }
        
        // Only allow manual controls when not in demo mode and after brief cooldown
        if (!demo_mode && (game_time - mode_switch_time) > 100) {
            // Direct movement controls - simple and smooth for 32x32 LED matrix
            const float move_speed = 1.00f;
            if (button_vol_up) { // Up
                player.y -= move_speed;
            }
            if (button_vol_down) { // Down  
                player.y += move_speed;
            }
            if (button_bright_down) { // Left
                player.x -= move_speed;
            }
            if (button_bright_up) { // Right
                player.x += move_speed;
            }
            
            // Note: Primary movement is Vol Up/Down for vertical, Brightness Up/Down for horizontal
            // This provides full 4-direction control
            
            // Shooting (now works in player mode)
            if (button_a) {
                uint32_t shot_delay;
                switch (player.weapon_type) {
                    case 1: shot_delay = player.triple_shot_delay; break;
                    case 2: shot_delay = player.missile_shot_delay; break;
                    case 3: shot_delay = player.quad_shot_delay; break;
                    default: shot_delay = player.single_shot_delay; break;
                }
                
                if (game_time - player.last_shot > shot_delay) {
                    switch (player.weapon_type) {
                        case 0: // Single shot
                            fireBullet(player.x + 2, player.y, 4.0f, 0, 0);
                            break;
                        case 1: // Triple shot
                            fireBullet(player.x + 2, player.y, 4.0f, 0, 1);
                            fireBullet(player.x + 2, player.y - 1, 4.0f, -0.5f, 1);
                            fireBullet(player.x + 2, player.y + 1, 4.0f, 0.5f, 1);
                            break;
                        case 2: // Missiles
                            fireBullet(player.x + 2, player.y, 3.0f, 0, 2);
                            break;
                        case 3: // Quad-directional
                            fireBullet(player.x + 2, player.y, 3.5f, 0, 3);       // East (right)
                            fireBullet(player.x, player.y, -2.5f, 0, 3);          // West (left)  
                            fireBullet(player.x, player.y - 1, 0, -3.0f, 3);      // North (up)
                            fireBullet(player.x, player.y + 1, 0, 3.0f, 3);       // South (down)
                            break;
                    }
                    player.last_shot = game_time;
                }
            }
            
            // Weapon cycling
            if (button_b && game_time - player.last_shot > 300) {
                player.weapon_type = (player.weapon_type + 1) % 4;
                player.last_shot = game_time; // Prevent rapid cycling
            }
        }
    }
    
    const char* getName() const override {
        return "Space Fighter";
    }
    
    const char* getDescription() const override {
        return "R-Type style shooter with demo mode. Press A to play, auto-restarts after game over";
    }
};
