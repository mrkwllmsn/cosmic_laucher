#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

class ShaderEffectsGame : public GameBase {
private:
    // Constants
    static const int DISPLAY_WIDTH = 32;
    static const int DISPLAY_HEIGHT = 32;
    static const int NUM_EFFECTS = 8;
    
    // Game state
    float time_counter = 0.0f;
    int current_effect = 0;
    float animation_speed = 1.0f;
    uint32_t last_button_time = 0;
    
    // Static arrays for effects (shared across instances)
    static float matrix_drops[32];
    static bool matrix_initialized;
    // Starfield data: [angle, distance, brightness, speed] for each star layer
    static float star_field_slow[8][4];   // 8 slow stars
    static float star_field_medium[12][4]; // 12 medium stars  
    static float star_field_fast[16][4];   // 16 fast stars
    static bool stars_initialized;
    
    // Debounce duration
    const uint32_t DEBOUNCE_DURATION = 200;
    
    // HSV to RGB conversion
    void hsv_to_rgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
        int i = int(h * 6.0f);
        float f = h * 6.0f - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        switch (i % 6) {
            case 0: r = v * 255; g = t * 255; b = p * 255; break;
            case 1: r = q * 255; g = v * 255; b = p * 255; break;
            case 2: r = p * 255; g = v * 255; b = t * 255; break;
            case 3: r = p * 255; g = q * 255; b = v * 255; break;
            case 4: r = t * 255; g = p * 255; b = v * 255; break;
            case 5: r = v * 255; g = p * 255; b = q * 255; break;
        }
    }
    
    // Effect 1: Plasma Wave
    void plasma_effect() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                
                float v1 = sin(x * 0.2f + time_counter * animation_speed);
                float v2 = sin(y * 0.3f + time_counter * 0.8f * animation_speed);
                float v3 = sin((cx + cy) * 0.25f + time_counter * 1.2f * animation_speed);
                float v4 = sin(sqrt(cx * cx + cy * cy) * 0.3f + time_counter * 0.7f * animation_speed);
                
                float plasma = (v1 + v2 + v3 + v4) * 0.25f;
                
                uint8_t r = 0, g = 0, b = 0;
                hsv_to_rgb((plasma + 1.0f) * 0.5f, 1.0f, 1.0f, r, g, b);
                
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 2: Rainbow Spiral
    void rainbow_spiral() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                
                float angle = atan2(cy, cx);
                float distance = sqrt(cx * cx + cy * cy);
                
                float hue = (angle / (2 * M_PI) + distance * 0.1f - time_counter * 0.3f * animation_speed);
                hue = hue - floor(hue);
                
                float brightness = 0.5f + 0.5f * sin(distance * 0.3f - time_counter * 2.0f * animation_speed);
                
                uint8_t r = 0, g = 0, b = 0;
                hsv_to_rgb(hue, 1.0f, brightness, r, g, b);
                
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 3: Matrix Rain
    void matrix_rain() {
        if (!matrix_initialized) {
            for (int i = 0; i < DISPLAY_WIDTH; i++) {
                matrix_drops[i] = rand() % DISPLAY_HEIGHT;
            }
            matrix_initialized = true;
        }
        
        gfx->set_pen(0, 0, 0);
        gfx->clear();
        
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int drop_pos = (int)matrix_drops[x];
            
            for (int y = 0; y < DISPLAY_HEIGHT; y++) {
                if (y == drop_pos) {
                    gfx->set_pen(0, 255, 0);
                } else if (y > drop_pos - 8 && y < drop_pos) {
                    int fade = 255 - (drop_pos - y) * 32;
                    fade = fade < 0 ? 0 : fade;
                    gfx->set_pen(0, fade, 0);
                } else {
                    continue;
                }
                
                gfx->pixel(Point(x, y));
            }
            
            matrix_drops[x] += (0.3f + (rand() % 10) * 0.01f) * animation_speed;
            if (matrix_drops[x] > DISPLAY_HEIGHT + 8) {
                matrix_drops[x] = -8 - rand() % 10;
            }
        }
    }
    
    // Effect 4: Fire Ripples
    void fire_ripples() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                float distance = sqrt(cx * cx + cy * cy);
                
                float wave1 = sin(distance * 0.5f - time_counter * 3.0f * animation_speed);
                float wave2 = sin(distance * 0.3f - time_counter * 2.0f * animation_speed);
                float wave3 = sin(distance * 0.8f - time_counter * 1.5f * animation_speed);
                
                float intensity = (wave1 + wave2 + wave3) * 0.33f + 0.5f;
                intensity = intensity < 0 ? 0 : intensity;
                intensity = intensity > 1 ? 1 : intensity;
                
                uint8_t r = intensity * 255;
                uint8_t g = intensity * intensity * 180;
                uint8_t b = intensity * intensity * intensity * 100;
                
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 5: Vortex Math
    void vortex_math() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                
                float angle = atan2(cy, cx);
                float distance = sqrt(cx * cx + cy * cy);
                
                // Create vortex transformation
                float vortex_strength = 0.3f * animation_speed;
                float twisted_angle = angle + distance * vortex_strength * sin(time_counter * animation_speed);
                
                // Create mathematical shapes within the vortex
                float shape1 = sin(twisted_angle * 3.0f + time_counter * 2.0f * animation_speed);
                float shape2 = cos(twisted_angle * 5.0f - time_counter * 1.5f * animation_speed);
                float shape3 = sin(distance * 0.8f + twisted_angle * 2.0f + time_counter * animation_speed);
                
                // Combine shapes
                float intensity = (shape1 * shape2 + shape3) * 0.5f + 0.5f;
                intensity = pow(intensity, 2.0f); // Make it more dramatic
                
                // Create color based on position and intensity
                float hue = (twisted_angle / (2 * M_PI) + time_counter * 0.1f * animation_speed);
                hue = hue - floor(hue);
                
                uint8_t r = 0, g = 0, b = 0;
                hsv_to_rgb(hue, 0.8f + intensity * 0.2f, intensity, r, g, b);
                
                gfx->set_pen(r, g, b);
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 6: Organic Blobs
    void organic_blobs() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                
                // Create multiple blob centers that move
                float blob1_x = sin(time_counter * 0.7f * animation_speed) * 8.0f;
                float blob1_y = cos(time_counter * 0.5f * animation_speed) * 6.0f;
                float dist1 = sqrt((cx - blob1_x) * (cx - blob1_x) + (cy - blob1_y) * (cy - blob1_y));
                
                float blob2_x = cos(time_counter * 0.9f * animation_speed) * 6.0f;
                float blob2_y = sin(time_counter * 0.8f * animation_speed) * 8.0f;
                float dist2 = sqrt((cx - blob2_x) * (cx - blob2_x) + (cy - blob2_y) * (cy - blob2_y));
                
                float blob3_x = sin(time_counter * 1.2f * animation_speed) * 4.0f;
                float blob3_y = cos(time_counter * 1.1f * animation_speed) * 5.0f;
                float dist3 = sqrt((cx - blob3_x) * (cx - blob3_x) + (cy - blob3_y) * (cy - blob3_y));
                
                // Create organic blob shapes using metaballs
                float blob_size = 6.0f + 2.0f * sin(time_counter * 2.0f * animation_speed);
                float influence1 = blob_size / (dist1 + 1.0f);
                float influence2 = blob_size / (dist2 + 1.0f);
                float influence3 = blob_size / (dist3 + 1.0f);
                
                float total_influence = influence1 + influence2 + influence3;
                total_influence = total_influence > 2.0f ? 2.0f : total_influence;
                
                // Add some noise for organic feel
                float noise = sin(cx * 0.3f + time_counter * animation_speed) * 
                             cos(cy * 0.4f + time_counter * 1.2f * animation_speed) * 0.2f;
                total_influence += noise;
                
                if (total_influence > 0.8f) {
                    float hue = (time_counter * 0.1f * animation_speed + total_influence * 0.3f);
                    hue = hue - floor(hue);
                    
                    uint8_t r = 0, g = 0, b = 0;
                    hsv_to_rgb(hue, 0.9f, total_influence * 0.5f, r, g, b);
                    gfx->set_pen(r, g, b);
                } else {
                    gfx->set_pen(0, 0, 0);
                }
                
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 7: Pulsing Blobs
    void pulsing_blobs() {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float cx = x - DISPLAY_WIDTH / 2.0f;
                float cy = y - DISPLAY_HEIGHT / 2.0f;
                
                // Multiple pulsing blob centers
                float pulse1 = 1.0f + 0.5f * sin(time_counter * 3.0f * animation_speed);
                float pulse2 = 1.0f + 0.5f * cos(time_counter * 2.5f * animation_speed);
                float pulse3 = 1.0f + 0.3f * sin(time_counter * 4.0f * animation_speed);
                
                float blob1_dist = sqrt((cx - 8) * (cx - 8) + (cy - 6) * (cy - 6)) / pulse1;
                float blob2_dist = sqrt((cx + 8) * (cx + 8) + (cy - 6) * (cy - 6)) / pulse2;
                float blob3_dist = sqrt(cx * cx + (cy + 8) * (cy + 8)) / pulse3;
                
                float blob1 = exp(-blob1_dist * 0.3f);
                float blob2 = exp(-blob2_dist * 0.3f);
                float blob3 = exp(-blob3_dist * 0.3f);
                
                float intensity = blob1 + blob2 + blob3;
                intensity = intensity > 1.0f ? 1.0f : intensity;
                
                if (intensity > 0.1f) {
                    float hue = 0.7f + intensity * 0.3f + time_counter * 0.05f * animation_speed;
                    hue = hue - floor(hue);
                    
                    uint8_t r = 0, g = 0, b = 0;
                    hsv_to_rgb(hue, 1.0f, intensity, r, g, b);
                    
                    gfx->set_pen(r, g, b);
                } else {
                    gfx->set_pen(0, 0, 0);
                }
                
                gfx->pixel(Point(x, y));
            }
        }
    }
    
    // Effect 8: Star Field - Radial starfield flying through space
    void star_field() {
        const float CENTER_X = DISPLAY_WIDTH / 2.0f;
        const float CENTER_Y = DISPLAY_HEIGHT / 2.0f;
        const float MAX_DISTANCE = sqrt(CENTER_X * CENTER_X + CENTER_Y * CENTER_Y) + 5.0f;
        
        // Initialize starfield layers
        if (!stars_initialized) {
            // Initialize slow stars - spawn randomly across screen
            for (int i = 0; i < 8; i++) {
                star_field_slow[i][0] = (rand() % 628) / 100.0f; // angle
                star_field_slow[i][1] = (rand() % (int)(MAX_DISTANCE * 80)) / 100.0f; // distance - random across screen
                star_field_slow[i][2] = 0.4f + (rand() % 400) / 1000.0f; // brightness
                star_field_slow[i][3] = 0.1f + (rand() % 100) / 100.0f; // speed - much slower
            }
            
            // Initialize medium stars - spawn randomly across screen
            for (int i = 0; i < 12; i++) {
                star_field_medium[i][0] = (rand() % 628) / 100.0f;
                star_field_medium[i][1] = (rand() % (int)(MAX_DISTANCE * 80)) / 100.0f; // distance - random across screen
                star_field_medium[i][2] = 0.5f + (rand() % 400) / 1000.0f;
                star_field_medium[i][3] = 0.4f + (rand() % 200) / 100.0f;
            }
            
            // Initialize Slow Slow stars - spawn randomly across screen
            for (int i = 0; i < 16; i++) {
                star_field_fast[i][0] = (rand() % 628) / 100.0f;
                star_field_fast[i][1] = (rand() % (int)(MAX_DISTANCE * 80)) / 100.0f; // distance - random across screen
                star_field_fast[i][2] = 0.2f + (rand() % 400) / 1000.0f;
                star_field_fast[i][3] = 0.1f + (rand() % 100) / 100.0f;
            }
            
            stars_initialized = true;
        }
        
        // Clear screen with dark space background
        gfx->set_pen(0, 0, 8);
        gfx->clear();
         
        // Add rich nebula clouds
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                float noise1 = sin(x * 0.1f + time_counter * 0.3f * animation_speed) * 
                              cos(y * 0.15f + time_counter * 0.2f * animation_speed);
                float noise2 = sin(x * 0.08f - time_counter * 0.25f * animation_speed) * 
                              cos(y * 0.12f - time_counter * 0.15f * animation_speed);
                
                float nebula = (noise1 + noise2) * 0.3f + 0.3f;
                nebula = nebula < 0 ? 0 : nebula;
                nebula = nebula > 0.4f ? 0.4f : nebula;
                
                if (nebula > 0.2f) {
                    uint8_t intensity = nebula * 100;
                    gfx->set_pen(intensity, intensity / 2, intensity);
                    gfx->pixel(Point(x, y));
                }
            }
        }

        // Update and render slow stars (background layer)
        for (int i = 0; i < 8; i++) {
            // Move star outward from center
            star_field_slow[i][1] += star_field_slow[i][3] * animation_speed * 0.4f;
            
            // Reset star randomly across screen when it goes off screen
            if (star_field_slow[i][1] > MAX_DISTANCE) {
                star_field_slow[i][0] = (rand() % 628) / 100.0f;
                star_field_slow[i][1] = (rand() % 200) / 100.0f; // Start closer to center for new stars
                star_field_slow[i][2] = 0.4f + (rand() % 400) / 1000.0f;
                star_field_slow[i][3] = 0.1f + (rand() % 100) / 100.0f;
            }
            
            // Convert polar to cartesian and draw
            float angle = star_field_slow[i][0];
            float distance = star_field_slow[i][1];
            float sx = CENTER_X + cos(angle) * distance;
            float sy = CENTER_Y + sin(angle) * distance;
            
            int x = (int)sx;
            int y = (int)sy;
            if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
                float brightness = star_field_slow[i][2] * (0.6f + 0.4f * sin(time_counter * 2.0f + i * 0.3f));
                gfx->set_pen(120 * brightness, 120 * brightness, 140 * brightness);
                gfx->pixel(Point(x, y));
            }
        }

        // Update and render medium stars (middle layer)  
        for (int i = 0; i < 12; i++) {
            // Move star outward from center
            star_field_medium[i][1] += star_field_medium[i][3] * animation_speed * 0.7f;
            
            // Reset star randomly across screen when it goes off screen
            if (star_field_medium[i][1] > MAX_DISTANCE) {
                star_field_medium[i][0] = (rand() % 628) / 100.0f;
                star_field_medium[i][1] = (rand() % 200) / 100.0f; // Start closer to center for new stars
                star_field_medium[i][2] = 0.5f + (rand() % 400) / 1000.0f;
                star_field_medium[i][3] = 0.4f + (rand() % 200) / 100.0f;
            }
            
            // Convert polar to cartesian and draw
            float angle = star_field_medium[i][0];
            float distance = star_field_medium[i][1];
            float sx = CENTER_X + cos(angle) * distance;
            float sy = CENTER_Y + sin(angle) * distance;
            
            int x = (int)sx;
            int y = (int)sy;
            if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
                float twinkle = 0.7f + 0.3f * sin(time_counter * 3.0f + i * 0.5f);
                float brightness = star_field_medium[i][2] * twinkle;
                
                // Color variation for medium stars
                if (i % 5 == 0) {
                    gfx->set_pen(255 * brightness, 220 * brightness, 180 * brightness); // Yellow
                } else {
                    gfx->set_pen(200 * brightness, 200 * brightness, 220 * brightness); // White
                }
                gfx->pixel(Point(x, y));
                
                // Add small cross pattern for brighter medium stars
                if (brightness > 0.7f && x > 0 && x < DISPLAY_WIDTH-1 && y > 0 && y < DISPLAY_HEIGHT-1) {
                    gfx->set_pen(brightness * 80, brightness * 80, brightness * 100);
                    gfx->pixel(Point(x-1, y));
                    gfx->pixel(Point(x+1, y));
                    gfx->pixel(Point(x, y-1));
                    gfx->pixel(Point(x, y+1));
                }
            }
        }

        // Update and render fast stars (foreground layer)
        for (int i = 0; i < 16; i++) {
            // Move star outward from center at high speed
            star_field_fast[i][1] += star_field_fast[i][3] * animation_speed * 1.4f;
            
            // Reset star randomly across screen when it goes off screen
            if (star_field_fast[i][1] > MAX_DISTANCE) {
                star_field_fast[i][0] = (rand() % 628) / 100.0f;
                star_field_fast[i][1] = (rand() % 200) / 100.0f; // Start closer to center for new stars
                star_field_fast[i][2] = 0.6f + (rand() % 400) / 1000.0f;
                star_field_fast[i][3] = 1.0f + (rand() % 400) / 100.0f;
            }
            
            // Convert polar to cartesian and draw
            float angle = star_field_fast[i][0];
            float distance = star_field_fast[i][1];
            float sx = CENTER_X + cos(angle) * distance;
            float sy = CENTER_Y + sin(angle) * distance;
            
            int x = (int)sx;
            int y = (int)sy;
            if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
                float twinkle = 0.8f + 0.2f * sin(time_counter * 5.0f + i * 0.3f);
                float brightness = star_field_fast[i][2] * twinkle;
                
                // Color variation for fast stars
                if (i % 7 == 0) {
                    // Yellow star
                    gfx->set_pen(255 * brightness, 255 * brightness, 180 * brightness);
                } else if (i % 11 == 0) {
                    // Blue star
                    gfx->set_pen(180 * brightness, 200 * brightness, 255 * brightness);
                } else if (i % 13 == 0) {
                    // Red star
                    gfx->set_pen(255 * brightness, 180 * brightness, 180 * brightness);
                } else {
                    // White star
                    gfx->set_pen(255 * brightness, 245 * brightness, 235 * brightness);
                }
                
                gfx->pixel(Point(x, y));
                
                // Add radial motion trail for fast bright stars
                if (brightness > 0.7f && distance > 3.0f) {
                    float speed = star_field_fast[i][3] * animation_speed;
                    for (int t = 1; t <= 2; t++) {
                        float trail_distance = distance - t * speed * 0.5f;
                        if (trail_distance > 0) {
                            float trail_sx = CENTER_X + cos(angle) * trail_distance;
                            float trail_sy = CENTER_Y + sin(angle) * trail_distance;
                            int trail_x = (int)trail_sx;
                            int trail_y = (int)trail_sy;
                            
                            if (trail_x >= 0 && trail_x < DISPLAY_WIDTH && 
                                trail_y >= 0 && trail_y < DISPLAY_HEIGHT) {
                                float trail_brightness = brightness * (1.0f - t * 0.4f);
                                gfx->set_pen(120 * trail_brightness, 140 * trail_brightness, 160 * trail_brightness);
                                gfx->pixel(Point(trail_x, trail_y));
                            }
                        }
                    }
                }
            }
        }
    }
    
    bool debounce(uint32_t duration = 200) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_button_time > duration) {
            last_button_time = now;
            return true;
        }
        return false;
    }

public:
    ShaderEffectsGame() : time_counter(0.0f), current_effect(0), animation_speed(1.0f), last_button_time(0) {}
    
    virtual ~ShaderEffectsGame() = default;
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        cosmic->set_brightness(0.6);
        
        // Reset static data
        matrix_initialized = false;
        stars_initialized = false;
        time_counter = 0.0f;
        current_effect = 0;
        animation_speed = 1.0f;
    }
    
    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down, 
                    bool button_bright_up, bool button_bright_down) override {
        
        // Handle brightness controls
        if (button_bright_up) {
            cosmic->adjust_brightness(+0.01);
        }
        if (button_bright_down) {
            cosmic->adjust_brightness(-0.01);
        }
        
        // Switch effects with A button
        if (button_a && debounce()) {
            current_effect = (current_effect + 1) % NUM_EFFECTS;
        }
        
        // Speed controls with B and C buttons
        if (button_b && debounce(100)) {
            animation_speed += 0.1f;
            if (animation_speed > 5.0f) animation_speed = 5.0f;
        }
        if (button_c && debounce(100)) {
            animation_speed -= 0.1f;
            if (animation_speed < 0.1f) animation_speed = 0.1f;
        }
    }
    
    bool update() override {
        // Check for exit condition using GameBase method
        bool button_d = cosmic->is_pressed(CosmicUnicorn::SWITCH_D);
        if (checkExitCondition(button_d)) {
            return false;  // Exit game
        }
        
        // Handle other input
        handleInput(
            cosmic->is_pressed(CosmicUnicorn::SWITCH_A),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_B),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_C),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_D),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_UP),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_DOWN),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_UP),
            cosmic->is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_DOWN)
        );
        
        // Update time counter
        time_counter += 0.05f;
        
        return true;  // Continue game
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        // Render current effect
        switch (current_effect) {
            case 0: plasma_effect(); break;
            case 1: rainbow_spiral(); break;
            case 2: matrix_rain(); break;
            case 3: fire_ripples(); break;
            case 4: vortex_math(); break;
            case 5: organic_blobs(); break;
            case 6: pulsing_blobs(); break;
            case 7: star_field(); break;
        }
    }
    
    const char* getName() const override {
        return "Shader Effects";
    }
    
    const char* getDescription() const override {
        return "Cycle through 8 visual effects with A button. B/C control speed.";
    }
};

// Static member definitions
float ShaderEffectsGame::matrix_drops[32];
bool ShaderEffectsGame::matrix_initialized = false;
float ShaderEffectsGame::star_field_slow[8][4];
float ShaderEffectsGame::star_field_medium[12][4];
float ShaderEffectsGame::star_field_fast[16][4];
bool ShaderEffectsGame::stars_initialized = false;
