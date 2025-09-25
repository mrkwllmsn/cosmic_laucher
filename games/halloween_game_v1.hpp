#pragma once

#include "../game_base.hpp"
#include "animated_eyes.hpp"
#include <cmath>
#include <vector>

class HalloweenGame : public GameBase {
private:
    enum HalloweenScene {
        CREEPY_EYES,
        PUMPKIN,
        FLYING_BATS,
        WITCH_HAT,
        CANDLE_FLAME,
        FLAME_FACE,
        GHOSTLY_SPIRITS,
        HAUNTED_TREE,
        SKULL_CROSSBONES,
        CASTLE,
        WOLF_HOWLING,
        SCENE_COUNT
    };
    
    HalloweenScene current_scene;
    uint32_t scene_start_time;
    uint32_t scene_duration;
    uint32_t animation_timer;
    
    // Animation states
    
    AnimatedEye animated_eyes;     // For creepy eyes scene
    AnimatedEye skull_eyes;        // For skull scene
    AnimatedEye tree_eyes;         // For tree scene
    float pumpkin_glow_phase;
    std::vector<float> bat_positions;
    std::vector<float> bat_speeds;
    float witch_sparkle_phase;
    
    // Candle flame animation
    float flame_heat[32 * 35]; // Extra rows for flame effect
    float candle_flicker_phase;
    
    // Flame face animation
    float flame_face_heat[32 * 35]; // Separate heat map for flame face
    float face_eye_blink_timer;
    bool face_left_eye_open;
    bool face_right_eye_open;
    float face_mouth_anim_phase;
    
    // Ghost spirits animation
    struct Ghost {
        float x, y;
        float speed_x, speed_y;
        float phase;
        float opacity;
    };
    std::vector<Ghost> ghosts;
    
    // Tree animation
    float tree_sway_phase;
    float tree_glow_phase;
    
    // Skull animation
    float skull_glow_phase;
    
    // Castle animation
    float castle_window_phase;
    
    // Wolf howling animation
    float wolf_howl_phase;
    float moon_glow_phase;
    float mountain_wind_phase;
    float witch_flight_phase;
    
    // Background animation
    float background_phase;
    
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
    
    // Flame effect helper functions
    void setFlameHeat(int x, int y, float v) {
        if (x >= 0 && x < 32 && y >= 0 && y < 35) {
            flame_heat[x + y * 32] = v;
        }
    }
    
    float getFlameHeat(int x, int y) {
        if (x < 0 || x >= 32 || y < 0 || y >= 35) {
            return 0.0f;
        }
        x = x < 0 ? 0 : x;
        x = x >= 32 ? 31 : x;
        return flame_heat[x + y * 32];
    }
    
    // Flame face heat map functions
    void setFlameFaceHeat(int x, int y, float v) {
        if (x >= 0 && x < 32 && y >= 0 && y < 35) {
            flame_face_heat[x + y * 32] = v;
        }
    }
    
    float getFlameFaceHeat(int x, int y) {
        if (x < 0 || x >= 32 || y < 0 || y >= 35) {
            return 0.0f;
        }
        x = x < 0 ? 0 : x;
        x = x >= 32 ? 31 : x;
        return flame_face_heat[x + y * 32];
    }
    
    void drawSpookyBackground() {
        // Nebula-like spooky background with animation
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                // Multi-layered noise for rich nebula effect
                float noise1 = sin(x * 0.1f + background_phase * 0.3f) * 
                              cos(y * 0.15f + background_phase * 0.2f);
                float noise2 = sin(x * 0.08f - background_phase * 0.25f) * 
                              cos(y * 0.12f - background_phase * 0.15f);
                float noise3 = sin((x + y) * 0.06f + background_phase * 0.4f) * 0.5f;
                
                // Combine noise layers for nebula effect
                float nebula = (noise1 + noise2 + noise3) * 0.3f + 0.2f;
                nebula = nebula < 0 ? 0 : nebula;
                nebula = nebula > 0.6f ? 0.6f : nebula;
                
                // Create spooky color palette - purples, deep blues, hints of green
                if (nebula > 0.05f) {
                    uint8_t base_intensity = (uint8_t)(nebula * 200);
                    
                    // Vary colors across the nebula
                    float color_variation = sin(x * 0.2f + background_phase * 0.1f) + 
                                          cos(y * 0.15f - background_phase * 0.08f);
                    
                    uint8_t r = base_intensity * 0.8f;
                    uint8_t g = base_intensity * (0.3f + color_variation * 0.2f);
                    uint8_t b = base_intensity * (1.2f + color_variation * 0.3f);
                    
                    // Clamp values
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;
                    
                    gfx->set_pen(gfx->create_pen(r, g, b));
                } else {
                    // Dark space background
                    gfx->set_pen(gfx->create_pen(0, 0, 8));
                }
                gfx->pixel({x, y});
            }
        }
        
        
        // Always draw bats flying across in background (behind all other scenes)
        for (size_t i = 0; i < bat_positions.size(); i++) {
            int bat_x = (int)bat_positions[i];
            int bat_y = 8 + i * 6 + (int)(sin(animation_timer * 0.01f + i) * 3);
            
            if (bat_x >= -5 && bat_x <= 37) {
                // Draw bat silhouette with slightly dimmed appearance for background effect
                Pen bat_pen = gfx->create_pen(25, 5, 25); // Dark purple instead of pure black
                gfx->set_pen(bat_pen);
                
                // Body
                gfx->pixel({bat_x + 2, bat_y});
                
                // Wings (animated flapping)
                bool wing_up = (animation_timer / 200 + i) % 2 == 0;
                if (wing_up) {
                    // Wings up
                    gfx->pixel({bat_x, bat_y - 1});
                    gfx->pixel({bat_x + 1, bat_y - 1});
                    gfx->pixel({bat_x + 3, bat_y - 1});
                    gfx->pixel({bat_x + 4, bat_y - 1});
                } else {
                    // Wings down
                    gfx->pixel({bat_x, bat_y + 1});
                    gfx->pixel({bat_x + 1, bat_y});
                    gfx->pixel({bat_x + 3, bat_y});
                    gfx->pixel({bat_x + 4, bat_y + 1});
                }
            }
        }
    }
    
    void generateRandomEyes() {
        // Clear existing eyes
        animated_eyes.clear();
        
        // Random number of eye pairs (1-5)
        int eye_count = 1 + (rand() % 5);
        
        // Predefined color options
        struct EyeColor {
            uint8_t r, g, b;
            bool is_triangle;
        };
        
        EyeColor color_options[] = {
            {255, 0, 0, false},    // Red round
            {0, 255, 0, true},     // Green triangle
            {255, 255, 0, false},  // Yellow round
            {255, 0, 255, true},   // Purple triangle
            {0, 255, 255, false},  // Cyan round
            {255, 128, 0, true},   // Orange triangle
            {128, 0, 255, false},  // Purple round
            {0, 255, 128, true},   // Green-cyan triangle
            {255, 64, 128, false}, // Pink round
            {128, 255, 0, true}    // Lime triangle
        };
        
        std::vector<AnimatedEye::EyeConfig> placed_eyes;
        
        for (int i = 0; i < eye_count; i++) {
            AnimatedEye::EyeConfig new_eye;
            bool position_valid = false;
            int attempts = 0;
            
            // Try to find a non-overlapping position
            while (!position_valid && attempts < 50) {
                // Eye pairs need 8 pixels width + 2 margin = 10 pixels
                // Eye pairs need 3 pixels height + 4 margin = 7 pixels  
                new_eye.x = 4 + (rand() % (32 - 12)); // Leave space for 8px width + margin
                new_eye.y = 3 + (rand() % (32 - 8));  // Leave space for 3px height + margin
                
                position_valid = true;
                
                // Check against existing eyes
                for (const auto& existing : placed_eyes) {
                    float dx = abs(new_eye.x - existing.x);
                    float dy = abs(new_eye.y - existing.y);
                    
                    // Eyes need 10px horizontal separation and 7px vertical separation
                    if (dx < 12 && dy < 9) {
                        position_valid = false;
                        break;
                    }
                }
                attempts++;
            }
            
            // If we found a valid position, add the eye pair
            if (position_valid) {
                // Random color selection
                int color_index = rand() % 10;
                
                // Create left eye
                AnimatedEye::EyeConfig left_eye;
                left_eye.x = new_eye.x - 2.5f; // Left eye position
                left_eye.y = new_eye.y;
                left_eye.r = color_options[color_index].r;
                left_eye.g = color_options[color_index].g;
                left_eye.b = color_options[color_index].b;
                left_eye.is_triangle = color_options[color_index].is_triangle;
                left_eye.radiusX = 1.5f; // Individual eye width
                left_eye.radiusY = 1.0f; // Individual eye height
                left_eye.glow_intensity = 0.8f;
                
                // Create right eye  
                AnimatedEye::EyeConfig right_eye;
                right_eye.x = new_eye.x + 2.5f; // Right eye position
                right_eye.y = new_eye.y;
                right_eye.r = color_options[color_index].r;
                right_eye.g = color_options[color_index].g;
                right_eye.b = color_options[color_index].b;
                right_eye.is_triangle = color_options[color_index].is_triangle;
                right_eye.radiusX = 1.5f; // Individual eye width
                right_eye.radiusY = 1.0f; // Individual eye height
                right_eye.glow_intensity = 0.8f;
                
                placed_eyes.push_back(left_eye);
                placed_eyes.push_back(right_eye);
            }
        }
        
        // Add all eyes to the animated eye system
        animated_eyes.addEyes(placed_eyes);
    }
    
    void setupSkullEyes() {
        skull_eyes.clear();
        
        // Add two red glowing eyes for the skull
        AnimatedEye::EyeConfig left_eye;
        left_eye.x = 14; // skull_x - 2 (assuming skull_x = 16)
        left_eye.y = 15; // skull_y - 1 (assuming skull_y = 16)
        left_eye.r = 255;
        left_eye.g = 0;
        left_eye.b = 0;
        left_eye.radiusX = 1.0f;
        left_eye.radiusY = 0.5f;
        left_eye.is_triangle = false;
        left_eye.glow_intensity = 1.0f; // Very intense red glow
        
        AnimatedEye::EyeConfig right_eye;
        right_eye.x = 18; // skull_x + 2
        right_eye.y = 15; // skull_y - 1
        right_eye.r = 255;
        right_eye.g = 0;
        right_eye.b = 0;
        right_eye.radiusX = 1.0f;
        right_eye.radiusY = 0.5f;
        right_eye.is_triangle = false;
        right_eye.glow_intensity = 1.0f; // Very intense red glow
        
        skull_eyes.addEye(left_eye);
        skull_eyes.addEye(right_eye);
    }
    
    void setupTreeEyes() {
        tree_eyes.clear();
        
        // Tree eyes need to move with sway, but for now we'll place them at base position
        // The tree is drawn around tree_base_x = 16, tree_base_y = 24
        int tree_base_x = 16;
        int tree_base_y = 24;
        
        // Add two red glowing eyes for the tree hollow
        AnimatedEye::EyeConfig left_eye;
        left_eye.x = tree_base_x - 1; // eye_x - 1
        left_eye.y = tree_base_y - 6; // eye_y
        left_eye.r = 255;
        left_eye.g = 0;
        left_eye.b = 0;
        left_eye.radiusX = 0.8f;
        left_eye.radiusY = 0.5f;
        left_eye.is_triangle = false;
        left_eye.glow_intensity = 0.9f; // Slightly less intense than skull
        
        AnimatedEye::EyeConfig right_eye;
        right_eye.x = tree_base_x + 1; // eye_x + 1
        right_eye.y = tree_base_y - 6; // eye_y
        right_eye.r = 255;
        right_eye.g = 0;
        right_eye.b = 0;
        right_eye.radiusX = 0.8f;
        right_eye.radiusY = 0.5f;
        right_eye.is_triangle = false;
        right_eye.glow_intensity = 0.9f; // Slightly less intense than skull
        
        tree_eyes.addEye(left_eye);
        tree_eyes.addEye(right_eye);
    }
    
    void drawCreepyEyes() {
        //drawSpookyBackground();
        
        // Update and draw all animated eyes
        animated_eyes.update();
        animated_eyes.draw(background_phase);
    }
    
    void drawPumpkin() {
        drawSpookyBackground();
        
        // Draw jack-o'-lantern in center
        int center_x = 16, center_y = 16;
        
        // Enhanced pulsing glow effect
        float glow_multiplier = 0.8f + 0.4f * sin(pumpkin_glow_phase * 1.5f);
        float secondary_glow = 0.6f + 0.3f * sin(pumpkin_glow_phase * 2.3f);
        
        // Outer glow/aura around entire pumpkin
        for (int y = -10; y <= 10; y++) {
            for (int x = -8; x <= 8; x++) {
                float dist = sqrt(x * x + y * y * 0.8f);
                if (dist > 6.5f && dist <= 9.0f) {
                    float glow_intensity = (9.0f - dist) / 2.5f * glow_multiplier * 0.3f;
                    uint8_t orange_glow = (uint8_t)(glow_intensity * 150);
                    uint8_t red_glow = (uint8_t)(glow_intensity * 80);
                    if (orange_glow > 10) {
                        Pen aura_pen = gfx->create_pen(orange_glow, red_glow, 0);
                        gfx->set_pen(aura_pen);
                        gfx->pixel({center_x + x, center_y + y});
                    }
                }
            }
        }
        
        // Pumpkin body (orange circle) with enhanced pulsing
        for (int y = -8; y <= 8; y++) {
            for (int x = -6; x <= 6; x++) {
                float dist = sqrt(x * x + y * y * 0.8f);
                if (dist <= 6.5f) {
                    // Multi-layered color with depth
                    float depth_factor = (6.5f - dist) / 6.5f;
                    uint8_t base_orange = (uint8_t)(180 + sin(pumpkin_glow_phase) * 75 * glow_multiplier);
                    uint8_t orange_intensity = (uint8_t)(base_orange * (0.7f + depth_factor * 0.3f));
                    uint8_t red_component = (uint8_t)(orange_intensity * 0.6f);
                    
                    // Add flickering candle effect
                    float flicker = sin(pumpkin_glow_phase * 4.0f + x * 0.3f + y * 0.2f) * 0.1f + 1.0f;
                    orange_intensity = (uint8_t)(orange_intensity * flicker);
                    if (orange_intensity > 255) orange_intensity = 255;
                    
                    Pen pumpkin_pen = gfx->create_pen(orange_intensity, red_component, 0);
                    gfx->set_pen(pumpkin_pen);
                    gfx->pixel({center_x + x, center_y + y});
                }
            }
        }
        
        // Enhanced vertical ridges with shading
        for (int y = center_y - 6; y <= center_y + 6; y++) {
            uint8_t ridge_intensity = (uint8_t)(140 + sin(pumpkin_glow_phase * 0.8f) * 40);
            Pen ridge_pen = gfx->create_pen(ridge_intensity, ridge_intensity * 0.5f, 0);
            gfx->set_pen(ridge_pen);
            gfx->pixel({center_x - 3, y});
            gfx->pixel({center_x, y});
            gfx->pixel({center_x + 3, y});
        }
        
        // Eyes (triangular) with animated inner glow
        Pen black_pen = gfx->create_pen(0, 0, 0);
        gfx->set_pen(black_pen);
        
        // Left eye
        gfx->pixel({center_x - 3, center_y - 2});
        gfx->pixel({center_x - 2, center_y - 1});
        gfx->pixel({center_x - 1, center_y - 1});
        gfx->pixel({center_x - 2, center_y});
        
        // Right eye
        gfx->pixel({center_x + 3, center_y - 2});
        gfx->pixel({center_x + 2, center_y - 1});
        gfx->pixel({center_x + 1, center_y - 1});
        gfx->pixel({center_x + 2, center_y});
        
        // Enhanced mouth (wider jagged smile)
        for (int x = -4; x <= 4; x++) {
            gfx->pixel({center_x + x, center_y + 3});
            if (x % 2 == 0 && x != 0) {
                gfx->pixel({center_x + x, center_y + 2});
            }
            if (x == -2 || x == 2) {
                gfx->pixel({center_x + x, center_y + 4});
            }
        }
        // Add teeth
        gfx->pixel({center_x - 1, center_y + 2});
        gfx->pixel({center_x + 1, center_y + 2});
        
        // Enhanced stem with leaves
        uint8_t stem_green = (uint8_t)(80 + sin(pumpkin_glow_phase * 0.5f) * 30);
        Pen stem_pen = gfx->create_pen(0, stem_green, 0);
        gfx->set_pen(stem_pen);
        gfx->pixel({center_x, center_y - 9});
        gfx->pixel({center_x, center_y - 10});
        gfx->pixel({center_x, center_y - 11});
        // Small leaves
        gfx->pixel({center_x - 1, center_y - 10});
        gfx->pixel({center_x + 1, center_y - 10});
        
        // Enhanced animated eye glow that pulses with different intensities
        float eye_glow_intensity = sin(pumpkin_glow_phase * 2.0f) * 0.5f + 0.5f;
        if (eye_glow_intensity > 0.3f) {
            uint8_t glow_red = (uint8_t)(eye_glow_intensity * 255);
            uint8_t glow_orange = (uint8_t)(eye_glow_intensity * 200);
            uint8_t glow_yellow = (uint8_t)(eye_glow_intensity * 100);
            
            // Multi-layer eye glow
            Pen inner_glow = gfx->create_pen(glow_red, glow_orange, glow_yellow);
            gfx->set_pen(inner_glow);
            
            // Inner glow
            gfx->pixel({center_x - 2, center_y - 1});
            gfx->pixel({center_x + 2, center_y - 1});
            
            // Outer glow (dimmer)
            Pen outer_glow = gfx->create_pen(glow_red/2, glow_orange/2, 0);
            gfx->set_pen(outer_glow);
            gfx->pixel({center_x - 4, center_y - 1});
            gfx->pixel({center_x + 4, center_y - 1});
            gfx->pixel({center_x - 3, center_y - 3});
            gfx->pixel({center_x + 3, center_y - 3});
            gfx->pixel({center_x - 3, center_y});
            gfx->pixel({center_x + 3, center_y});
        }
        
        // Animated sparkles around the pumpkin for magic effect
        if (secondary_glow > 0.7f) {
            for (int i = 0; i < 6; i++) {
                float angle = pumpkin_glow_phase * 0.5f + i * 1.047f; // 60 degrees apart
                int sparkle_x = center_x + (int)(cos(angle) * (10 + sin(pumpkin_glow_phase * 3 + i) * 2));
                int sparkle_y = center_y + (int)(sin(angle) * (8 + cos(pumpkin_glow_phase * 2.5f + i) * 2));
                
                if (sparkle_x >= 0 && sparkle_x < 32 && sparkle_y >= 0 && sparkle_y < 32) {
                    uint8_t sparkle_intensity = (uint8_t)(secondary_glow * 150);
                    Pen sparkle_pen = gfx->create_pen(sparkle_intensity, sparkle_intensity * 0.7f, 0);
                    gfx->set_pen(sparkle_pen);
                    gfx->pixel({sparkle_x, sparkle_y});
                }
            }
        }
    }
    
    void drawFlyingBats() {
        drawSpookyBackground();
        
        // Add the large moon from wolf scene for dramatic effect
        int moon_x = 20;
        int moon_y = 8;
        int moon_radius = 6;
        
        // Moon glow/aura
        float glow_intensity = 0.8f + 0.2f * sin(moon_glow_phase * 1.2f);
        for (int dy = -moon_radius - 2; dy <= moon_radius + 2; dy++) {
            for (int dx = -moon_radius - 2; dx <= moon_radius + 2; dx++) {
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > moon_radius && dist <= moon_radius + 2.5f) {
                    float glow_strength = (moon_radius + 2.5f - dist) / 2.5f * glow_intensity * 0.4f;
                    uint8_t glow_val = (uint8_t)(glow_strength * 150);
                    if (glow_val > 8) {
                        gfx->set_pen(gfx->create_pen(glow_val, glow_val, glow_val + 20));
                        gfx->pixel({moon_x + dx, moon_y + dy});
                    }
                }
            }
        }
        
        // Main moon body
        for (int dy = -moon_radius; dy <= moon_radius; dy++) {
            for (int dx = -moon_radius; dx <= moon_radius; dx++) {
                float dist = sqrt(dx * dx + dy * dy);
                if (dist <= moon_radius) {
                    // Create subtle moon surface texture
                    float surface_variation = sin(dx * 0.8f + moon_glow_phase * 0.3f) * 
                                            cos(dy * 0.9f - moon_glow_phase * 0.2f) * 0.1f;
                    float moon_brightness = (0.85f + surface_variation) * glow_intensity;
                    
                    uint8_t moon_white = (uint8_t)(moon_brightness * 240);
                    uint8_t moon_yellow = (uint8_t)(moon_brightness * 220);
                    
                    // Add some lunar depth/shading
                    float depth_factor = (moon_radius - dist) / moon_radius;
                    moon_white = (uint8_t)(moon_white * (0.7f + depth_factor * 0.3f));
                    moon_yellow = (uint8_t)(moon_yellow * (0.7f + depth_factor * 0.3f));
                    
                    gfx->set_pen(gfx->create_pen(moon_white, moon_yellow, moon_yellow * 0.8f));
                    gfx->pixel({moon_x + dx, moon_y + dy});
                }
            }
        }
        
        // Moon craters for detail
        gfx->set_pen(gfx->create_pen(160, 150, 120));
        gfx->pixel({moon_x - 2, moon_y - 1});
        gfx->pixel({moon_x - 1, moon_y - 1});
        gfx->pixel({moon_x + 1, moon_y + 2});
        gfx->pixel({moon_x + 3, moon_y - 2});
        
        // Draw bats flying across screen (some will silhouette against the moon)
        for (size_t i = 0; i < bat_positions.size(); i++) {
            int bat_x = (int)bat_positions[i];
            int bat_y = 8 + i * 6 + (int)(sin(animation_timer * 0.01f + i) * 3);
            
            if (bat_x >= -5 && bat_x <= 37) {
                // Draw bat silhouette
                Pen bat_pen = gfx->create_pen(0, 0, 0);
                gfx->set_pen(bat_pen);
                
                // Body
                gfx->pixel({bat_x + 2, bat_y});
                
                // Wings (animated flapping)
                bool wing_up = (animation_timer / 200 + i) % 2 == 0;
                if (wing_up) {
                    // Wings up
                    gfx->pixel({bat_x, bat_y - 1});
                    gfx->pixel({bat_x + 1, bat_y - 1});
                    gfx->pixel({bat_x + 3, bat_y - 1});
                    gfx->pixel({bat_x + 4, bat_y - 1});
                } else {
                    // Wings down
                    gfx->pixel({bat_x, bat_y + 1});
                    gfx->pixel({bat_x + 1, bat_y});
                    gfx->pixel({bat_x + 3, bat_y});
                    gfx->pixel({bat_x + 4, bat_y + 1});
                }
            }
        }
    }
    
    void drawWitchHat() {
        drawSpookyBackground();
        
        // Draw witch hat in center
        int center_x = 16, center_y = 20;
        
        // Hat brim
        Pen hat_pen = gfx->create_pen(50, 0, 50);
        gfx->set_pen(hat_pen);
        for (int x = -8; x <= 8; x++) {
            gfx->pixel({center_x + x, center_y});
            gfx->pixel({center_x + x, center_y + 1});
        }
        
        // Hat cone
        for (int y = 0; y < 15; y++) {
            int width = 6 - (y / 3);
            if (width < 1) width = 1;
            
            for (int x = -width; x <= width; x++) {
                gfx->pixel({center_x + x, center_y - y - 1});
            }
        }
        
        // Hat tip
        gfx->pixel({center_x, center_y - 16});
        
        // Sparkles around hat
        for (int i = 0; i < 8; i++) {
            float angle = witch_sparkle_phase + i * 0.785f; // 45 degrees apart
            int sparkle_x = center_x + (int)(cos(angle) * (8 + sin(witch_sparkle_phase * 2) * 2));
            int sparkle_y = center_y - 8 + (int)(sin(angle) * (8 + cos(witch_sparkle_phase * 2) * 2));
            
            if (sparkle_x >= 0 && sparkle_x < 32 && sparkle_y >= 0 && sparkle_y < 32) {
                uint8_t r, g, b;
                hsv_to_rgb(fmod(witch_sparkle_phase * 60 + i * 45, 360), 1.0f, 
                          0.5f + sin(witch_sparkle_phase * 3 + i) * 0.5f, r, g, b);
                Pen sparkle_pen = gfx->create_pen(r, g, b);
                gfx->set_pen(sparkle_pen);
                gfx->pixel({sparkle_x, sparkle_y});
            }
        }
        
        // Moon in background
        Pen moon_pen = gfx->create_pen(200, 200, 150);
        gfx->set_pen(moon_pen);
        for (int y = -3; y <= 3; y++) {
            for (int x = -3; x <= 3; x++) {
                if (x * x + y * y <= 9) {
                    gfx->pixel({25 + x, 6 + y});
                }
            }
        }
    }
    
    void drawCandleFlame() {
        drawSpookyBackground();
        
        // Draw candle base in center bottom
        int candle_x = 16;
        int candle_bottom = 28;
        
        // Candle body (wax)
        Pen candle_pen = gfx->create_pen(200, 180, 120);
        gfx->set_pen(candle_pen);
        for (int y = candle_bottom; y >= candle_bottom - 8; y--) {
            for (int x = candle_x - 2; x <= candle_x + 2; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Candle wick
        Pen wick_pen = gfx->create_pen(60, 40, 20);
        gfx->set_pen(wick_pen);
        gfx->pixel({candle_x, candle_bottom - 9});
        gfx->pixel({candle_x, candle_bottom - 10});
        
        // Update flame heat map
        for (int y = 0; y < 35; y++) {
            for (int x = 0; x < 32; x++) {
                // Average surrounding pixels for flame effect
                float average = (getFlameHeat(x, y) + getFlameHeat(x, y + 2) + 
                               getFlameHeat(x, y + 1) + getFlameHeat(x - 1, y + 1) + 
                               getFlameHeat(x + 1, y + 1)) / 5.0f;
                
                // Damping factor
                average *= 0.96f;
                
                setFlameHeat(x, y, average);
            }
        }
        
        // Clear bottom and add new heat sources near wick
        for (int x = 0; x < 32; x++) {
            setFlameHeat(x, 34, 0.0f);
        }
        
        // Add flame heat source above wick with flicker
        float flicker_intensity = 0.8f + 0.4f * sin(candle_flicker_phase * 8.0f);
        int flame_base_y = candle_bottom - 11;
        
        for (int i = 0; i < 3; i++) {
            int fx = candle_x + (rand() % 3) - 1;
            setFlameHeat(fx, flame_base_y, flicker_intensity);
            setFlameHeat(fx, flame_base_y + 1, flicker_intensity * 0.8f);
        }
        
        // Draw flame based on heat map
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                float heat_value = getFlameHeat(x, y + 3); // Offset for display
                
                if (heat_value > 0.5f) {
                    gfx->set_pen(gfx->create_pen(255, 255, 180)); // Hot white/yellow
                } else if (heat_value > 0.4f) {
                    gfx->set_pen(gfx->create_pen(255, 200, 0)); // Bright yellow
                } else if (heat_value > 0.3f) {
                    gfx->set_pen(gfx->create_pen(255, 100, 0)); // Orange
                } else if (heat_value > 0.2f) {
                    gfx->set_pen(gfx->create_pen(200, 50, 0)); // Red
                } else if (heat_value > 0.1f) {
                    gfx->set_pen(gfx->create_pen(100, 20, 0)); // Dark red
                }
                
                if (heat_value > 0.1f) {
                    gfx->pixel({x, y});
                }
            }
        }
        
        // Add some wax drips for effect
        Pen drip_pen = gfx->create_pen(180, 160, 100);
        gfx->set_pen(drip_pen);
        float drip_phase = sin(candle_flicker_phase * 0.3f);
        if (drip_phase > 0.5f) {
            gfx->pixel({candle_x - 2, candle_bottom + 1});
            gfx->pixel({candle_x - 2, candle_bottom + 2});
        }
        if (drip_phase < -0.3f) {
            gfx->pixel({candle_x + 2, candle_bottom + 1});
        }
    }
    
    void drawFlameFace() {
        // No background - pure flame effect covering entire screen
        
        // Update fullscreen flame heat map
        for (int y = 0; y < 35; y++) {
            for (int x = 0; x < 32; x++) {
                // Average surrounding pixels for flame effect
                float average = (getFlameFaceHeat(x, y) + getFlameFaceHeat(x, y + 2) + 
                               getFlameFaceHeat(x, y + 1) + getFlameFaceHeat(x - 1, y + 1) + 
                               getFlameFaceHeat(x + 1, y + 1)) / 5.0f;
                
                // Damping factor for realistic flame decay
                average *= 0.94f;
                
                setFlameFaceHeat(x, y, average);
            }
        }
        
        // Clear bottom rows and add multiple heat sources across bottom
        for (int x = 0; x < 32; x++) {
            setFlameFaceHeat(x, 34, 0.0f);
            setFlameFaceHeat(x, 33, 0.0f);
        }
        
        // Add distributed flame heat sources at bottom with intense flicker
        float flicker_intensity1 = 0.9f + 0.3f * sin(candle_flicker_phase * 12.0f);
        float flicker_intensity2 = 0.85f + 0.35f * sin(candle_flicker_phase * 10.0f + 2.1f);
        
        // Multiple flame sources across the bottom
        for (int base_x = 2; base_x < 30; base_x += 4) {
            float local_flicker = flicker_intensity1 + 0.1f * sin(candle_flicker_phase * 15.0f + base_x * 0.5f);
            for (int spread = -1; spread <= 1; spread++) {
                int fx = base_x + spread + (rand() % 3) - 1;
                if (fx >= 0 && fx < 32) {
                    setFlameFaceHeat(fx, 32, local_flicker);
                    setFlameFaceHeat(fx, 31, local_flicker * 0.9f);
                    setFlameFaceHeat(fx, 30, local_flicker * 0.8f);
                }
            }
        }
        
        // Add some mid-level heat sources for more turbulent flames
        for (int mid_x = 4; mid_x < 28; mid_x += 6) {
            float mid_flicker = flicker_intensity2 * 0.7f;
            for (int spread = -1; spread <= 1; spread++) {
                int fx = mid_x + spread;
                if (fx >= 0 && fx < 32) {
                    setFlameFaceHeat(fx, 20, mid_flicker);
                    setFlameFaceHeat(fx, 19, mid_flicker * 0.8f);
                }
            }
        }
        
        // Draw fullscreen flame based on heat map
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                float heat_value = getFlameFaceHeat(x, y + 3); // Offset for display
                
                // Enhanced color mapping for more intense flames
                if (heat_value > 0.6f) {
                    // Intense white/yellow core
                    uint8_t intensity = (uint8_t)(255 * heat_value);
                    gfx->set_pen(gfx->create_pen(255, 255, intensity));
                } else if (heat_value > 0.5f) {
                    // Bright yellow
                    uint8_t yellow = (uint8_t)(255 * heat_value);
                    gfx->set_pen(gfx->create_pen(255, yellow, 60));
                } else if (heat_value > 0.4f) {
                    // Orange
                    uint8_t orange = (uint8_t)(255 * heat_value);
                    gfx->set_pen(gfx->create_pen(orange, orange * 0.6f, 0));
                } else if (heat_value > 0.3f) {
                    // Red-orange
                    uint8_t red = (uint8_t)(255 * heat_value);
                    gfx->set_pen(gfx->create_pen(red, red * 0.3f, 0));
                } else if (heat_value > 0.2f) {
                    // Deep red
                    uint8_t deep_red = (uint8_t)(200 * heat_value);
                    gfx->set_pen(gfx->create_pen(deep_red, 0, 0));
                } else if (heat_value > 0.1f) {
                    // Dark red embers
                    uint8_t ember = (uint8_t)(120 * heat_value);
                    gfx->set_pen(gfx->create_pen(ember, ember * 0.2f, 0));
                }
                
                if (heat_value > 0.1f) {
                    gfx->pixel({x, y});
                }
            }
        }
        
        // Now draw the black creepy face over the flames
        int face_center_x = 16;
        int face_center_y = 16;
        
        // Eyes - animated blinking
        Pen black_pen = gfx->create_pen(0, 0, 0);
        gfx->set_pen(black_pen);
        
        // Left eye (larger, more menacing)
        if (face_left_eye_open) {
            // Eye shape - vertical oval
            for (int ey = -3; ey <= 3; ey++) {
                for (int ex = -2; ex <= 2; ex++) {
                    float eye_dist = sqrt(ex * ex * 1.5f + ey * ey * 0.8f);
                    if (eye_dist <= 2.5f) {
                        gfx->pixel({face_center_x - 6 + ex, face_center_y - 2 + ey});
                    }
                }
            }
        }
        
        // Right eye
        if (face_right_eye_open) {
            // Eye shape - vertical oval
            for (int ey = -3; ey <= 3; ey++) {
                for (int ex = -2; ex <= 2; ex++) {
                    float eye_dist = sqrt(ex * ex * 1.5f + ey * ey * 0.8f);
                    if (eye_dist <= 2.5f) {
                        gfx->pixel({face_center_x + 6 + ex, face_center_y - 2 + ey});
                    }
                }
            }
        }
        
        // Animated mouth - sinister grin that moves
        float mouth_curve = sin(face_mouth_anim_phase) * 2.0f;
        float mouth_width_anim = sin(face_mouth_anim_phase * 0.7f) * 0.5f + 1.0f;
        
        // Main mouth curve
        for (int mx = -8; mx <= 8; mx++) {
            float normalized_x = mx / 8.0f; // -1 to 1
            int mouth_y = face_center_y + 6 + (int)(mouth_curve * normalized_x * normalized_x);
            
            // Draw mouth pixels with varying thickness
            if (abs(mx) < 6 * mouth_width_anim) {
                gfx->pixel({face_center_x + mx, mouth_y});
                if (abs(mx) < 4) {
                    gfx->pixel({face_center_x + mx, mouth_y + 1});
                }
            }
        }
        
        // Mouth corners extending upward (sinister grin)
        for (int corner = 0; corner < 3; corner++) {
            int corner_y = face_center_y + 6 - corner;
            gfx->pixel({face_center_x - 7 + corner/2, corner_y});
            gfx->pixel({face_center_x + 7 - corner/2, corner_y});
        }
        
        // Add some jagged teeth inside the mouth occasionally
        if (sin(face_mouth_anim_phase * 2.0f) > 0.3f) {
            // Upper teeth
            for (int tooth = -2; tooth <= 2; tooth++) {
                if (tooth % 2 == 0) {
                    gfx->pixel({face_center_x + tooth * 2, face_center_y + 5});
                }
            }
        }
    }
    
    void drawWolfHowling() {
        // Dark night sky background with stars
        gfx->set_pen(gfx->create_pen(5, 5, 20));
        gfx->clear();
        
        // Draw stars (small twinkling dots)
        for (int i = 0; i < 15; i++) {
            float star_phase = wolf_howl_phase * 2.0f + i * 0.7f;
            float twinkle = 0.5f + 0.5f * sin(star_phase);
            if (twinkle > 0.7f) {
                uint8_t star_brightness = (uint8_t)(twinkle * 200);
                gfx->set_pen(gfx->create_pen(star_brightness, star_brightness, star_brightness + 50));
                
                // Pseudo-random star positions based on index
                int star_x = (i * 7 + 3) % 32;
                int star_y = (i * 11 + 2) % 15; // Keep stars in upper portion
                gfx->pixel({star_x, star_y});
            }
        }
        
        // Large moon with glow effect
        int moon_x = 20;
        int moon_y = 8;
        int moon_radius = 6;
        
        // Moon glow/aura
        float glow_intensity = 0.8f + 0.2f * sin(moon_glow_phase * 1.2f);
        for (int dy = -moon_radius - 2; dy <= moon_radius + 2; dy++) {
            for (int dx = -moon_radius - 2; dx <= moon_radius + 2; dx++) {
                float dist = sqrt(dx * dx + dy * dy);
                if (dist > moon_radius && dist <= moon_radius + 2.5f) {
                    float glow_strength = (moon_radius + 2.5f - dist) / 2.5f * glow_intensity * 0.4f;
                    uint8_t glow_val = (uint8_t)(glow_strength * 150);
                    if (glow_val > 8) {
                        gfx->set_pen(gfx->create_pen(glow_val, glow_val, glow_val + 20));
                        gfx->pixel({moon_x + dx, moon_y + dy});
                    }
                }
            }
        }
        
        // Main moon body
        for (int dy = -moon_radius; dy <= moon_radius; dy++) {
            for (int dx = -moon_radius; dx <= moon_radius; dx++) {
                float dist = sqrt(dx * dx + dy * dy);
                if (dist <= moon_radius) {
                    // Create subtle moon surface texture
                    float surface_variation = sin(dx * 0.8f + moon_glow_phase * 0.3f) * 
                                            cos(dy * 0.9f - moon_glow_phase * 0.2f) * 0.1f;
                    float moon_brightness = (0.85f + surface_variation) * glow_intensity;
                    
                    uint8_t moon_white = (uint8_t)(moon_brightness * 240);
                    uint8_t moon_yellow = (uint8_t)(moon_brightness * 220);
                    
                    // Add some lunar depth/shading
                    float depth_factor = (moon_radius - dist) / moon_radius;
                    moon_white = (uint8_t)(moon_white * (0.7f + depth_factor * 0.3f));
                    moon_yellow = (uint8_t)(moon_yellow * (0.7f + depth_factor * 0.3f));
                    
                    gfx->set_pen(gfx->create_pen(moon_white, moon_yellow, moon_yellow * 0.8f));
                    gfx->pixel({moon_x + dx, moon_y + dy});
                }
            }
        }
        
        // Moon craters for detail
        gfx->set_pen(gfx->create_pen(160, 150, 120));
        gfx->pixel({moon_x - 2, moon_y - 1});
        gfx->pixel({moon_x - 1, moon_y - 1});
        gfx->pixel({moon_x + 1, moon_y + 2});
        gfx->pixel({moon_x + 3, moon_y - 2});
        
        // Mountain ridge silhouette
        gfx->set_pen(gfx->create_pen(0, 0, 0));
        
        // Create jagged mountain profile with wind animation
        float wind_sway = sin(mountain_wind_phase) * 0.5f;
        
        // Left mountain peak
        for (int x = 0; x < 12; x++) {
            float mountain_height = 26 - (x * x) * 0.08f + sin(x * 0.5f + mountain_wind_phase) * 0.3f;
            for (int y = (int)mountain_height; y < 32; y++) {
                gfx->pixel({x, y});
            }
        }
        
        // Center valley dip
        for (int x = 12; x < 16; x++) {
            float valley_height = 28 + sin(x * 0.8f + mountain_wind_phase * 0.5f) * 0.2f;
            for (int y = (int)valley_height; y < 32; y++) {
                gfx->pixel({x, y});
            }
        }
        
        // Right mountain (lower so wolf is visible against moon)
        for (int x = 16; x < 32; x++) {
            float peak_height = 26 - (x - 24) * (x - 24) * 0.03f + 
                              sin(x * 0.3f + mountain_wind_phase * 0.7f) * 0.4f + wind_sway;
            for (int y = (int)peak_height; y < 32; y++) {
                gfx->pixel({x, y});
            }
        }
        
        // Wolf silhouette positioned to be visible against the moon
        int wolf_x = 18;  // Moved left to be more in front of moon
        int wolf_base_y = 12; // Moved much higher to be against moon (moon y=8, radius=6, so moon spans y: 2-14)
        
        // Wolf howling pose animation
        float howl_intensity = sin(wolf_howl_phase * 1.5f);
        bool is_howling = howl_intensity > 0.3f;
        
        // Wolf body
        for (int x = wolf_x - 2; x <= wolf_x + 1; x++) {
            for (int y = wolf_base_y; y <= wolf_base_y + 2; y++) {
                gfx->pixel({x, y});
            }
        }
        
        // Wolf head and snout
        if (is_howling) {
            // Howling pose - head tilted up toward moon
            gfx->pixel({wolf_x - 1, wolf_base_y - 1});
            gfx->pixel({wolf_x, wolf_base_y - 1});
            gfx->pixel({wolf_x, wolf_base_y - 2});
            gfx->pixel({wolf_x + 1, wolf_base_y - 2});
            gfx->pixel({wolf_x + 1, wolf_base_y - 3}); // Extended snout upward toward moon
            
            // Ears
            gfx->pixel({wolf_x - 2, wolf_base_y - 1});
            gfx->pixel({wolf_x - 1, wolf_base_y - 2});
        } else {
            // Normal pose
            gfx->pixel({wolf_x - 1, wolf_base_y - 1});
            gfx->pixel({wolf_x, wolf_base_y - 1});
            gfx->pixel({wolf_x + 1, wolf_base_y - 1});
            gfx->pixel({wolf_x + 2, wolf_base_y - 1}); // Snout forward
            
            // Ears
            gfx->pixel({wolf_x - 2, wolf_base_y - 1});
            gfx->pixel({wolf_x - 1, wolf_base_y - 2});
        }
        
        // Wolf legs
        gfx->pixel({wolf_x - 2, wolf_base_y + 3});
        gfx->pixel({wolf_x - 1, wolf_base_y + 3});
        gfx->pixel({wolf_x, wolf_base_y + 3});
        gfx->pixel({wolf_x + 1, wolf_base_y + 3});
        
        // Wolf tail
        float tail_sway = sin(wolf_howl_phase * 2.0f + 1.5f) * 0.5f;
        int tail_x = wolf_x - 3 + (int)tail_sway;
        int tail_y = wolf_base_y + 1;
        gfx->pixel({tail_x, tail_y});
        gfx->pixel({tail_x, tail_y + 1});
        
        // Howl effect - visible breath/sound waves when howling
        if (is_howling && howl_intensity > 0.7f) {
            float breath_intensity = (howl_intensity - 0.7f) / 0.3f;
            uint8_t breath_alpha = (uint8_t)(breath_intensity * 100);
            
            gfx->set_pen(gfx->create_pen(breath_alpha, breath_alpha, breath_alpha + 50));
            
            // Breath cloud/sound waves
            for (int i = 0; i < 3; i++) {
                float wave_phase = wolf_howl_phase * 3.0f + i * 1.0f;
                int wave_x = wolf_x + 2 + i * 2 + (int)(sin(wave_phase) * 1.5f);
                int wave_y = wolf_base_y - 3 - i + (int)(cos(wave_phase * 1.2f) * 0.8f);
                
                if (wave_x >= 0 && wave_x < 32 && wave_y >= 0 && wave_y < 32) {
                    gfx->pixel({wave_x, wave_y});
                }
            }
        }
        
        // Flying witch silhouette across the moon
        gfx->set_pen(gfx->create_pen(0, 0, 0));
        
        // Witch flies in a slow arc across the screen
        float witch_cycle = fmod(witch_flight_phase, 6.28f); // Full cycle every 2π
        float witch_progress = witch_cycle / 6.28f; // 0 to 1
        
        // Witch flies from left to right in an arc
        int witch_x = (int)(-5 + witch_progress * 42); // -5 to 37 (off screen to off screen)
        int witch_base_y = moon_y + (int)(sin(witch_progress * 3.14f) * 8); // Arc across moon area
        
        // Only draw witch when she's visible on screen
        if (witch_x >= -3 && witch_x <= 35 && witch_base_y >= 0 && witch_base_y <= 29) {
            // Witch body (small)
            gfx->pixel({witch_x, witch_base_y});
            gfx->pixel({witch_x, witch_base_y + 1});
            
            // Witch hat (pointy)
            gfx->pixel({witch_x - 1, witch_base_y - 1});
            gfx->pixel({witch_x, witch_base_y - 1});
            gfx->pixel({witch_x, witch_base_y - 2});
            
            // Broomstick
            gfx->pixel({witch_x - 2, witch_base_y + 1});
            gfx->pixel({witch_x - 3, witch_base_y + 1});
            gfx->pixel({witch_x - 4, witch_base_y + 1});
            
            // Broom bristles (animated)
            bool bristle_frame = ((int)(witch_flight_phase * 4)) % 2 == 0;
            if (bristle_frame) {
                gfx->pixel({witch_x - 4, witch_base_y});
                gfx->pixel({witch_x - 4, witch_base_y + 2});
                gfx->pixel({witch_x - 5, witch_base_y + 1});
            } else {
                gfx->pixel({witch_x - 5, witch_base_y});
                gfx->pixel({witch_x - 5, witch_base_y + 2});
                gfx->pixel({witch_x - 4, witch_base_y + 1});
            }
            
            // Witch cape flowing behind
            if (witch_x > 2) { // Only draw cape when there's room
                float cape_flow = sin(witch_flight_phase * 3.0f) * 0.5f;
                gfx->pixel({witch_x - 1, witch_base_y + 2 + (int)cape_flow});
                gfx->pixel({witch_x - 2, witch_base_y + 1 + (int)cape_flow});
            }
        }
        
        // Atmospheric fog/mist at base of mountains
        gfx->set_pen(gfx->create_pen(25, 25, 35));
        for (int i = 0; i < 10; i++) {
            float mist_phase = mountain_wind_phase * 0.4f + i * 0.6f;
            float mist_x = i * 3.2f + sin(mist_phase) * 2.0f;
            float mist_y = 30 + sin(mist_phase * 1.3f) * 0.8f;
            
            if (mist_x >= 0 && mist_x < 32 && mist_y >= 0 && mist_y < 32) {
                gfx->pixel({(int)mist_x, (int)mist_y});
            }
        }
    }
    
    void drawGhostlySpirits() {
        //drawSpookyBackground();
        
        // Draw floating ghosts
        for (auto& ghost : ghosts) {
            // Calculate ghost shape
            int ghost_size = 4;
            float opacity_mult = ghost.opacity * (0.7f + 0.3f * sin(ghost.phase));
            
            // Ghost body (ethereal white/blue)
            for (int dy = -ghost_size; dy <= ghost_size; dy++) {
                for (int dx = -ghost_size; dx <= ghost_size; dx++) {
                    float dist = sqrt(dx * dx + dy * dy);
                    if (dist <= ghost_size) {
                        float intensity = (1.0f - dist / ghost_size) * opacity_mult;
                        if (intensity > 0.1f) {
                            uint8_t white_val = (uint8_t)(intensity * 200);
                            uint8_t blue_val = (uint8_t)(intensity * 150);
                            
                            int px = (int)ghost.x + dx;
                            int py = (int)ghost.y + dy;
                            
                            if (px >= 0 && px < 32 && py >= 0 && py < 32) {
                                gfx->set_pen(gfx->create_pen(white_val, white_val, white_val + blue_val/2));
                                gfx->pixel({px, py});
                            }
                        }
                    }
                }
            }
            
            // Ghost eyes (dark holes)
            if (ghost.opacity > 0.5f) {
                gfx->set_pen(gfx->create_pen(0, 0, 0));
                gfx->pixel({(int)ghost.x - 1, (int)ghost.y - 1});
                gfx->pixel({(int)ghost.x + 1, (int)ghost.y - 1});
            }
            
            // Trailing effect
            for (int i = 1; i <= 3; i++) {
                int trail_x = (int)(ghost.x - ghost.speed_x * i * 2);
                int trail_y = (int)(ghost.y - ghost.speed_y * i * 2);
                
                if (trail_x >= 0 && trail_x < 32 && trail_y >= 0 && trail_y < 32) {
                    float trail_intensity = ghost.opacity * (0.3f - i * 0.1f);
                    if (trail_intensity > 0) {
                        uint8_t trail_val = (uint8_t)(trail_intensity * 100);
                        gfx->set_pen(gfx->create_pen(trail_val, trail_val, trail_val + 20));
                        gfx->pixel({trail_x, trail_y});
                    }
                }
            }
        }
    }
    
    void drawHauntedTree() {
        drawSpookyBackground();
        
        int tree_base_x = 16;
        int tree_base_y = 30;
        
        // Tree trunk with sway
        float sway_offset = sin(tree_sway_phase) * 2.0f;
        Pen trunk_pen = gfx->create_pen(80, 60, 40);
        gfx->set_pen(trunk_pen);
        
        for (int y = tree_base_y; y >= tree_base_y - 12; y--) {
            float height_factor = (float)(tree_base_y - y) / 12.0f;
            int sway_x = tree_base_x + (int)(sway_offset * height_factor);
            
            for (int x = sway_x - 1; x <= sway_x + 1; x++) {
                if (x >= 0 && x < 32 && y >= 0 && y < 32) {
                    gfx->pixel({x, y});
                }
            }
        }
        
        // Gnarled branches
        Pen branch_pen = gfx->create_pen(60, 45, 30);
        gfx->set_pen(branch_pen);
        
        // Left branch
        int branch_start_y = tree_base_y - 8;
        for (int i = 0; i < 6; i++) {
            int branch_x = tree_base_x - 2 - i + (int)(sway_offset * 0.5f);
            int branch_y = branch_start_y - i/2 + (int)(sin(tree_sway_phase + i * 0.5f) * 1.5f);
            
            if (branch_x >= 0 && branch_x < 32 && branch_y >= 0 && branch_y < 32) {
                gfx->pixel({branch_x, branch_y});
            }
        }
        
        // Right branch
        for (int i = 0; i < 5; i++) {
            int branch_x = tree_base_x + 2 + i + (int)(sway_offset * 0.5f);
            int branch_y = branch_start_y - 2 - i/2 + (int)(sin(tree_sway_phase + i * 0.3f) * 1.2f);
            
            if (branch_x >= 0 && branch_x < 32 && branch_y >= 0 && branch_y < 32) {
                gfx->pixel({branch_x, branch_y});
            }
        }
        
        // Draw animated glowing eyes in tree hollow
        tree_eyes.update();
        tree_eyes.draw(tree_glow_phase);
        
        // Hanging moss/cobwebs
        Pen web_pen = gfx->create_pen(40, 40, 40);
        gfx->set_pen(web_pen);
        
        for (int i = 0; i < 4; i++) {
            float web_sway = sin(tree_sway_phase * 1.5f + i * 0.8f) * 0.5f;
            int web_x = tree_base_x - 4 + i * 2 + (int)(web_sway + sway_offset * 0.2f);
            int web_length = 3 + i % 2;
            
            for (int j = 0; j < web_length; j++) {
                int web_y = tree_base_y - 15 + j;
                if (web_x >= 0 && web_x < 32 && web_y >= 0 && web_y < 32) {
                    gfx->pixel({web_x, web_y});
                }
            }
        }
    }
    
    void drawSkullCrossbones() {
        drawSpookyBackground();
        
        int skull_x = 16;
        int skull_y = 16;
        
        // Skull glow effect
        float glow_intensity = 0.8f + 0.3f * sin(skull_glow_phase * 1.5f);
        
        // Crossbones behind skull
        Pen bone_pen = gfx->create_pen((uint8_t)(180 * glow_intensity), 
                                     (uint8_t)(170 * glow_intensity), 
                                     (uint8_t)(140 * glow_intensity));
        gfx->set_pen(bone_pen);
        
        // Diagonal crossbones
        for (int i = -8; i <= 8; i++) {
            // Upper left to lower right
            if (skull_x + i >= 0 && skull_x + i < 32 && skull_y + i >= 0 && skull_y + i < 32) {
                gfx->pixel({skull_x + i, skull_y + i});
            }
            // Upper right to lower left  
            if (skull_x - i >= 0 && skull_x - i < 32 && skull_y + i >= 0 && skull_y + i < 32) {
                gfx->pixel({skull_x - i, skull_y + i});
            }
        }
        
        // Bone ends (clubs)
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                // Top ends
                if (skull_x - 7 + dx >= 0 && skull_x - 7 + dx < 32 && skull_y - 7 + dy >= 0 && skull_y - 7 + dy < 32) {
                    gfx->pixel({skull_x - 7 + dx, skull_y - 7 + dy});
                }
                if (skull_x + 7 + dx >= 0 && skull_x + 7 + dx < 32 && skull_y - 7 + dy >= 0 && skull_y - 7 + dy < 32) {
                    gfx->pixel({skull_x + 7 + dx, skull_y - 7 + dy});
                }
                // Bottom ends
                if (skull_x - 7 + dx >= 0 && skull_x - 7 + dx < 32 && skull_y + 7 + dy >= 0 && skull_y + 7 + dy < 32) {
                    gfx->pixel({skull_x - 7 + dx, skull_y + 7 + dy});
                }
                if (skull_x + 7 + dx >= 0 && skull_x + 7 + dx < 32 && skull_y + 7 + dy >= 0 && skull_y + 7 + dy < 32) {
                    gfx->pixel({skull_x + 7 + dx, skull_y + 7 + dy});
                }
            }
        }
        
        // Skull shape
        Pen skull_pen = gfx->create_pen((uint8_t)(220 * glow_intensity), 
                                      (uint8_t)(220 * glow_intensity), 
                                      (uint8_t)(200 * glow_intensity));
        gfx->set_pen(skull_pen);
        
        // Skull outline (rounded)
        for (int y = -4; y <= 2; y++) {
            for (int x = -3; x <= 3; x++) {
                if ((x * x + y * y) <= 12) { // Rough circle
                    gfx->pixel({skull_x + x, skull_y + y});
                }
            }
        }
        
        // Jaw
        for (int x = -2; x <= 2; x++) {
            gfx->pixel({skull_x + x, skull_y + 3});
        }
        gfx->pixel({skull_x - 1, skull_y + 4});
        gfx->pixel({skull_x, skull_y + 4});
        gfx->pixel({skull_x + 1, skull_y + 4});
        
        // Eye sockets (black)
        gfx->set_pen(gfx->create_pen(0, 0, 0));
        gfx->pixel({skull_x - 2, skull_y - 1});
        gfx->pixel({skull_x - 1, skull_y - 1});
        gfx->pixel({skull_x - 2, skull_y});
        
        gfx->pixel({skull_x + 2, skull_y - 1});
        gfx->pixel({skull_x + 1, skull_y - 1});
        gfx->pixel({skull_x + 2, skull_y});
        
        // Nose cavity
        gfx->pixel({skull_x, skull_y + 1});
        
        // Teeth
        gfx->pixel({skull_x - 1, skull_y + 3});
        gfx->pixel({skull_x + 1, skull_y + 3});
        
        // Draw animated glowing eyes
        skull_eyes.update();
        skull_eyes.draw(skull_glow_phase);
    }
    
    void drawCastle() {
        drawSpookyBackground();
        
        // Draw crescent moon in upper left corner
        int moon_x = 8;
        int moon_y = 6;
        Pen moon_pen = gfx->create_pen(220, 220, 180);
        gfx->set_pen(moon_pen);
        
        // Draw crescent moon shape
        for (int dy = -3; dy <= 3; dy++) {
            for (int dx = -3; dx <= 3; dx++) {
                float dist = sqrt(dx * dx + dy * dy);
                // Main circle
                if (dist <= 3.0f) {
                    // Create crescent by excluding a portion
                    float crescent_x = dx + 1.5f; // Offset for crescent effect
                    float crescent_dist = sqrt(crescent_x * crescent_x + dy * dy);
                    if (crescent_dist > 2.5f) { // Only draw the crescent part
                        gfx->pixel({moon_x + dx, moon_y + dy});
                    }
                }
            }
        }
        
        // Castle silhouette - draw from bottom up
        Pen castle_pen = gfx->create_pen(80, 30, 80); // Very dark gray silhouette
        gfx->set_pen(castle_pen);
        
        // Castle base foundation
        for (int y = 28; y <= 31; y++) {
            for (int x = 8; x <= 24; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Main castle wall
        for (int y = 18; y <= 27; y++) {
            for (int x = 10; x <= 22; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Left tower
        for (int y = 12; y <= 27; y++) {
            for (int x = 8; x <= 12; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Right tower
        for (int y = 12; y <= 27; y++) {
            for (int x = 20; x <= 24; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Central tower (tallest)
        for (int y = 8; y <= 17; y++) {
            for (int x = 14; x <= 18; x++) {
                gfx->pixel({x, y});
            }
        }
        
        // Tower battlements (crenellations)
        // Left tower battlements
        for (int x = 8; x <= 12; x += 2) {
            gfx->pixel({x, 11});
            gfx->pixel({x, 10});
        }
        
        // Right tower battlements
        for (int x = 20; x <= 24; x += 2) {
            gfx->pixel({x, 11});
            gfx->pixel({x, 10});
        }
        
        // Central tower battlements
        for (int x = 14; x <= 18; x += 2) {
            gfx->pixel({x, 7});
            gfx->pixel({x, 6});
        }
        
        // Main wall battlements
        for (int x = 12; x <= 20; x += 3) {
            gfx->pixel({x, 17});
            gfx->pixel({x, 16});
        }
        
        // Castle gate (arched entrance)
        Pen gate_pen = gfx->create_pen(5, 5, 5); // Even darker for entrance
        gfx->set_pen(gate_pen);
        
        // Gate entrance
        for (int y = 24; y <= 27; y++) {
            for (int x = 15; x <= 17; x++) {
                gfx->pixel({x, y});
            }
        }
        // Arch top
        gfx->pixel({15, 23});
        gfx->pixel({17, 23});
        gfx->pixel({16, 22});
        
        // Glowing windows with animation
        float window_glow = 0.7f + 0.3f * sin(castle_window_phase * 1.2f);
        uint8_t window_brightness = (uint8_t)(window_glow * 255);
        uint8_t window_yellow = (uint8_t)(window_glow * 200);
        Pen window_pen = gfx->create_pen(window_brightness, window_yellow, 0);
        gfx->set_pen(window_pen);
        
        // Left tower windows
        gfx->pixel({10, 15});
        gfx->pixel({10, 20});
        gfx->pixel({10, 24});
        
        // Right tower windows
        gfx->pixel({22, 15});
        gfx->pixel({22, 20});
        gfx->pixel({22, 24});
        
        // Central tower windows
        gfx->pixel({16, 10});
        gfx->pixel({16, 13});
        
        // Main wall windows
        gfx->pixel({12, 21});
        gfx->pixel({20, 21});
        gfx->pixel({14, 24});
        gfx->pixel({18, 24});
        
        // Add window glow effect for more dramatic lighting
        if (window_glow > 0.8f) {
            Pen glow_pen = gfx->create_pen(window_brightness/2, window_yellow/2, 0);
            gfx->set_pen(glow_pen);
            
            // Glow around some windows
            gfx->pixel({9, 15});
            gfx->pixel({11, 15});
            gfx->pixel({10, 14});
            gfx->pixel({10, 16});
            
            gfx->pixel({21, 20});
            gfx->pixel({23, 20});
            gfx->pixel({22, 19});
            gfx->pixel({22, 21});
            
            gfx->pixel({15, 10});
            gfx->pixel({17, 10});
            gfx->pixel({16, 9});
            gfx->pixel({16, 11});
        }
        
        // Add some atmospheric fog at the base
        Pen fog_pen = gfx->create_pen(30, 25, 35);
        gfx->set_pen(fog_pen);
        for (int i = 0; i < 8; i++) {
            float fog_x = 6 + i * 2.5f + sin(castle_window_phase * 0.5f + i * 0.8f) * 1.5f;
            float fog_y = 29 + sin(castle_window_phase * 0.3f + i) * 0.5f;
            if (fog_x >= 0 && fog_x < 32 && fog_y >= 0 && fog_y < 32) {
                gfx->pixel({(int)fog_x, (int)fog_y});
            }
        }
    }

public:
    const char* getName() const override {
        return "SPOOK";
    }
    
    const char* getDescription() const override {
        return "Halloween spookiness";
    }
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        animated_eyes.init(graphics);
        skull_eyes.init(graphics);
        tree_eyes.init(graphics);
        
        current_scene = CREEPY_EYES;
        scene_start_time = to_ms_since_boot(get_absolute_time());
        scene_duration = 8000; // 4 seconds per scene
        animation_timer = 0;
        
        // Initialize animation states and generate random eyes
        generateRandomEyes();
        setupSkullEyes();
        setupTreeEyes();
        pumpkin_glow_phase = 0;
        witch_sparkle_phase = 0;
        background_phase = 0;
        
        // Initialize new scene animations
        candle_flicker_phase = 0;
        tree_sway_phase = 0;
        tree_glow_phase = 0;
        skull_glow_phase = 0;
        castle_window_phase = 0;
        wolf_howl_phase = 0;
        moon_glow_phase = 0;
        mountain_wind_phase = 0;
        witch_flight_phase = 0;
        
        // Initialize flame heat map
        for (int i = 0; i < 32 * 35; i++) {
            flame_heat[i] = 0.0f;
            flame_face_heat[i] = 0.0f;
        }
        
        // Initialize flame face animation
        face_eye_blink_timer = 0;
        face_left_eye_open = true;
        face_right_eye_open = true;
        face_mouth_anim_phase = 0;
        
        // Initialize ghosts
        ghosts.clear();
        for (int i = 0; i < 3; i++) {
            Ghost ghost;
            ghost.x = rand() % 32;
            ghost.y = rand() % 32;
            ghost.speed_x = (rand() % 100 - 50) * 0.01f;
            ghost.speed_y = (rand() % 100 - 50) * 0.01f;
            ghost.phase = rand() % 100 * 0.1f;
            ghost.opacity = 0.3f + (rand() % 50) * 0.01f;
            ghosts.push_back(ghost);
        }
        
        // Initialize bats
        bat_positions.clear();
        bat_speeds.clear();
        for (int i = 0; i < 4; i++) {
            bat_positions.push_back(-10 - i * 15);
            bat_speeds.push_back(0.3f + (i * 0.1f));
        }
    }
    
    bool update() override {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        animation_timer = current_time;
        
        // Update background animation
        background_phase += 0.02f;
        
        // Always update bat positions (they're now part of the background)
        for (size_t i = 0; i < bat_positions.size(); i++) {
            bat_positions[i] += bat_speeds[i];
            if (bat_positions[i] > 37) {
                bat_positions[i] = -10;
            }
        }
        
        // Check for scene transition (bats get longer duration)
        uint32_t current_scene_duration = (current_scene == FLYING_BATS) ? scene_duration * 2 : scene_duration;
        if (current_time - scene_start_time > current_scene_duration) {
            current_scene = (HalloweenScene)((current_scene + 1) % SCENE_COUNT);
            scene_start_time = current_time;
            
            // Reset scene-specific states when entering scenes
            if (current_scene == FLYING_BATS) {
                for (size_t i = 0; i < bat_positions.size(); i++) {
                    bat_positions[i] = -10 - i * 15;
                }
            } else if (current_scene == CREEPY_EYES) {
                // Generate new random eye configuration
                generateRandomEyes();
            } else if (current_scene == SKULL_CROSSBONES) {
                // Setup skull eyes
                setupSkullEyes();
            } else if (current_scene == HAUNTED_TREE) {
                // Setup tree eyes
                setupTreeEyes();
            }
        }
        
        // Update scene-specific animations
        switch (current_scene) {
            case CREEPY_EYES:
                // Eye animation is now handled by the AnimatedEye class
                break;
                
            case PUMPKIN:
                pumpkin_glow_phase += 0.05f;
                break;
                
            case FLYING_BATS:
                // Bats are now always moving as part of background
                // Also animate moon glow for the moon display in this scene
                moon_glow_phase += 0.02f;
                break;
                
            case WITCH_HAT:
                witch_sparkle_phase += 0.03f;
                break;
                
            case CANDLE_FLAME:
                candle_flicker_phase += 0.08f;
                break;
                
            case FLAME_FACE:
                candle_flicker_phase += 0.08f; // Use same flicker for consistency
                face_mouth_anim_phase += 0.04f;
                
                // Animate face eye blinking
                face_eye_blink_timer += 50; // Rough ms increment
                if (face_eye_blink_timer > 3000) { // Blink every 3 seconds
                    face_left_eye_open = !face_left_eye_open;
                    face_eye_blink_timer = 0;
                }
                if (((int)face_eye_blink_timer + 500) % 2500 == 0) { // Right eye blinks independently
                    face_right_eye_open = !face_right_eye_open;
                }
                break;
                
            case GHOSTLY_SPIRITS:
                // Update ghosts
                for (auto& ghost : ghosts) {
                    ghost.x += ghost.speed_x;
                    ghost.y += ghost.speed_y;
                    ghost.phase += 0.05f;
                    
                    // Wrap around screen
                    if (ghost.x < -5) ghost.x = 37;
                    if (ghost.x > 37) ghost.x = -5;
                    if (ghost.y < -5) ghost.y = 37;
                    if (ghost.y > 37) ghost.y = -5;
                    
                    // Vary opacity
                    ghost.opacity = 0.3f + 0.4f * sin(ghost.phase);
                }
                break;
                
            case HAUNTED_TREE:
                tree_sway_phase += 0.02f;
                tree_glow_phase += 0.04f;
                break;
                
            case SKULL_CROSSBONES:
                skull_glow_phase += 0.03f;
                break;
                
            case CASTLE:
                castle_window_phase += 0.04f;
                break;
                
            case WOLF_HOWLING:
                wolf_howl_phase += 0.03f;
                moon_glow_phase += 0.02f;
                mountain_wind_phase += 0.015f;
                witch_flight_phase += 0.08f;
                break;
                
            case SCENE_COUNT:
                break;
        }
        
        return true;
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        gfx->set_pen(gfx->create_pen(0, 0, 0));
        gfx->clear();
        
        switch (current_scene) {
            case CREEPY_EYES:
                drawCreepyEyes();
                break;
            case PUMPKIN:
                drawPumpkin();
                break;
            case FLYING_BATS:
                drawFlyingBats();
                break;
            case WITCH_HAT:
                drawWitchHat();
                break;
            case CANDLE_FLAME:
                drawCandleFlame();
                break;
            case FLAME_FACE:
                drawFlameFace();
                break;
            case GHOSTLY_SPIRITS:
                drawGhostlySpirits();
                break;
            case HAUNTED_TREE:
                drawHauntedTree();
                break;
            case SKULL_CROSSBONES:
                drawSkullCrossbones();
                break;
            case CASTLE:
                drawCastle();
                break;
            case WOLF_HOWLING:
                drawWolfHowling();
                break;
            case SCENE_COUNT:
                break;
        }
    }
    
    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down, 
                    bool button_bright_up, bool button_bright_down) override {
        // Allow manual scene switching with A button
        static bool a_pressed = false;
        if (button_a && !a_pressed) {
            current_scene = (HalloweenScene)((current_scene + 1) % SCENE_COUNT);
            scene_start_time = to_ms_since_boot(get_absolute_time());
            a_pressed = true;
            
            // Reset scene-specific states when manually entering scenes
            if (current_scene == FLYING_BATS) {
                for (size_t i = 0; i < bat_positions.size(); i++) {
                    bat_positions[i] = -10 - i * 15;
                }
            } else if (current_scene == CREEPY_EYES) {
                // Generate new random eye configuration
                generateRandomEyes();
            } else if (current_scene == SKULL_CROSSBONES) {
                // Setup skull eyes
                setupSkullEyes();
            } else if (current_scene == HAUNTED_TREE) {
                // Setup tree eyes
                setupTreeEyes();
            } else if (current_scene == CANDLE_FLAME) {
                // Reset flame heat map
                for (int i = 0; i < 32 * 35; i++) {
                    flame_heat[i] = 0.0f;
                }
                candle_flicker_phase = 0;
            } else if (current_scene == FLAME_FACE) {
                // Reset flame face heat map and animations
                for (int i = 0; i < 32 * 35; i++) {
                    flame_face_heat[i] = 0.0f;
                }
                candle_flicker_phase = 0;
                face_eye_blink_timer = 0;
                face_left_eye_open = true;
                face_right_eye_open = true;
                face_mouth_anim_phase = 0;
            } else if (current_scene == GHOSTLY_SPIRITS) {
                // Reset ghosts
                for (auto& ghost : ghosts) {
                    ghost.x = rand() % 32;
                    ghost.y = rand() % 32;
                    ghost.phase = rand() % 100 * 0.1f;
                }
            }
        } else if (!button_a) {
            a_pressed = false;
        }
        
        // Check for exit
        if (checkExitCondition(button_d)) {
            // Game will exit
        }
    }
};
