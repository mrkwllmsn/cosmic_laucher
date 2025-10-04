#pragma once

#include "../game_base.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace pimoroni;

class AfterburnerGame : public GameBase {
private:
    static constexpr int DISPLAY_WIDTH = 32;
    static constexpr int DISPLAY_HEIGHT = 32;
    static constexpr int MAX_BULLETS = 15;
    static constexpr int MAX_ENEMIES = 8;
    static constexpr int MAX_PARTICLES = 30;

    // Simple Perlin noise implementation
    class SimplePerlin {
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
        SimplePerlin() {
            for (int i = 0; i < PERM_SIZE; i++) {
                perm[i] = i;
            }
            // Simple shuffle
            for (int i = 0; i < PERM_SIZE; i++) {
                int j = (i * 17 + 19) % PERM_SIZE;
                int temp = perm[i];
                perm[i] = perm[j];
                perm[j] = temp;
            }
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
        float x = 16.0f, y = 24.0f;  // Screen position
        float target_x = 16.0f;     // Target position for smooth movement
        float target_y = 24.0f;     // Target position for smooth movement
        int health = 100;
        uint32_t last_shot = 0;
        uint32_t invulnerable_until = 0;
        bool alive = true;
        float bank_angle = 0.0f;    // Banking left/right for visual effect
        float pitch_angle = 0.0f;   // Pitching up/down for visual effect
    } player;

    struct Bullet {
        float x, y, z;      // 3D position
        float vx, vy, vz;   // 3D velocity
        bool active = false;
        uint32_t created_time;
    };

    struct Enemy {
        float x, y, z;      // 3D position (z is distance from player)
        float vx, vy, vz;   // 3D velocity
        int health;
        int type;           // 0=fighter, 1=bomber
        bool active = false;
        uint32_t last_shot = 0;
        float bank_angle = 0.0f;
    };

    struct Particle {
        float x, y, z;
        float vx, vy, vz;
        uint8_t r, g, b;
        uint32_t life_time;
        uint32_t max_life;
        bool active = false;
    };

    // Ocean wave system
    struct OceanWave {
        float amplitude;
        float frequency;
        float phase;
        float speed;
    };

    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;
    std::vector<OceanWave> ocean_waves;
    SimplePerlin perlin;

    // Game state
    uint32_t game_time = 0;
    uint32_t last_enemy_spawn = 0;
    int score = 0;
    float speed = -2.0f;
    float camera_tilt = 15.0f;   // For horizon effects
    uint32_t last_theme_change = 0;
    int current_theme = 0;      // 0 = ocean, 1 = terrain, 2 = flying_terrain

    // Rendering helpers
    float horizon_y = 50.0f;    // Horizon line position

    // Performance optimization - pre-computed terrain cache
    static constexpr int TERRAIN_CACHE_WIDTH = 64;
    static constexpr int TERRAIN_CACHE_HEIGHT = 64;
    float terrain_cache[TERRAIN_CACHE_WIDTH][TERRAIN_CACHE_HEIGHT];
    uint32_t terrain_cache_time = 0;
    bool terrain_cache_valid = false;

    void hsv_to_rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
        int i = int(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch (i % 6) {
            case 0: r = v * 255; g = t * 255; b = p * 255; break;
            case 1: r = q * 255; g = v * 255; b = p * 255; break;
            case 2: r = p * 255; g = v * 255; b = t * 255; break;
            case 3: r = p * 255; g = q * 255; b = v * 255; break;
            case 4: r = t * 255; g = p * 255; b = v * 255; break;
            case 5: r = v * 255; g = p * 255; b = q * 255; break;
        }
    }

    // Convert 3D world position to 2D screen position
    bool worldToScreen(float world_x, float world_y, float world_z, int &screen_x, int &screen_y) {
        if (world_z <= 0.1f) return false;  // Behind camera

        float perspective = 50.0f / world_z;  // Simple perspective projection
        screen_x = DISPLAY_WIDTH/2 + (int)(world_x * perspective);
        screen_y = horizon_y + (int)(world_y * perspective);

        return (screen_x >= 0 && screen_x < DISPLAY_WIDTH &&
                screen_y >= 0 && screen_y < DISPLAY_HEIGHT);
    }

    void renderSky() {
        // Simplified sky with basic gradients - much faster
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            float sky_factor = (float)y / DISPLAY_HEIGHT;
            sky_factor = sky_factor * sky_factor;

            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                // Remove expensive per-pixel noise - use simple gradients
                uint8_t r, g, b;
                if (current_theme == 0) {
                    // Ocean theme - simple blue gradient
                    r = (uint8_t)(50 + sky_factor * 80);
                    g = (uint8_t)(80 + sky_factor * 140);
                    b = (uint8_t)(150 + sky_factor * 105);
                } else {
                    // Terrain theme - simple warm gradient
                    r = (uint8_t)(120 + sky_factor * 100);
                    g = (uint8_t)(100 + sky_factor * 120);
                    b = (uint8_t)(120 + sky_factor * 80);
                }

                Pen sky_pen = gfx->create_pen(r, g, b);
                gfx->set_pen(sky_pen);
                gfx->pixel(Point(x, y));
            }
        }

        // Simplified clouds - fewer and simpler shapes
        uint32_t cloud_time = game_time / 80;
        for (int i = 0; i < 3; i++) {  // Reduced from 5 to 3 clouds
            float cloud_x = ((cloud_time + i * 150) % 250) - 60;
            float cloud_y = 2 + i * 3.0f;

            if (cloud_x >= -10 && cloud_x <= DISPLAY_WIDTH + 10 && cloud_y < DISPLAY_HEIGHT * 0.3f) {
                uint8_t cloud_r = 200;
                uint8_t cloud_g = 220;
                uint8_t cloud_b = 240;

                Pen cloud_pen = gfx->create_pen(cloud_r, cloud_g, cloud_b);
                gfx->set_pen(cloud_pen);

                // Simple circular clouds without noise
                for (int cx = -2; cx <= 2; cx++) {
                    for (int cy = -1; cy <= 1; cy++) {
                        int px = (int)cloud_x + cx;
                        int py = (int)cloud_y + cy;
                        if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                            float distance = sqrt(cx*cx + cy*cy);
                            if (distance <= 2.0f) {
                                gfx->pixel(Point(px, py));
                            }
                        }
                    }
                }
            }
        }
    }

    void updateTerrainCache() {
        if (terrain_cache_valid && (game_time - terrain_cache_time) < 100) {
            return; // Cache is still valid
        }

        float time_factor = game_time * 0.005f;

        // Pre-compute terrain heights for the cache
        for (int cx = 0; cx < TERRAIN_CACHE_WIDTH; cx++) {
            for (int cz = 0; cz < TERRAIN_CACHE_HEIGHT; cz++) {
                float world_x = (cx - TERRAIN_CACHE_WIDTH/2) * 0.5f;
                float world_z = cz * 0.5f - time_factor * 3.0f;

                // Simplified terrain generation - only 1 octave for speed
                float terrain_height = perlin.noise(world_x * 0.1f, world_z * 0.1f) * 12.0f;
                terrain_height += perlin.noise(world_x * 0.03f, world_z * 0.03f) * 13.0f;
                terrain_height += sin(world_x * 0.04f) * cos(world_z * 0.03f) * 11.0f;

                terrain_cache[cx][cz] = std::max(-8.0f, std::min(8.0f, terrain_height));
            }
        }

        terrain_cache_valid = true;
        terrain_cache_time = game_time;
    }

    void renderTerrain() {
        float time_factor = game_time * 0.005f;

        // First render the sky above terrain
        renderSky();

        // Update terrain cache
        updateTerrainCache();

        // Enhanced terrain flyover with cached terrain heights
        for (int y = horizon_y; y < DISPLAY_HEIGHT; y++) {
            float depth_factor = (float)(y - horizon_y) / (DISPLAY_HEIGHT - horizon_y);
            depth_factor = depth_factor * depth_factor;

            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                // Convert screen coordinates to cache coordinates
                float world_x = (x - DISPLAY_WIDTH/2) * (0.08f + depth_factor * 0.8f);
                float world_z = depth_factor * 20.0f - time_factor * 3.0f;

                // Map to cache coordinates
                int cache_x = (int)((world_x / 0.5f) + TERRAIN_CACHE_WIDTH/2);
                int cache_z = (int)(world_z / 0.5f);

                // Bounds check and get cached height
                float terrain_height = 0;
                if (cache_x >= 0 && cache_x < TERRAIN_CACHE_WIDTH && cache_z >= 0 && cache_z < TERRAIN_CACHE_HEIGHT) {
                    terrain_height = terrain_cache[cache_x][cache_z];
                } else {
                    // Fallback for out-of-bounds (simplified calculation)
                    terrain_height = perlin.noise(world_x * 0.1f, world_z * 0.1f) * 2.0f;
                }

                // Convert terrain height to screen offset
                float height_scale = 0.5f - depth_factor * 0.2f;
                int height_offset = (int)(terrain_height * height_scale);
                int terrain_y = y + height_offset;

                // Check if we should draw terrain at this pixel
                bool is_terrain = terrain_y >= y;

                if (is_terrain) {
                    // Simplified terrain coloring - much faster
                    float height_factor = (terrain_height + 8.0f) / 16.0f;
                    height_factor = std::max(0.0f, std::min(1.0f, height_factor));

                    // Distance fade
                    float distance_fade = 0.7f + (1.0f - depth_factor) * 0.3f;

                    // Simplified terrain type (using height factor only)
                    uint8_t r, g, b;

                    if (height_factor < 0.2f) {
                        // Water - blues
                        r = (uint8_t)(60 + height_factor * 100);
                        g = (uint8_t)(120 + height_factor * 135);
                        b = (uint8_t)(180 + height_factor * 75);
                    } else if (height_factor < 0.6f) {
                        // Grass/vegetation - greens
                        r = (uint8_t)(40 + height_factor * 120);
                        g = (uint8_t)(100 + height_factor * 155);
                        b = (uint8_t)(30 + height_factor * 90);
                    } else if (height_factor < 0.8f) {
                        // Rock/dirt - browns
                        r = (uint8_t)(80 + height_factor * 140);
                        g = (uint8_t)(60 + height_factor * 130);
                        b = (uint8_t)(40 + height_factor * 100);
                    } else {
                        // Snow/peaks - whites
                        r = (uint8_t)(180 + height_factor * 75);
                        g = (uint8_t)(200 + height_factor * 55);
                        b = (uint8_t)(220 + height_factor * 35);
                    }

                    // Apply distance fade
                    r = (uint8_t)std::max(20, (int)(r * distance_fade));
                    g = (uint8_t)std::max(20, (int)(g * distance_fade));
                    b = (uint8_t)std::max(20, (int)(b * distance_fade));

                    Pen terrain_pen = gfx->create_pen(r, g, b);
                    gfx->set_pen(terrain_pen);
                    gfx->pixel(Point(x, terrain_y));
                }
            }
        }
    }

    void renderFlyingTerrain() {
        float time_factor = game_time * 0.005f;

        // Render sky first - simplified
        for (int y = 0; y < horizon_y; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float sky_factor = (float)y / horizon_y;
                sky_factor = sky_factor * sky_factor;

                uint8_t r = (uint8_t)(40 + sky_factor * 100);
                uint8_t g = (uint8_t)(60 + sky_factor * 150);
                uint8_t b = (uint8_t)(120 + sky_factor * 135);

                Pen sky_pen = gfx->create_pen(r, g, b);
                gfx->set_pen(sky_pen);
                gfx->pixel(Point(x, y));
            }
        }

        // Simplified flying terrain - much faster
        float flying_offset = -time_factor * 2.0f;

        // Reduced terrain calculation - only calculate what we need
        static float terrain_heights[DISPLAY_WIDTH][32];  // Reduced from 32 to 16

        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            for (int z = 0; z < 32; z++) {  // Reduced depth
                float world_x = x * 0.5f;
                float world_z = z * 0.9f + flying_offset;

                // Single noise call for speed
                float height = perlin.noise(world_x * 0.2f, world_z * 0.2f);
                height = (height + 1.0f) * 0.6f;
                terrain_heights[x][z] = height * 8.0f;  // Reduced scale
            }
        }

        // Render terrain with simplified perspective
        for (int z = 8; z < 16; z++) {  // Start further back for performance
            float depth_factor = (float)z / 16.0f;
            depth_factor = depth_factor * depth_factor;

            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float height = terrain_heights[x][z];
                float perspective_height = height * (1.0f - depth_factor * 0.6f);
                int screen_y = horizon_y + (int)(perspective_height * depth_factor * 1.5f);

                // Simplified fill
                for (int y = screen_y; y < DISPLAY_HEIGHT; y++) {
                    if (y >= horizon_y && y < DISPLAY_HEIGHT) {
                        float height_factor = height / 8.0f;
                        float distance_fade = 1.0f - depth_factor * 0.4f;

                        uint8_t r, g, b;
                        if (height_factor < 0.4f) {
                            r = (uint8_t)(40 + height_factor * 100);
                            g = (uint8_t)(80 + height_factor * 140);
                            b = (uint8_t)(20 + height_factor * 80);
                        } else {
                            r = (uint8_t)(100 + height_factor * 120);
                            g = (uint8_t)(80 + height_factor * 120);
                            b = (uint8_t)(50 + height_factor * 100);
                        }

                        r = (uint8_t)(r * distance_fade);
                        g = (uint8_t)(g * distance_fade);
                        b = (uint8_t)(b * distance_fade);

                        Pen terrain_pen = gfx->create_pen(r, g, b);
                        gfx->set_pen(terrain_pen);
                        gfx->pixel(Point(x, y));
                    }
                }
            }
        }
    }

    void renderOcean() {
        float time_factor = game_time * 0.01f;

        // Simplified ocean - much faster
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            float depth_factor = (float)y / DISPLAY_HEIGHT;
            depth_factor = depth_factor * depth_factor;

            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float world_x = (x - DISPLAY_WIDTH/2) * (0.1f + depth_factor * 0.3f);
                float world_z = depth_factor * 10.0f + time_factor;

                // Single octave wave for speed
                float wave_noise = perlin.noise(world_x, world_z);

                // Add simple directional waves using sin
                float directional_waves = sin(world_x * 3.0f + time_factor * 2.0f) * 0.3f;
                wave_noise += directional_waves;

                // Normalize and create wave intensity
                float wave_intensity = (wave_noise + 1.0f) * 0.5f;
                float distance_fade = 1.0f - depth_factor * 0.4f;

                // Simplified color calculation
                uint8_t r, g, b;
                if (wave_intensity < 0.4f) {
                    // Deep water
                    r = (uint8_t)(20 + wave_intensity * 100);
                    g = (uint8_t)(60 + wave_intensity * 140);
                    b = (uint8_t)(120 + wave_intensity * 135);
                } else {
                    // Wave crests
                    r = (uint8_t)(40 + wave_intensity * 140);
                    g = (uint8_t)(80 + wave_intensity * 160);
                    b = (uint8_t)(140 + wave_intensity * 115);
                }

                // Apply distance fade
                r = (uint8_t)(r * distance_fade);
                g = (uint8_t)(g * distance_fade);
                b = (uint8_t)(b * distance_fade);

                Pen water_pen = gfx->create_pen(r, g, b);
                gfx->set_pen(water_pen);
                gfx->pixel(Point(x, y));
            }
        }

        // Simplified reflections - fewer and simpler
        for (int i = 0; i < 4; i++) {  // Reduced from 8 to 4
            float reflect_x = DISPLAY_WIDTH * 0.3f + i * 6.0f + sin(time_factor + i) * 4.0f;
            float reflect_y = horizon_y + 2 + i * 3;

            if (reflect_x >= 0 && reflect_x < DISPLAY_WIDTH && reflect_y >= horizon_y && reflect_y < DISPLAY_HEIGHT) {
                Pen reflect_pen = gfx->create_pen(200, 230, 255);
                gfx->set_pen(reflect_pen);
                gfx->pixel(Point((int)reflect_x, (int)reflect_y));
            }
        }
    }

    void renderPlayer() {
        int px = (int)player.x;
        int py = (int)player.y;

        // Banking and pitching effects
        int wing_offset = (int)(player.bank_angle * 1.5f);
        int nose_offset = (int)(player.pitch_angle * 0.8f);

        // Main aircraft body (larger, more detailed)
        Pen body_pen = gfx->create_pen(200, 200, 200);  // Light gray fuselage
        gfx->set_pen(body_pen);

        // Fuselage center line (3 pixels long)
        gfx->pixel(Point(px, py - 1 + nose_offset));
        gfx->pixel(Point(px, py));
        gfx->pixel(Point(px, py + 1));

        // Main wings (wider span, affected by banking)
        Pen wing_pen = gfx->create_pen(180, 180, 180);  // Darker gray wings
        if(wing_offset > 2){
            wing_offset = 2;
        }
        if(wing_offset < -2){
            wing_offset = -2;
        }
        gfx->set_pen(wing_pen);
        gfx->pixel(Point(px - 2 + wing_offset, py));
        gfx->pixel(Point(px - 1 + wing_offset/2, py));
        gfx->pixel(Point(px + 1 - wing_offset/2, py));
        gfx->pixel(Point(px + 2 - wing_offset, py));

        // Wing tips (yellow highlights)
        Pen wingtip_pen = gfx->create_pen(255, 255, 100);
        gfx->set_pen(wingtip_pen);
        gfx->pixel(Point(px - 2 + wing_offset, py - 1));
        gfx->pixel(Point(px + 2 - wing_offset, py - 1));

        // Tail fins
        Pen tail_pen = gfx->create_pen(160, 160, 160);
        gfx->set_pen(tail_pen);
        gfx->pixel(Point(px - 1, py + 2));
        gfx->pixel(Point(px + 1, py + 2));

        // Engine exhausts with glow
        Pen engine_pen = gfx->create_pen(255, 100, 0);  // Orange engine glow
        gfx->set_pen(engine_pen);
        gfx->pixel(Point(px - 1, py + 1));
        gfx->pixel(Point(px + 1, py + 1));

        // Engine core (hot white)
        Pen engine_core_pen = gfx->create_pen(255, 255, 255);
        gfx->set_pen(engine_core_pen);
        gfx->pixel(Point(px, py + 1));

        // Cockpit (blue tint)
        Pen cockpit_pen = gfx->create_pen(100, 150, 255);
        gfx->set_pen(cockpit_pen);
        gfx->pixel(Point(px, py - 1 + nose_offset));
    }

    void renderEnemies() {
        for (const auto& enemy : enemies) {
            if (!enemy.active) continue;

            int screen_x, screen_y;
            if (worldToScreen(enemy.x, enemy.y, enemy.z, screen_x, screen_y)) {
                // Much brighter enemies with better visibility
                float dist_factor = std::min(1.0f, 30.0f / enemy.z);
                uint8_t brightness = (uint8_t)(200 + 55 * dist_factor);  // Much brighter base

                // Larger, more detailed enemy aircraft
                float scale = std::min(1.0f, 15.0f / enemy.z);
                int size = std::max(2, (int)(scale * 5));  // Minimum size 2, max size 5

                if (enemy.type == 0) {
                    // Fighter - bright red with white highlights
                    Pen body_pen = gfx->create_pen(brightness, 0, 0);
                    Pen highlight_pen = gfx->create_pen(255, 100, 100);

                    gfx->set_pen(body_pen);
                    // Main body
                    for (int dx = -size/2; dx <= size/2; dx++) {
                        for (int dy = -size/2; dy <= size/2; dy++) {
                            int ex = screen_x + dx;
                            int ey = screen_y + dy;
                            if (ex >= 0 && ex < DISPLAY_WIDTH && ey >= 0 && ey < DISPLAY_HEIGHT) {
                                gfx->pixel(Point(ex, ey));
                            }
                        }
                    }

                    // Bright highlights for visibility
                    gfx->set_pen(highlight_pen);
                    gfx->pixel(Point(screen_x, screen_y));
                    if (size > 2) {
                        gfx->pixel(Point(screen_x-1, screen_y));
                        gfx->pixel(Point(screen_x+1, screen_y));
                    }

                } else {
                    // Bomber - bright orange with yellow highlights
                    Pen body_pen = gfx->create_pen(brightness, brightness*0.7f, 0);
                    Pen highlight_pen = gfx->create_pen(255, 255, 100);

                    gfx->set_pen(body_pen);
                    // Larger body for bomber
                    for (int dx = -(size+1)/2; dx <= (size+1)/2; dx++) {
                        for (int dy = -(size+1)/2; dy <= (size+1)/2; dy++) {
                            int ex = screen_x + dx;
                            int ey = screen_y + dy;
                            if (ex >= 0 && ex < DISPLAY_WIDTH && ey >= 0 && ey < DISPLAY_HEIGHT) {
                                gfx->pixel(Point(ex, ey));
                            }
                        }
                    }

                    // Bright center highlight
                    gfx->set_pen(highlight_pen);
                    gfx->pixel(Point(screen_x, screen_y));
                    if (size > 2) {
                        gfx->pixel(Point(screen_x, screen_y-1));
                        gfx->pixel(Point(screen_x, screen_y+1));
                    }
                }

                // Add engine glow for extra visibility
                Pen glow_pen = gfx->create_pen(255, 255, 255);
                gfx->set_pen(glow_pen);
                if (size > 1) {
                    gfx->pixel(Point(screen_x, screen_y + size/2 + 1));
                }
            }
        }
    }

    void renderBullets() {
        for (const auto& bullet : bullets) {
            if (!bullet.active) continue;

            int screen_x, screen_y;
            if (worldToScreen(bullet.x, bullet.y, bullet.z, screen_x, screen_y)) {
                Pen bullet_pen = gfx->create_pen(255, 255, 255);
                gfx->set_pen(bullet_pen);
                gfx->pixel(Point(screen_x, screen_y));

                // Add trail effect
                Pen trail_pen = gfx->create_pen(255, 200, 100);
                gfx->set_pen(trail_pen);
                if (screen_y + 1 < DISPLAY_HEIGHT) {
                    gfx->pixel(Point(screen_x, screen_y + 1));
                }
            }
        }
    }

    void renderParticles() {
        for (const auto& particle : particles) {
            if (!particle.active) continue;

            int screen_x, screen_y;
            if (worldToScreen(particle.x, particle.y, particle.z, screen_x, screen_y)) {
                float life_factor = (float)particle.life_time / particle.max_life;
                uint8_t alpha = (uint8_t)(255 * life_factor);

                Pen particle_pen = gfx->create_pen(
                    (particle.r * alpha) / 255,
                    (particle.g * alpha) / 255,
                    (particle.b * alpha) / 255
                );
                gfx->set_pen(particle_pen);
                gfx->pixel(Point(screen_x, screen_y));
            }
        }
    }

    void spawnEnemy() {
        for (auto& enemy : enemies) {
            if (!enemy.active) {
                enemy.active = true;
                enemy.x = (rand() % 40) - 20;  // Random X position
                enemy.y = (rand() % 10) - 5;   // Random Y position
                enemy.z = 80 + (rand() % 40);  // Spawn far away
                enemy.vx = (rand() % 3) - 1;   // Random movement
                enemy.vy = 0;
                enemy.vz = -speed * (0.8f + (rand() % 40) / 100.0f);
                enemy.type = rand() % 2;
                enemy.health = (enemy.type == 0) ? 1 : 3;
                enemy.bank_angle = 0;
                break;
            }
        }
    }

    void createExplosion(float x, float y, float z) {
        for (int i = 0; i < 8; i++) {
            for (auto& particle : particles) {
                if (!particle.active) {
                    particle.active = true;
                    particle.x = x + (rand() % 4) - 2;
                    particle.y = y + (rand() % 4) - 2;
                    particle.z = z + (rand() % 4) - 2;
                    particle.vx = (rand() % 6) - 3;
                    particle.vy = (rand() % 6) - 3;
                    particle.vz = (rand() % 6) - 3;
                    particle.r = 255;
                    particle.g = 100 + (rand() % 155);
                    particle.b = 0;
                    particle.life_time = 500 + (rand() % 500);
                    particle.max_life = particle.life_time;
                    break;
                }
            }
        }
    }

public:
    AfterburnerGame() {
        bullets.resize(MAX_BULLETS);
        enemies.resize(MAX_ENEMIES);
        particles.resize(MAX_PARTICLES);

        // Initialize ocean waves
        ocean_waves = {
            {0.5f, 0.1f, 0.0f, 0.02f},
            {0.3f, 0.2f, 1.5f, 0.03f},
            {0.2f, 0.4f, 3.0f, 0.05f},
            {0.1f, 0.8f, 4.5f, 0.08f}
        };
    }

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;

        // Reset game state
        player.x = 16.0f;
        player.y = 24.0f;
        player.target_x = 16.0f;
        player.target_y = 24.0f;
        player.health = 100;
        player.alive = true;
        player.bank_angle = 0.0f;
        player.pitch_angle = 0.0f;

        score = 0;
        speed = 2.0f;
        game_time = 0;
        last_enemy_spawn = 0;
        last_theme_change = 0;
        current_theme = 0;

        // Clear all game objects
        for (auto& bullet : bullets) bullet.active = false;
        for (auto& enemy : enemies) enemy.active = false;
        for (auto& particle : particles) particle.active = false;
    }

    bool update() override {
        if (!player.alive) return false;

        game_time += 16;  // Assuming ~60fps

        // Update player movement (smooth following of target with less friction)
        float move_speed = 0.15f;  // Reduced from 0.3f for less friction
        player.x += (player.target_x - player.x) * move_speed;
        player.y += (player.target_y - player.y) * move_speed;
        player.bank_angle += (player.target_x - player.x) * 0.08f;
        player.pitch_angle += (player.target_y - player.y) * 0.08f;
        player.bank_angle *= 0.95f;  // Less decay for more momentum
        player.pitch_angle *= 0.95f; // Less decay for more momentum

        // Update bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.x += bullet.vx;
                bullet.y += bullet.vy;
                bullet.z += bullet.vz;

                // Remove bullets that are too far
                if (bullet.z > 100 || bullet.z < 0) {
                    bullet.active = false;
                }
            }
        }

        // Update enemies
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.x += enemy.vx;
                enemy.y += enemy.vy;
                enemy.z += enemy.vz;

                // Simple AI - slight movement toward player
                if (enemy.z < 50) {
                    float dx = player.x - 16.0f;  // Convert screen to world coords
                    enemy.vx += dx * 0.001f;
                    enemy.vx *= 0.98f;  // Damping

                    enemy.bank_angle = enemy.vx * 2.0f;
                }

                // Remove enemies that are too close or far
                if (enemy.z < -5 || enemy.z > 120) {
                    enemy.active = false;
                }

                // Check collision with player
                int screen_x, screen_y;
                if (worldToScreen(enemy.x, enemy.y, enemy.z, screen_x, screen_y)) {
                    float dx = screen_x - player.x;
                    float dy = screen_y - player.y;
                    if (dx*dx + dy*dy < 9 && game_time > player.invulnerable_until) {
                        player.health -= 20;
                        player.invulnerable_until = game_time + 1000;
                        createExplosion(enemy.x, enemy.y, enemy.z);
                        enemy.active = false;

                        if (player.health <= 0) {
                            player.alive = false;
                        }
                    }
                }
            }
        }

        // Check bullet-enemy collisions
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;

            for (auto& enemy : enemies) {
                if (!enemy.active) continue;

                float dx = bullet.x - enemy.x;
                float dy = bullet.y - enemy.y;
                float dz = bullet.z - enemy.z;
                float dist_sq = dx*dx + dy*dy + dz*dz;

                if (dist_sq < 9) {  // Hit!
                    enemy.health--;
                    bullet.active = false;
                    createExplosion(enemy.x, enemy.y, enemy.z);

                    if (enemy.health <= 0) {
                        enemy.active = false;
                        score += (enemy.type == 0) ? 100 : 200;
                    }
                    break;
                }
            }
        }

        // Update particles
        for (auto& particle : particles) {
            if (particle.active) {
                particle.x += particle.vx * 0.1f;
                particle.y += particle.vy * 0.1f;
                particle.z += particle.vz * 0.1f;
                particle.life_time -= 16;

                if (particle.life_time <= 0) {
                    particle.active = false;
                }
            }
        }

        // Spawn enemies
        if (game_time - last_enemy_spawn > 2000) {
            spawnEnemy();
            last_enemy_spawn = game_time;
        }

        // Increase speed over time
        speed += 0.001f;

        // Switch themes every 5 seconds for testing
        if (game_time - last_theme_change > 5000) {
            current_theme = (current_theme + 1) % 3;  // Now cycle through 3 themes
            last_theme_change = game_time;
        }

        return true;
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        gfx = &graphics;
        gfx->set_pen(gfx->create_pen(0, 0, 0));
        gfx->clear();

        // Render full-screen terrain or ocean based on current theme
        if (current_theme == 0) {
            renderSky();  // Ocean theme includes sky in background
            renderOcean();
        } else if (current_theme == 1) {
            renderTerrain();  // Terrain theme includes sky rendering internally
        } else {
            renderFlyingTerrain();  // Flying terrain like demo.p5.js
        }

        renderEnemies();
        renderBullets();
        renderParticles();
        renderPlayer();

        // UI - Score
        gfx->set_pen(gfx->create_pen(255, 255, 255));
        // Note: Score display would need a proper font system
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down,
                    bool button_bright_up, bool button_bright_down) override {

        static bool last_c = false;
        static bool last_b = false;

        // Movement controls (same as Frogger)
        // A = Up, B = Down, Vol_Down = Left, Vol_Up = Right
        if (button_a && player.target_y > 3) {
            player.target_y -= 2.5f;
        }
        if (button_b && !last_b && player.target_y < DISPLAY_HEIGHT - 3) {
            player.target_y += 2.5f;
        }

        // Manual theme switching with B button (double-tap)
        static uint32_t last_b_press = 0;
        if (button_b && !last_b) {
            if (game_time - last_b_press < 300) {  // Double-tap detection
                current_theme = (current_theme + 1) % 3;  // Now cycle through 3 themes
                last_theme_change = game_time;  // Reset auto-switch timer
            }
            last_b_press = game_time;
        }
        if (button_vol_down && player.target_x > 3) {
            player.target_x -= 2.5f;
        }
        if (button_vol_up && player.target_x < DISPLAY_WIDTH - 3) {
            player.target_x += 2.5f;
        }

        // Shooting
        if (button_c && !last_c && game_time - player.last_shot > 200) {
            // Find available bullet
            for (auto& bullet : bullets) {
                if (!bullet.active) {
                    bullet.active = true;
                    // Spawn bullet from player's current position
                    bullet.x = (player.x - 16.0f) * 0.5f;  // Convert screen to world X
                    bullet.y = (player.y - horizon_y) * -0.3f;  // Convert screen Y to world Y
                    bullet.z = 2.0f;  // Start just in front of player
                    bullet.vx = 0;
                    bullet.vy = 0;
                    bullet.vz = speed * 1.5f;  // Slower bullet speed
                    bullet.created_time = game_time;
                    player.last_shot = game_time;
                    break;
                }
            }
        }

        last_c = button_c;
        last_b = button_b;
    }

    const char* getName() const override {
        return "Afterburner";
    }

    const char* getDescription() const override {
        return "Fly over ocean/terrain, shoot enemies";
    }
};
