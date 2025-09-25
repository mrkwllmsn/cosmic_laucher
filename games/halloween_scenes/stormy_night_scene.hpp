#pragma once

#include "../../game_base.hpp"
#include <cmath>
#include <vector>
#include <random>

struct LightningBranch {
    float x1, y1, x2, y2;
    int generation;
    float intensity;
    bool active;
    float life_timer;
    float max_life;
};

struct CloudParticle {
    float x, y, z;
    float velocity_x, velocity_y;
    float density;
    float noise_offset;
};

struct RainDrop {
    float x, y;
    float speed;
    float length;
    bool active;
};

struct StormTheme {
    uint8_t sky_top_r, sky_top_g, sky_top_b;
    uint8_t sky_bottom_r, sky_bottom_g, sky_bottom_b;
    uint8_t cloud_dark_r, cloud_dark_g, cloud_dark_b;
    uint8_t cloud_light_r, cloud_light_g, cloud_light_b;
    uint8_t lightning_r, lightning_g, lightning_b;
    uint8_t lightning_glow_r, lightning_glow_g, lightning_glow_b;
    uint8_t ground_r, ground_g, ground_b;
    uint8_t rain_r, rain_g, rain_b;
    std::string name;
};

class StormyNightScene {
private:
    static constexpr int MAX_LIGHTNING_BRANCHES = 100;
    static constexpr int MAX_CLOUD_PARTICLES = 80;
    static constexpr int MAX_RAINDROPS = 40;
    static constexpr float LIGHTNING_SPAWN_CHANCE = 0.020f; // Per frame - increased for more strikes
    static constexpr float BRANCH_ANGLE_VARIATION = 45.0f; // Degrees
    static constexpr float BRANCH_LENGTH_DECAY = 0.7f;
    static constexpr float MIN_BRANCH_LENGTH = 2.0f;
    static constexpr float CLOUD_SPEED = 8.0f;
    static constexpr float RAIN_INTENSITY = 0.6f;
    static constexpr float THEME_CHANGE_TIME = 8.0f;
    
    std::vector<LightningBranch> lightning_branches;
    std::vector<CloudParticle> cloud_particles;
    std::vector<RainDrop> raindrops;
    std::vector<StormTheme> themes;
    int current_theme_index;
    StormTheme current_theme;
    
    float time_accumulator;
    float lightning_timer;
    float thunder_flash_timer;
    bool thunder_flash_active;
    float cloud_animation_time;
    float theme_timer;
    uint32_t last_update_time;
    bool last_c_pressed;
    
    // Perlin noise implementation for smooth cloud movement
    static constexpr int NOISE_SIZE = 256;
    float noise_table[NOISE_SIZE];
    
public:
    void init(PicoGraphics* graphics) {
        lightning_branches.clear();
        lightning_branches.reserve(MAX_LIGHTNING_BRANCHES);
        cloud_particles.clear();
        cloud_particles.reserve(MAX_CLOUD_PARTICLES);
        raindrops.clear();
        raindrops.reserve(MAX_RAINDROPS);
        
        time_accumulator = 0.0f;
        lightning_timer = 0.0f;
        thunder_flash_timer = 0.0f;
        thunder_flash_active = false;
        cloud_animation_time = 0.0f;
        theme_timer = 0.0f;
        current_theme_index = 0;

        size_t random_index = std::rand() % themes.size();
        current_theme_index = random_index;

        last_c_pressed = false;
        last_update_time = to_ms_since_boot(get_absolute_time());
        
        initializeNoiseTable();
        initializeThemes();
        initializeCloudParticles();
        initializeRain();
    }
    
    void update(CosmicUnicorn* cosmic = nullptr) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        float dt = (current_time - last_update_time) / 1000.0f;
        last_update_time = current_time;
        
        // Check for manual theme change with C button
        bool c_pressed = false;
        if (cosmic) {
            c_pressed = cosmic->is_pressed(CosmicUnicorn::SWITCH_C);
            if (c_pressed && !last_c_pressed && !themes.empty()) {
                current_theme_index = (current_theme_index + 1) % themes.size();
                current_theme = themes[current_theme_index];
                theme_timer = 0.0f;
            }
        }
        last_c_pressed = c_pressed;
        
        time_accumulator += dt;
        lightning_timer += dt;
        cloud_animation_time += dt;
        theme_timer += dt;
       
       /* // Disabled this for now  
                    if ((rand() % 100) < 70) { // 70% chance for each trail pixel
        // Update theme periodically
        if (theme_timer >= THEME_CHANGE_TIME && !themes.empty()) {
            theme_timer = 0.0f;
            current_theme_index = (current_theme_index + 1) % themes.size();
            current_theme = themes[current_theme_index];
        }
       */ 
        // Update thunder flash
        if (thunder_flash_active) {
            thunder_flash_timer -= dt;
            if (thunder_flash_timer <= 0) {
                thunder_flash_active = false;
            }
        }
        
        // Spawn new lightning strikes randomly
        if ((rand() % 10000) < (LIGHTNING_SPAWN_CHANCE * 10000)) {
            spawnLightningStrike();
        }
        
        updateLightning(dt);
        updateClouds(dt);
        updateRain(dt);
    }
    
    void render(PicoGraphics* graphics) {
        drawStormySky(graphics);
        drawClouds(graphics);
        drawRain(graphics);
        drawLightning(graphics);
        drawGround(graphics);
        
        // Thunder flash overlay
        if (thunder_flash_active) {
            float flash_intensity = thunder_flash_timer / 0.2f; // 0.2 second flash
            if (flash_intensity > 1.0f) flash_intensity = 1.0f;
            
            uint32_t flash_color = graphics->create_pen(
                (int)(255 * flash_intensity * 0.3f),
                (int)(255 * flash_intensity * 0.4f),
                (int)(255 * flash_intensity * 0.7f)
            );
            graphics->set_pen(flash_color);
            
            // Random flash pixels across the screen
            for (int i = 0; i < (int)(flash_intensity * 20); i++) {
                int x = rand() % 32;
                int y = rand() % 32;
                graphics->pixel(Point(x, y));
            }
        }
    }
    
private:
    void initializeNoiseTable() {
        // Simple noise table for cloud movement
        for (int i = 0; i < NOISE_SIZE; i++) {
            noise_table[i] = (float)(rand() % 1000) / 1000.0f;
        }
    }
    
    float noise(float x, float y) {
        int xi = (int)x & (NOISE_SIZE - 1);
        int yi = (int)y & (NOISE_SIZE - 1);
        
        float fx = x - (int)x;
        float fy = y - (int)y;
        
        float n1 = noise_table[xi];
        float n2 = noise_table[(xi + 1) & (NOISE_SIZE - 1)];
        float n3 = noise_table[yi];
        float n4 = noise_table[(yi + 1) & (NOISE_SIZE - 1)];
        
        float i1 = n1 * (1.0f - fx) + n2 * fx;
        float i2 = n3 * (1.0f - fx) + n4 * fx;
        
        return i1 * (1.0f - fy) + i2 * fy;
    }
    
    void initializeThemes() {
        themes.clear();
        
        // Classic stormy night - dark blues and purples
        StormTheme classic_storm;
        classic_storm.sky_top_r = 15; classic_storm.sky_top_g = 15; classic_storm.sky_top_b = 35;
        classic_storm.sky_bottom_r = 5; classic_storm.sky_bottom_g = 5; classic_storm.sky_bottom_b = 20;
        classic_storm.cloud_dark_r = 45; classic_storm.cloud_dark_g = 45; classic_storm.cloud_dark_b = 60;
        classic_storm.cloud_light_r = 70; classic_storm.cloud_light_g = 70; classic_storm.cloud_light_b = 90;
        classic_storm.lightning_r = 255; classic_storm.lightning_g = 255; classic_storm.lightning_b = 255;
        classic_storm.lightning_glow_r = 200; classic_storm.lightning_glow_g = 220; classic_storm.lightning_glow_b = 255;
        classic_storm.ground_r = 20; classic_storm.ground_g = 25; classic_storm.ground_b = 15;
        classic_storm.rain_r = 80; classic_storm.rain_g = 85; classic_storm.rain_b = 95;
        classic_storm.name = "Classic Storm";
        themes.push_back(classic_storm);
        
        // Purple nightmare
        StormTheme purple_nightmare;
        purple_nightmare.sky_top_r = 20; purple_nightmare.sky_top_g = 5; purple_nightmare.sky_top_b = 35;
        purple_nightmare.sky_bottom_r = 10; purple_nightmare.sky_bottom_g = 0; purple_nightmare.sky_bottom_b = 20;
        purple_nightmare.cloud_dark_r = 60; purple_nightmare.cloud_dark_g = 35; purple_nightmare.cloud_dark_b = 75;
        purple_nightmare.cloud_light_r = 85; purple_nightmare.cloud_light_g = 55; purple_nightmare.cloud_light_b = 110;
        purple_nightmare.lightning_r = 255; purple_nightmare.lightning_g = 255; purple_nightmare.lightning_b = 255;
        purple_nightmare.lightning_glow_r = 200; purple_nightmare.lightning_glow_g = 100; purple_nightmare.lightning_glow_b = 255;
        purple_nightmare.ground_r = 25; purple_nightmare.ground_g = 10; purple_nightmare.ground_b = 35;
        purple_nightmare.rain_r = 70; purple_nightmare.rain_g = 50; purple_nightmare.rain_b = 80;
        purple_nightmare.name = "Purple Nightmare";
        themes.push_back(purple_nightmare);
        
        current_theme = themes[0];
    }
    
    void initializeCloudParticles() {
        cloud_particles.clear();
        
        for (int i = 0; i < MAX_CLOUD_PARTICLES; i++) {
            CloudParticle particle;
            particle.x = (float)(rand() % 64 - 16); // -16 to 48 for wrapping
            particle.y = (float)(rand() % 20);      // Top 20 rows
            particle.z = (float)(rand() % 100) / 100.0f; // Depth for layering
            particle.velocity_x = 0.5f + (float)(rand() % 100) / 200.0f; // 0.5 to 1.0
            particle.velocity_y = (float)(rand() % 40 - 20) / 100.0f;   // Slight vertical drift
            particle.density = 0.3f + (float)(rand() % 70) / 100.0f;    // 0.3 to 1.0
            particle.noise_offset = (float)(rand() % 1000) / 10.0f;     // Random noise offset
            cloud_particles.push_back(particle);
        }
    }
    
    void initializeRain() {
        raindrops.clear();
        
        for (int i = 0; i < MAX_RAINDROPS; i++) {
            RainDrop drop;
            drop.x = (float)(rand() % 40 - 4); // -4 to 36 for diagonal movement
            drop.y = (float)(rand() % 40 - 8); // Start above screen
            drop.speed = 15.0f + (float)(rand() % 100) / 10.0f; // 15-25 speed
            drop.length = 2.0f + (float)(rand() % 30) / 10.0f;  // 2-5 length
            drop.active = true;
            raindrops.push_back(drop);
        }
    }
    
    void spawnLightningStrike() {
        // Create main lightning bolt using DLA-inspired algorithm
        float start_x = 8.0f + (float)(rand() % 16); // Start somewhere in middle-upper
        float start_y = 2.0f + (float)(rand() % 8);  // Start in cloud area
        
        // Target ground area
        float target_x = start_x + (float)(rand() % 12 - 6); // Slight horizontal drift
        float target_y = 28.0f + (float)(rand() % 4);        // Ground level
        
        generateLightningBranches(start_x, start_y, target_x, target_y, 0, 1.0f);
        
        // Trigger thunder flash
        thunder_flash_active = true;
        thunder_flash_timer = 0.2f;
    }
    
    void generateLightningBranches(float x1, float y1, float target_x, float target_y, int generation, float intensity) {
        if (generation > 6 || lightning_branches.size() >= MAX_LIGHTNING_BRANCHES) return;
        
        // Calculate direction towards target with some randomness
        float dx = target_x - x1;
        float dy = target_y - y1;
        float distance = sqrt(dx * dx + dy * dy);
        
        if (distance < MIN_BRANCH_LENGTH) return;
        
        // Add randomness to direction (DLA-style random walk with bias)
        float angle = atan2(dy, dx);
        float random_angle = angle + (float)(rand() % (int)(BRANCH_ANGLE_VARIATION * 2) - BRANCH_ANGLE_VARIATION) * M_PI / 180.0f;
        
        // Calculate branch length with decay
        float length = MIN_BRANCH_LENGTH + (distance * BRANCH_LENGTH_DECAY * pow(0.8f, generation));
        length = std::min(length, distance * 0.8f); // Don't overshoot target
        
        float x2 = x1 + cos(random_angle) * length;
        float y2 = y1 + sin(random_angle) * length;
        
        // Create branch
        LightningBranch branch;
        branch.x1 = x1;
        branch.y1 = y1;
        branch.x2 = x2;
        branch.y2 = y2;
        branch.generation = generation;
        branch.intensity = intensity * (0.8f + 0.2f * (float)(rand() % 100) / 100.0f);
        branch.active = true;
        branch.life_timer = 0.0f;
        branch.max_life = 0.15f + (float)(rand() % 50) / 1000.0f; // 0.15-0.2 seconds
        
        lightning_branches.push_back(branch);
        
        // Occasionally create secondary branches
        if (generation < 4 && (rand() % 100) < (40 - generation * 8)) {
            // Create branch in random direction
            float branch_angle = random_angle + (float)(rand() % 90 - 45) * M_PI / 180.0f;
            float branch_length = length * 0.5f;
            
            float branch_x = x2 + cos(branch_angle) * branch_length;
            float branch_y = y2 + sin(branch_angle) * branch_length;
            
            generateLightningBranches(x2, y2, branch_x, branch_y, generation + 1, intensity * 0.6f);
        }
        
        // Continue main path towards target
        if (generation < 3) {
            generateLightningBranches(x2, y2, target_x, target_y, generation + 1, intensity * 0.9f);
        }
    }
    
    void updateLightning(float dt) {
        // Update existing lightning branches
        for (auto it = lightning_branches.begin(); it != lightning_branches.end();) {
            it->life_timer += dt;
            
            if (it->life_timer >= it->max_life) {
                it = lightning_branches.erase(it);
            } else {
                // Fade intensity over time
                float life_ratio = it->life_timer / it->max_life;
                it->intensity *= (1.0f - life_ratio * 0.1f); // Slow fade
                ++it;
            }
        }
    }
    
    void updateClouds(float dt) {
        for (auto& particle : cloud_particles) {
            // Move clouds with perlin noise for natural motion
            float noise_x = noise(particle.x * 0.1f + cloud_animation_time * 0.2f, particle.y * 0.1f);
            float noise_y = noise(particle.x * 0.1f, particle.y * 0.1f + cloud_animation_time * 0.15f);
            
            particle.x += (particle.velocity_x + noise_x * 2.0f) * dt * CLOUD_SPEED;
            particle.y += (particle.velocity_y + noise_y * 0.5f) * dt * CLOUD_SPEED;
            
            // Wrap horizontally
            if (particle.x > 48.0f) particle.x = -16.0f;
            if (particle.x < -16.0f) particle.x = 48.0f;
            
            // Keep in upper area
            if (particle.y > 22.0f) particle.y = 0.0f;
            if (particle.y < 0.0f) particle.y = 22.0f;
        }
    }
    
    void updateRain(float dt) {
        for (auto& drop : raindrops) {
            if (drop.active) {
                drop.x += dt * 2.0f; // Slight diagonal movement (wind effect)
                drop.y += dt * drop.speed;
                
                // Reset when off screen
                if (drop.y > 35.0f || drop.x > 36.0f) {
                    drop.x = (float)(rand() % 40 - 4);
                    drop.y = (float)(rand() % 10 - 8);
                    drop.speed = 15.0f + (float)(rand() % 100) / 10.0f;
                }
            }
        }
    }
    
    void drawStormySky(PicoGraphics* graphics) {
        // Draw gradient stormy sky
        for (int y = 0; y < 32; y++) {
            float gradient_factor = (float)y / 32.0f;
            
            uint8_t r = current_theme.sky_top_r + (current_theme.sky_bottom_r - current_theme.sky_top_r) * gradient_factor;
            uint8_t g = current_theme.sky_top_g + (current_theme.sky_bottom_g - current_theme.sky_top_g) * gradient_factor;
            uint8_t b = current_theme.sky_top_b + (current_theme.sky_bottom_b - current_theme.sky_top_b) * gradient_factor;
            
            uint32_t sky_color = graphics->create_pen(r, g, b);
            graphics->set_pen(sky_color);
            
            for (int x = 0; x < 32; x++) {
                graphics->pixel(Point(x, y));
            }
        }
    }
    
    void drawClouds(PicoGraphics* graphics) {
        // Draw clouds using particle system
        for (const auto& particle : cloud_particles) {
            if (particle.x >= 0 && particle.x < 32 && particle.y >= 0 && particle.y < 32) {
                // Use noise to determine cloud density at this position
                float noise_density = noise(particle.x * 0.3f + cloud_animation_time * 0.1f, 
                                          particle.y * 0.3f + cloud_animation_time * 0.08f);
                
                if (noise_density > 0.35f) { // Threshold for cloud visibility - moderate increase
                    // Choose cloud color based on density and depth
                    uint8_t cloud_r, cloud_g, cloud_b;
                    if (particle.density > 0.7f) {
                        // Dense cloud - darker
                        cloud_r = current_theme.cloud_dark_r;
                        cloud_g = current_theme.cloud_dark_g;
                        cloud_b = current_theme.cloud_dark_b;
                    } else {
                        // Lighter cloud
                        cloud_r = current_theme.cloud_light_r;
                        cloud_g = current_theme.cloud_light_g;
                        cloud_b = current_theme.cloud_light_b;
                    }
                    
                    // Apply depth-based alpha blending effect - moderate visibility
                    float depth_factor = 0.5f + particle.z * 0.4f;
                    cloud_r = (uint8_t)(cloud_r * depth_factor);
                    cloud_g = (uint8_t)(cloud_g * depth_factor);
                    cloud_b = (uint8_t)(cloud_b * depth_factor);
                    
                    uint32_t cloud_color = graphics->create_pen(cloud_r, cloud_g, cloud_b);
                    graphics->set_pen(cloud_color);
                    graphics->pixel(Point((int)particle.x, (int)particle.y));
                    
                    // Add some cloud spread for larger appearance - selective threshold
                    if (noise_density > 0.55f && particle.density > 0.6f) {
                        // Draw adjacent pixels for thicker clouds
                        if ((int)particle.x + 1 < 32) {
                            graphics->pixel(Point((int)particle.x + 1, (int)particle.y));
                        }
                        if ((int)particle.y + 1 < 32) {
                            graphics->pixel(Point((int)particle.x, (int)particle.y + 1));
                        }
                    }
                }
            }
        }
    }
    
    void drawRain(PicoGraphics* graphics) {
        uint32_t rain_color = graphics->create_pen(current_theme.rain_r, current_theme.rain_g, current_theme.rain_b);
        graphics->set_pen(rain_color);
        
        for (const auto& drop : raindrops) {
            if (drop.active && drop.x >= 0 && drop.x < 32 && drop.y >= 0 && drop.y < 32) {
                // Draw raindrop as a short line
                graphics->pixel(Point((int)drop.x, (int)drop.y));
                
                // Draw drop trail based on speed and length
                for (int i = 1; i < (int)drop.length && (drop.y - i) >= 0; i++) {
                    if ((rand() % 100) < 70) { // 70% chance for each trail pixel
                        graphics->pixel(Point((int)drop.x, (int)drop.y - i));
                    }
                }
            }
        }
    }
    
    void drawLightning(PicoGraphics* graphics) {
        for (const auto& branch : lightning_branches) {
            if (branch.active) {
                // Calculate intensity-based color
                float intensity_factor = branch.intensity;
                
                // Draw lightning glow first (wider, dimmer)
                uint32_t glow_color = graphics->create_pen(
                    (int)(current_theme.lightning_glow_r * intensity_factor * 0.6f),
                    (int)(current_theme.lightning_glow_g * intensity_factor * 0.6f),
                    (int)(current_theme.lightning_glow_b * intensity_factor * 0.6f)
                );
                graphics->set_pen(glow_color);
                
                // Draw glow around main lightning bolt
                drawThickLine(graphics, branch.x1, branch.y1, branch.x2, branch.y2, 2);
                
                // Draw main lightning bolt (bright, thin)
                uint32_t lightning_color = graphics->create_pen(
                    (int)(current_theme.lightning_r * intensity_factor),
                    (int)(current_theme.lightning_g * intensity_factor),
                    (int)(current_theme.lightning_b * intensity_factor)
                );
                graphics->set_pen(lightning_color);
                
                drawLine(graphics, branch.x1, branch.y1, branch.x2, branch.y2);
            }
        }
    }
    
    void drawGround(PicoGraphics* graphics) {
        // Draw ground at bottom of screen
        uint32_t ground_color = graphics->create_pen(current_theme.ground_r, current_theme.ground_g, current_theme.ground_b);
        graphics->set_pen(ground_color);
        
        for (int y = 28; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                // Add some texture to ground with noise
                float ground_noise = noise(x * 0.5f, y * 0.5f + time_accumulator);
                if (ground_noise > 0.3f) {
                    graphics->pixel(Point(x, y));
                }
            }
        }
    }
    
    void drawLine(PicoGraphics* graphics, float x1, float y1, float x2, float y2) {
        int ix1 = (int)x1, iy1 = (int)y1, ix2 = (int)x2, iy2 = (int)y2;
        
        int dx = abs(ix2 - ix1);
        int dy = abs(iy2 - iy1);
        int sx = (ix1 < ix2) ? 1 : -1;
        int sy = (iy1 < iy2) ? 1 : -1;
        int err = dx - dy;
        
        while (true) {
            if (ix1 >= 0 && ix1 < 32 && iy1 >= 0 && iy1 < 32) {
                graphics->pixel(Point(ix1, iy1));
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
    
    void drawThickLine(PicoGraphics* graphics, float x1, float y1, float x2, float y2, int thickness) {
        // Draw multiple lines offset by thickness for glow effect
        for (int offset = -thickness/2; offset <= thickness/2; offset++) {
            drawLine(graphics, x1 + offset, y1, x2 + offset, y2);
            drawLine(graphics, x1, y1 + offset, x2, y2 + offset);
        }
    }
};
