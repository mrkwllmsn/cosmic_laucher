#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

class CirclesGame : public GameBase {
private:
    // Constants
    static const int DISPLAY_WIDTH = 32;
    static const int DISPLAY_HEIGHT = 32;
    static const int MAX_POINTS = 200;  // Reduced to show trail effect

    // Game state
    float angle = 0.0f;
    float time_counter = 0.0f;

    // Options - optimized for 32x32
    bool draw_circles = true;
    bool draw_lines = false;
    bool draw_points = true;
    bool draw_trails = false;
    float angle_speed = 0.05f;     // weirdness
    int angle_modifier = 100;      // strangeness (color)
    int min_circle_size = 1;       // wonkyness
    float scale = 8.0f;            // scale (2-8)
    float n = 12.0f;                // aloofness (pattern complexity) - higher = more detail

    // Point storage
    struct Point2D {
        float x, y;
    };
    std::vector<Point2D> points;

    // Auto-toggle timers
    uint32_t last_mode_change = 0;
    uint32_t mode_change_interval = 43000;  // 43 seconds
    uint32_t last_param_change = 0;
    uint32_t param_change_interval = 31000;  // 31 seconds

    // Debounce
    uint32_t last_button_time = 0;
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

    // Recursive circle drawing - finds the endpoint and optionally draws circles
    void find_circle_point(float x, float y, float r, float a, bool draw_this_circle = false) {
        if (r > min_circle_size) {
            float r1 = r * 0.5f;
            float rsum = r1 + r * (n / 10.0f) * 0.5f;
            float newx = x + rsum * cos(a);
            float newy = y - rsum * sin(a);

            find_circle_point(newx, newy, r * n / 10.0f, -a * n, draw_this_circle);
        } else {
            // We've reached the endpoint
            if (points.size() >= MAX_POINTS) {
                points.erase(points.begin());
            }
            points.push_back({x, y});
        }

        // Draw circles at each recursion level if enabled
        if (draw_circles && draw_this_circle) {
            int ix = (int)(x + 0.5f);
            int iy = (int)(y + 0.5f);
            int ir = (int)(r / 2.0f + 0.5f);

            // Draw circle outline
            for (int i = 0; i < 360; i += 15) {
                float rad = i * M_PI / 180.0f;
                int px = ix + ir * cos(rad);
                int py = iy + ir * sin(rad);

                if (px >= 0 && px < DISPLAY_WIDTH && py >= 0 && py < DISPLAY_HEIGHT) {
                    gfx->set_pen(255, 255, 255);
                    gfx->pixel(Point(px, py));
                }
            }
        }
    }

public:
    CirclesGame() {}

    const char* getName() const override {
        return "CIRCLES";
    }

    const char* getDescription() const override {
        return "Recursive circle patterns";
    }

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;

        cosmic->set_brightness(1.0f);

        gfx->set_pen(0, 0, 0);
        gfx->clear();

        angle = 0.0f;
        time_counter = 0.0f;
        points.clear();

        // Start optimized for 32x32 - circles and points only
        draw_circles = false;
        draw_lines = false;
        draw_points = true;
        draw_trails = false;
        angle_speed = 0.05f;
        angle_modifier = 100;
        min_circle_size = 1;
        scale = 3.0f;
        n = 6.5f;

        last_button_time = to_ms_since_boot(get_absolute_time());
        last_mode_change = to_ms_since_boot(get_absolute_time());
        last_param_change = to_ms_since_boot(get_absolute_time());
    }

    bool update() override {
        uint32_t time_ms = to_ms_since_boot(get_absolute_time());
        time_counter += 0.016f;

        // Auto-toggle display modes (only trails, circles and points always on)
        if (time_ms - last_mode_change > mode_change_interval) {
            draw_trails = !draw_trails;
            if (!draw_trails) {
                points.clear();
            }
            last_mode_change = time_ms;
        }

        // Auto-change parameters
        if (time_ms - last_param_change > param_change_interval) {
            int param = rand() % 4;
            switch (param) {
                case 0:
                    n = 5.0f + (rand() % 60) / 10.0f;  // 5.0 to 8.0
                    points.clear();  // Clear on param change
                    break;
                case 1:
                    angle_speed = 0.02f + (rand() % 80) / 1000.0f;  // 0.02 to 0.1
                    break;
                case 2:
                    angle_modifier = 50 + rand() % 200;  // 50 to 250
                    break;
                case 3:
                    scale = 2.0f + (rand() % 40) / 10.0f;  // 2.0 to 6.0
                    points.clear();  // Clear on param change
                    break;
            }
            last_param_change = time_ms;
        }

        // Generate points - run many iterations per frame like original
        for (int j = 0; j < 500; j++) {
            float center_x = DISPLAY_WIDTH / 2.0f;
            float center_y = DISPLAY_HEIGHT / 2.0f;
            float radius = DISPLAY_WIDTH * (scale / 10.0f);

            // Draw circles on last iteration only
            find_circle_point(center_x, center_y, radius, angle, (j == 499));
            angle += 1.0f * (angle_speed / 10000.0f);
        }

        return true;
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Clear screen completely each frame - trails are from limited point history
        if (!draw_trails) {
            graphics.set_pen(0, 0, 0);
            graphics.clear();
        }

        // Draw all the points with color based on PREVIOUS point's distance
        for (size_t i = 0; i < points.size(); i++) {
            Point2D& p = points[i];
            Point2D* prev = (i > 0) ? &points[i - 1] : nullptr;

            // Calculate color based on PREVIOUS point's distance from center (like original)
            float dx, dy;
            if (prev) {
                dx = prev->x - DISPLAY_WIDTH / 2.0f;
                dy = prev->y - DISPLAY_HEIGHT / 2.0f;
            } else {
                dx = p.x - DISPLAY_WIDTH / 2.0f;
                dy = p.y - DISPLAY_HEIGHT / 2.0f;
            }
            float distance = sqrt(dx * dx + dy * dy);
            // Max distance from center to corner is ~22.6 pixels on 32x32 display
            // Scale to use full color spectrum: normalize to 0-1, then multiply by modifier
            float normalized_distance = distance / 22.6f;  // 0.0 to 1.0
            float hue_val = fmod(normalized_distance * angle_modifier * 0.01f, 1.0f);

            uint8_t r = 0, g = 0, b = 0;
            hsv_to_rgb(hue_val, 1.0f, 1.0f, r, g, b);

            int ix = (int)(p.x + 0.5f);
            int iy = (int)(p.y + 0.5f);

            // Draw lines between consecutive points
            if (draw_lines && prev) {
                int prev_x = (int)(prev->x + 0.5f);
                int prev_y = (int)(prev->y + 0.5f);

                int adx = abs(ix - prev_x);
                int ady = abs(iy - prev_y);

                // Only draw if not too long
                if (adx < 20 && ady < 20) {
                    graphics.set_pen(r, g, b);

                    // Simple line
                    int steps = adx > ady ? adx : ady;
                    for (int step = 0; step <= steps; step++) {
                        int lx = prev_x + ((ix - prev_x) * step) / (steps > 0 ? steps : 1);
                        int ly = prev_y + ((iy - prev_y) * step) / (steps > 0 ? steps : 1);

                        if (lx >= 0 && lx < DISPLAY_WIDTH && ly >= 0 && ly < DISPLAY_HEIGHT) {
                            graphics.pixel(Point(lx, ly));
                        }
                    }
                }
            }

            // Draw points
            if (draw_points && ix >= 0 && ix < DISPLAY_WIDTH && iy >= 0 && iy < DISPLAY_HEIGHT) {
                graphics.set_pen(r, g, b);
                graphics.pixel(Point(ix, iy));
            }
        }
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down,
                    bool button_bright_up, bool button_bright_down) override {
        uint32_t time_ms = to_ms_since_boot(get_absolute_time());

        // A: Toggle circles
        if (button_a && time_ms - last_button_time > DEBOUNCE_DURATION) {
            draw_circles = !draw_circles;
            printf("CIRCLES: circles=%s\n", draw_circles ? "ON" : "OFF");
            last_button_time = time_ms;
        }

        // B: Toggle points
        if (button_b && time_ms - last_button_time > DEBOUNCE_DURATION) {
            draw_points = !draw_points;
            printf("CIRCLES: points=%s\n", draw_points ? "ON" : "OFF");
            last_button_time = time_ms;
        }

        // C: Toggle trails
        if (button_c && time_ms - last_button_time > DEBOUNCE_DURATION) {
            draw_trails = !draw_trails;
            printf("CIRCLES: trails=%s\n", draw_trails ? "ON" : "OFF");
            if (!draw_trails) {
                points.clear();
            }
            last_button_time = time_ms;
        }

        // D: Clear and randomize
        if (button_d && time_ms - last_button_time > DEBOUNCE_DURATION) {
            n = 2.0f + (rand() % 60) / 10.0f;
            scale = 2.0f + (rand() % 40) / 10.0f;
            angle_modifier = 50 + rand() % 200;
            printf("CIRCLES: RANDOMIZED - n=%.1f, scale=%.1f, angle_mod=%d\n", n, scale, angle_modifier);
            points.clear();
            last_button_time = time_ms;
        }

        // Volume up: Increase n (pattern complexity)
        if (button_vol_up && time_ms - last_button_time > DEBOUNCE_DURATION) {
            n += 0.5f;
            if (n > 8.0f) n = 8.0f;
            printf("CIRCLES: n=%.1f (aloofness UP)\n", n);
            points.clear();
            last_param_change = time_ms;
            last_button_time = time_ms;
        }

        // Volume down: Decrease n (pattern complexity)
        if (button_vol_down && time_ms - last_button_time > DEBOUNCE_DURATION) {
            n -= 0.5f;
            if (n < 1.0f) n = 1.0f;
            printf("CIRCLES: n=%.1f (aloofness DOWN)\n", n);
            points.clear();
            last_param_change = time_ms;
            last_button_time = time_ms;
        }
    }

    void cleanup() override {
        if (gfx) {
            gfx->set_pen(0, 0, 0);
            gfx->clear();
        }
        points.clear();
    }
};
