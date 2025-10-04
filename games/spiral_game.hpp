#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

class SpiralGame : public GameBase {
private:
    // Constants
    static const int DISPLAY_WIDTH = 32;
    static const int DISPLAY_HEIGHT = 32;
    static const int MAX_MULTIPLY = 300;

    // Game state
    float time_counter = 0.0f;
    float dimensions = 6.0f;  // Number of divisions around the circle
    int base = 3;             // Multiplication table base (the 'multiple')
    float zoom = 0.15f;       // Radius increment per number
    bool show_lines = true;   // Start with lines connected
    bool show_trails = false;
    bool show_lengths = false; // Color lines by their length
    bool dim_dir = true;      // Direction of dimension animation
    bool zoom_dir = true;     // Direction of zoom animation
    uint32_t last_zoom_change = 0;
    uint32_t zoom_change_interval = 47000; // Change zoom direction every 47 seconds
    uint32_t last_base_change = 0;
    uint32_t base_change_interval = 30000; // Change base every 30 seconds (configurable)
    uint32_t last_lines_toggle = 0;
    uint32_t lines_toggle_interval = 53000; // Toggle lines every 53 seconds (rarely off)
    uint32_t last_trails_toggle = 0;
    uint32_t trails_toggle_interval = 61000; // Toggle trails every 61 seconds (rarely on)
    uint32_t last_lengths_toggle = 0;
    uint32_t lengths_toggle_interval = 13000; // Toggle length coloring every 13 seconds (frequently on)

    // Color palette
    struct Color {
        uint8_t r, g, b;
    };
    static const int NUM_COLORS = 20;
    Color colors[NUM_COLORS];
    int current_color_idx = 0;

    // Debounce
    uint32_t last_button_time = 0;
    const uint32_t DEBOUNCE_DURATION = 200;

    // HSV to RGB conversion for beautiful colors
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

    // Initialize beautiful, vibrant colors using HSV
    void init_colors() {
        for (int i = 0; i < NUM_COLORS; i++) {
            float hue = (float)i / NUM_COLORS;  // Evenly spaced hues around color wheel
            float saturation = 0.9f + (rand() % 10) / 100.0f;  // High saturation (90-100%)
            float value = 0.9f + (rand() % 10) / 100.0f;  // High brightness (90-100%)
            hsv_to_rgb(hue, saturation, value, colors[i].r, colors[i].g, colors[i].b);
        }
    }

    // Check if number is prime
    bool is_prime(int num) {
        if (num <= 1) return false;
        for (int i = 2; i < num && i < 100; i++) {
            if (num % i == 0) return false;
        }
        return true;
    }

    // Random number in range
    int random_range(int from, int to) {
        return (rand() % (to - from)) + from;
    }

    // Draw the multiplication spiral
    void draw_spiral() {
        float radius = 0;
        float angle = -M_PI / 2.0f;  // Start at -90 degrees
        float center_x = DISPLAY_WIDTH / 2.0f;
        float center_y = DISPLAY_HEIGHT / 2.0f;

        Color& spiral_color = colors[current_color_idx];

        // Array to store points for line drawing with colors
        struct Point2D {
            int x, y;
            uint8_t r, g, b;
        };
        Point2D points[MAX_MULTIPLY];
        int point_count = 0;

        // Get complementary color for gradient
        int complement_idx = (current_color_idx + NUM_COLORS / 2) % NUM_COLORS;
        Color& complement_color = colors[complement_idx];

        for (int n = 0; n < MAX_MULTIPLY; n++) {
            angle += (2.0f * M_PI) / dimensions;
            float x = center_x + radius * cos(angle);
            float y = center_y + radius * sin(angle);

            // Check bounds
            int ix = (int)(x + 0.5f);
            int iy = (int)(y + 0.5f);

            if (ix >= 0 && ix < DISPLAY_WIDTH && iy >= 0 && iy < DISPLAY_HEIGHT) {
                // Check if this number is a multiple of the base
                if (n % base == 0 && n > 0) {
                    // Calculate gradient position along spiral
                    float gradient = (float)point_count / (MAX_MULTIPLY / base);
                    gradient = gradient > 1.0f ? 1.0f : gradient;

                    // Interpolate between spiral color and complement
                    uint8_t r = spiral_color.r * (1.0f - gradient) + complement_color.r * gradient;
                    uint8_t g = spiral_color.g * (1.0f - gradient) + complement_color.g * gradient;
                    uint8_t b = spiral_color.b * (1.0f - gradient) + complement_color.b * gradient;

                    // Add brightness variation based on distance from center
                    float brightness = 0.7f + 0.3f * sin(radius * 0.3f + time_counter);
                    r = r * brightness;
                    g = g * brightness;
                    b = b * brightness;

                    // Draw point for this number
                    gfx->set_pen(r, g, b);
                    gfx->pixel(Point(ix, iy));

                    // Store point for line drawing
                    if (show_lines && point_count < MAX_MULTIPLY) {
                        points[point_count].x = ix;
                        points[point_count].y = iy;
                        points[point_count].r = r;
                        points[point_count].g = g;
                        points[point_count].b = b;
                        point_count++;
                    }
                }
            }

            radius += zoom;
        }

        // Draw connecting lines with gradient colors or length-based colors
        if (show_lines && point_count > 1) {
            for (int i = 0; i < point_count - 1; i++) {
                // Calculate line length
                int dx = points[i + 1].x - points[i].x;
                int dy = points[i + 1].y - points[i].y;
                float length = sqrt(dx * dx + dy * dy);

                // Skip lines that are too long (likely wrapping around or going off-screen)
                if (length > 25.0f) {
                    continue;
                }

                uint8_t r = 0, g = 0, b = 0;

                if (show_lengths) {
                    // Map length to hue (0-40 pixels maps to full color wheel)
                    float max_length = 35.0f;
                    float hue = (length / max_length);
                    hue = hue > 1.0f ? 1.0f : hue;

                    // Use HSV to create rainbow based on length
                    hsv_to_rgb(hue, 1.0f, 1.0f, r, g, b);
                } else {
                    // Use dimmer version of point color for lines
                    r = points[i].r * 0.6f;
                    g = points[i].g * 0.6f;
                    b = points[i].b * 0.6f;
                }

                gfx->set_pen(r, g, b);

                // Simple line drawing using Bresenham-style algorithm
                int x0 = points[i].x;
                int y0 = points[i].y;
                int x1 = points[i + 1].x;
                int y1 = points[i + 1].y;

                int adx = abs(x1 - x0);
                int ady = abs(y1 - y0);
                int sx = x0 < x1 ? 1 : -1;
                int sy = y0 < y1 ? 1 : -1;
                int err = adx - ady;

                int steps = 0;
                while (steps++ < 100) {  // Limit steps to prevent infinite loops
                    if (x0 >= 0 && x0 < DISPLAY_WIDTH && y0 >= 0 && y0 < DISPLAY_HEIGHT) {
                        gfx->pixel(Point(x0, y0));
                    }

                    if (x0 == x1 && y0 == y1) break;

                    int e2 = 2 * err;
                    if (e2 > -ady) {
                        err -= ady;
                        x0 += sx;
                    }
                    if (e2 < adx) {
                        err += adx;
                        y0 += sy;
                    }
                }
            }
        }
    }

    // Animate dimensions with variable speed
    void animate_dimensions() {
        const float max_dimensions = 18.0f;
        const float min_dimensions = 3.0f;

        // Variable animation speed - slower most of the time, occasionally faster
        static float anim_speed = 0.01f;
        static uint32_t last_speed_change = 0;
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Change animation speed every 37 seconds
        if (current_time - last_speed_change > 37000) {
            // 70% chance of slow, 30% chance of faster
            int speed_choice = rand() % 10;
            if (speed_choice < 7) {
                anim_speed = 0.005f + (rand() % 10) * 0.0005f;  // 0.005 - 0.01 (very slow)
            } else {
                anim_speed = 0.02f + (rand() % 10) * 0.001f;    // 0.02 - 0.03 (moderate)
            }
            last_speed_change = current_time;
        }

        if (dim_dir) {
            dimensions += anim_speed;
            if (dimensions >= max_dimensions) {
                dim_dir = false;
                dimensions = max_dimensions;
            }
        } else {
            dimensions -= anim_speed;
            if (dimensions <= min_dimensions) {
                dim_dir = true;
                dimensions = min_dimensions;
                // Change base when we hit minimum
                base = random_range(2, 17);
                // Change color
                current_color_idx = random_range(0, NUM_COLORS);
            }
        }
    }

    // Animate zoom level
    void animate_zoom() {
        const float max_zoom = 0.55f;  // Zoomed out - see more of spiral
        const float min_zoom = 0.08f;  // Zoomed in - see details
        const float zoom_speed = 0.0003f;

        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Change zoom direction every 47 seconds
        if (current_time - last_zoom_change > zoom_change_interval) {
            zoom_dir = !zoom_dir;
            last_zoom_change = current_time;
        }

        if (zoom_dir) {
            zoom += zoom_speed;
            if (zoom >= max_zoom) {
                zoom = max_zoom;
                zoom_dir = false;
            }
        } else {
            zoom -= zoom_speed;
            if (zoom <= min_zoom) {
                zoom = min_zoom;
                zoom_dir = true;
            }
        }
    }

public:
    SpiralGame() {
        init_colors();
        current_color_idx = 0;
        base = 3;
    }

    const char* getName() const override {
        return "SPIRAL";
    }

    const char* getDescription() const override {
        return "Mathematical number spiral";
    }

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;

        // Set brightness to full for this demo
        cosmic->set_brightness(1.0f);

        gfx->set_pen(0, 0, 0);
        gfx->clear();
        time_counter = 0.0f;
        dimensions = 6.0f;
        dim_dir = true;
        show_lines = true;  // Always start with lines connected
        show_trails = false;
        show_lengths = false;
        zoom_dir = true;
        zoom = 0.15f;

        // Reset button debounce timer to prevent button A from menu selection toggling lines
        last_button_time = to_ms_since_boot(get_absolute_time());
        last_base_change = to_ms_since_boot(get_absolute_time());
        last_lines_toggle = to_ms_since_boot(get_absolute_time());
        last_trails_toggle = to_ms_since_boot(get_absolute_time());
        last_lengths_toggle = to_ms_since_boot(get_absolute_time());
        last_zoom_change = to_ms_since_boot(get_absolute_time());
    }

    bool update() override {
        uint32_t time_ms = to_ms_since_boot(get_absolute_time());
        time_counter += 0.016f;  // ~60fps

        // Animate dimensions
        animate_dimensions();

        // Animate zoom level
        animate_zoom();

        // Periodic base change
        if (time_ms - last_base_change > base_change_interval) {
            base = random_range(2, 12);
            current_color_idx = random_range(0, NUM_COLORS);
            last_base_change = time_ms;
        }

        // Periodic lines toggle
        if (time_ms - last_lines_toggle > lines_toggle_interval) {
            show_lines = !show_lines;
            last_lines_toggle = time_ms;
        }

        // Periodic trails toggle
        if (time_ms - last_trails_toggle > trails_toggle_interval) {
            show_trails = !show_trails;
            last_trails_toggle = time_ms;
        }

        // Periodic length coloring toggle
        if (time_ms - last_lengths_toggle > lengths_toggle_interval) {
            show_lengths = !show_lengths;
            last_lengths_toggle = time_ms;
        }

        return true;  // Keep running
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Clear or leave trails
        if (!show_trails) {
            graphics.set_pen(0, 0, 0);
            graphics.clear();
        } else {
            // Fade effect for trails
            for (int y = 0; y < DISPLAY_HEIGHT; y++) {
                for (int x = 0; x < DISPLAY_WIDTH; x++) {
                    // Simple fade - we can't read pixels easily, so we overlay a semi-transparent black
                    // This creates a trail effect
                }
            }
        }

        // Draw the spiral
        draw_spiral();
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down,
                    bool button_bright_up, bool button_bright_down) override {
        uint32_t time_ms = to_ms_since_boot(get_absolute_time());

        if (button_a && time_ms - last_button_time > DEBOUNCE_DURATION) {
            show_lines = !show_lines;
            last_button_time = time_ms;
        }

        if (button_b && time_ms - last_button_time > DEBOUNCE_DURATION) {
            show_trails = !show_trails;
            last_button_time = time_ms;
        }

        if (button_c && time_ms - last_button_time > DEBOUNCE_DURATION) {
            base = random_range(2, 12);
            current_color_idx = random_range(0, NUM_COLORS);
            last_button_time = time_ms;
        }

        if (button_d && time_ms - last_button_time > DEBOUNCE_DURATION) {
            // Adjust zoom
            zoom += 0.05f;
            if (zoom > 0.5f) zoom = 0.1f;
            last_button_time = time_ms;
        }

        // Volume up: increment the multiple (base)
        if (button_vol_up && time_ms - last_button_time > DEBOUNCE_DURATION) {
            base++;
            if (base > 36) base = 36;  // Cap at 36
            current_color_idx = random_range(0, NUM_COLORS);
            last_base_change = time_ms;  // Reset auto-change timer
            last_button_time = time_ms;
        }

        // Volume down: decrement the multiple (base)
        if (button_vol_down && time_ms - last_button_time > DEBOUNCE_DURATION) {
            base--;
            if (base < 1) base = 1;  // Minimum of 1
            current_color_idx = random_range(0, NUM_COLORS);
            last_base_change = time_ms;  // Reset auto-change timer
            last_button_time = time_ms;
        }
    }

    void cleanup() override {
        if (gfx) {
            gfx->set_pen(0, 0, 0);
            gfx->clear();
        }
    }
};
