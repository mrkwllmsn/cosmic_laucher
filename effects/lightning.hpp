#pragma once

#include "../game_base.hpp"
#include <cmath>
#include <vector>
#include <functional>

class Lightning {
public:
    struct LightningBranch {
        float x1, y1, x2, y2;
        int generation;
        float intensity;
        bool active;
        float life_timer;
        float max_life;
    };
    
    using LightningCallback = std::function<void(float x, float y, float intensity)>;
    
private:
    static constexpr int MAX_LIGHTNING_BRANCHES = 100;
    static constexpr float DEFAULT_SPAWN_CHANCE = 0.020f; // Per frame
    static constexpr float BRANCH_ANGLE_VARIATION = 45.0f; // Degrees
    static constexpr float BRANCH_LENGTH_DECAY = 0.7f;
    static constexpr float MIN_BRANCH_LENGTH = 2.0f;
    
    std::vector<LightningBranch> lightning_branches;
    float lightning_timer;
    float thunder_flash_timer;
    bool thunder_flash_active;
    
    // Customizable properties
    float spawn_chance;
    uint8_t lightning_r, lightning_g, lightning_b;
    uint8_t lightning_glow_r, lightning_glow_g, lightning_glow_b;
    float start_y_min, start_y_max;
    float target_y_min, target_y_max;
    float start_x_min, start_x_max;
    
    // Callback for when lightning strikes
    LightningCallback strike_callback;
    
public:
    Lightning() : 
        lightning_timer(0.0f),
        thunder_flash_timer(0.0f),
        thunder_flash_active(false),
        spawn_chance(DEFAULT_SPAWN_CHANCE),
        lightning_r(255), lightning_g(255), lightning_b(255),
        lightning_glow_r(200), lightning_glow_g(220), lightning_glow_b(255),
        start_y_min(2.0f), start_y_max(10.0f),
        target_y_min(28.0f), target_y_max(32.0f),
        start_x_min(8.0f), start_x_max(24.0f) {
        
        lightning_branches.reserve(MAX_LIGHTNING_BRANCHES);
    }
    
    void init() {
        lightning_branches.clear();
        lightning_timer = 0.0f;
        thunder_flash_timer = 0.0f;
        thunder_flash_active = false;
    }
    
    // Configuration methods
    void setSpawnChance(float chance) { spawn_chance = chance; }
    void setLightningColor(uint8_t r, uint8_t g, uint8_t b) {
        lightning_r = r; lightning_g = g; lightning_b = b;
    }
    void setLightningGlowColor(uint8_t r, uint8_t g, uint8_t b) {
        lightning_glow_r = r; lightning_glow_g = g; lightning_glow_b = b;
    }
    void setStartArea(float x_min, float x_max, float y_min, float y_max) {
        start_x_min = x_min; start_x_max = x_max;
        start_y_min = y_min; start_y_max = y_max;
    }
    void setTargetArea(float y_min, float y_max) {
        target_y_min = y_min; target_y_max = y_max;
    }
    void setStrikeCallback(const LightningCallback& callback) {
        strike_callback = callback;
    }
    
    void update(float dt) {
        lightning_timer += dt;
        
        // Update thunder flash
        if (thunder_flash_active) {
            thunder_flash_timer -= dt;
            if (thunder_flash_timer <= 0) {
                thunder_flash_active = false;
            }
        }
        
        // Spawn new lightning strikes randomly
        if ((rand() % 10000) < (spawn_chance * 10000)) {
            spawnLightningStrike();
        }
        
        updateLightningBranches(dt);
    }
    
    void render(PicoGraphics* graphics) {
        drawLightning(graphics);
        
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
    
    // Check if thunder flash is currently active (useful for other effects)
    bool isThunderFlashing() const { return thunder_flash_active; }
    float getThunderIntensity() const { 
        if (!thunder_flash_active) return 0.0f;
        float intensity = thunder_flash_timer / 0.2f;
        return intensity > 1.0f ? 1.0f : intensity;
    }
    
    // Manual lightning strike
    void triggerStrike(float start_x = -1, float start_y = -1, float target_x = -1, float target_y = -1) {
        if (start_x < 0) start_x = start_x_min + (float)(rand() % (int)(start_x_max - start_x_min));
        if (start_y < 0) start_y = start_y_min + (float)(rand() % (int)(start_y_max - start_y_min));
        if (target_x < 0) target_x = start_x + (float)(rand() % 12 - 6);
        if (target_y < 0) target_y = target_y_min + (float)(rand() % (int)(target_y_max - target_y_min));
        
        generateLightningBranches(start_x, start_y, target_x, target_y, 0, 1.0f);
        
        // Trigger thunder flash
        thunder_flash_active = true;
        thunder_flash_timer = 0.2f;
        
        // Call callback if set
        if (strike_callback) {
            strike_callback(start_x, start_y, 1.0f);
        }
    }
    
private:
    void spawnLightningStrike() {
        // Create main lightning bolt
        float start_x = start_x_min + (float)(rand() % (int)(start_x_max - start_x_min));
        float start_y = start_y_min + (float)(rand() % (int)(start_y_max - start_y_min));
        
        // Target ground area
        float target_x = start_x + (float)(rand() % 12 - 6); // Slight horizontal drift
        float target_y = target_y_min + (float)(rand() % (int)(target_y_max - target_y_min));
        
        generateLightningBranches(start_x, start_y, target_x, target_y, 0, 1.0f);
        
        // Trigger thunder flash
        thunder_flash_active = true;
        thunder_flash_timer = 0.2f;
        
        // Call callback if set
        if (strike_callback) {
            strike_callback(start_x, start_y, 1.0f);
        }
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
    
    void updateLightningBranches(float dt) {
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
    
    void drawLightning(PicoGraphics* graphics) {
        for (const auto& branch : lightning_branches) {
            if (branch.active) {
                // Calculate intensity-based color
                float intensity_factor = branch.intensity;
                
                // Draw lightning glow first (wider, dimmer)
                uint32_t glow_color = graphics->create_pen(
                    (int)(lightning_glow_r * intensity_factor * 0.6f),
                    (int)(lightning_glow_g * intensity_factor * 0.6f),
                    (int)(lightning_glow_b * intensity_factor * 0.6f)
                );
                graphics->set_pen(glow_color);
                
                // Draw glow around main lightning bolt
                drawThickLine(graphics, branch.x1, branch.y1, branch.x2, branch.y2, 2);
                
                // Draw main lightning bolt (bright, thin)
                uint32_t lightning_color = graphics->create_pen(
                    (int)(lightning_r * intensity_factor),
                    (int)(lightning_g * intensity_factor),
                    (int)(lightning_b * intensity_factor)
                );
                graphics->set_pen(lightning_color);
                
                drawLine(graphics, branch.x1, branch.y1, branch.x2, branch.y2);
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