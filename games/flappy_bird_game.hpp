#ifndef FLAPPY_BIRD_GAME_HPP
#define FLAPPY_BIRD_GAME_HPP

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include <cstdlib>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

class FlappyBirdGame : public GameBase {
private:
    // Bird properties
    float bird_y;
    float bird_velocity;
    static const int BIRD_X = 6;  // Bird horizontal position
    static const int BIRD_SIZE = 4;  // 4x4 bird sprite

    // Physics constants
    static constexpr float GRAVITY = 0.35f;
    static constexpr float FLAP_STRENGTH = -2.8f;
    static constexpr float MAX_FALL_SPEED = 4.0f;
    static constexpr float VELOCITY_DAMPING = 0.96f;

    // Pipe properties
    static const int PIPE_WIDTH = 3;
    static const int PIPE_GAP = 14;
    static const int PIPE_SPACING = 24;
    float pipe_offset;
    int pipe1_gap_y;
    int pipe2_gap_y;
    int pipe3_gap_y;

    // Game state
    int score;
    bool game_over;
    bool started;
    int frame_count;

    // Game over messages
    const char* game_over_messages[32] = {
        "YOU FLOPPED",
        "R.I.P. BIRD",
        "BETTER LUCK",
        "TRY AGAIN?",
        "YOU CRASHED!",
        "WINGS CLIPPED",
        "NICE WALL HUG",
        "10/10 LANDING!",
        "FLAP HARDER NEXT TIME",
        "SKILL ISSUE",
        "BIRD WENT BONK",
        "OBSTACLE WINS",
        "WHY BIRD WHY",
        "NOT LIKE THAT...",
        "EVEN MY GRANDMA FLAPS BETTER",
        "DID YOU EVEN TRY?",
        "TRAGIC.",
        "SO CLOSE... NOT REALLY.",
        "ARE YOU NEW HERE?",
        "GET GOOD",
        "OUCH.",
        "EGG-SPLOSION!",
        "BIRD DOWN!",
        "NO MORE FLAPS",
        "YOU'RE GROUNDED",
        "TOO LOW, LITTLE BIRD",
        "FLEW TOO CLOSE TO THE PIPE",
        "NICE TRY!",
        "YOU GOT THIS",
        "ALMOST THERE!",
        "PROGRESS!",
        "KEEP GOING!"
    };
    int selected_message_index;
    float scroll_offset;

    // Random color palette for game over shader
    uint8_t shader_hue_offset;

    // Animation
    float wing_angle;

    // Button state
    bool last_button_a;

    // Demo mode state
    bool demo_mode;
    uint32_t last_input_time;
    static constexpr uint32_t DEMO_TIMEOUT = 30000;  // 30 seconds

public:
    FlappyBirdGame() = default;

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        demo_mode = true;
        last_input_time = to_ms_since_boot(get_absolute_time());
        reset();
    }

    const char* getName() const override {
        return "FLAPPY";
    }

    const char* getDescription() const override {
        return "Flap through pipes";
    }

    void reset() {
        bird_y = 16.0f;
        bird_velocity = 0.0f;
        pipe_offset = 40.0f;  // Start first pipe well off screen to the right
        score = 0;
        game_over = false;
        started = true;  // Start immediately, no intro screen
        frame_count = 0;
        wing_angle = 0.0f;
        last_button_a = false;
        scroll_offset = -32.0f;  // Start scrolling from off-screen right

        // Randomize pipe gaps - keep them in safe range
        pipe1_gap_y = 10 + (rand() % 12);
        pipe2_gap_y = 10 + (rand() % 12);
        pipe3_gap_y = 10 + (rand() % 12);

        // Pick a random game over message for next game over
        selected_message_index = rand() % 32;

        // Pick a random color palette for shader
        shader_hue_offset = rand() % 256;
    }

    void drawBird(int x, int y) {
        // 4x4 detailed bird sprite with yellow body, orange beak, white eye

        // Bird rotation based on velocity
        float rotation = bird_velocity * 0.15f;
        rotation = std::max(-0.5f, std::min(0.5f, rotation));

        // Body color - bright yellow/orange
        uint8_t body_r = 255, body_g = 200, body_b = 0;
        Pen body_pen = gfx->create_pen(body_r, body_g, body_b);

        // Darker outline
        Pen outline_pen = gfx->create_pen(180, 120, 0);

        // Beak - orange
        Pen beak_pen = gfx->create_pen(255, 100, 0);

        // Eye - white
        Pen eye_pen = gfx->create_pen(255, 255, 255);

        // Pupil - black
        Pen pupil_pen = gfx->create_pen(0, 0, 0);

        // Draw bird sprite (4x4)
        // Row 0 (top)
        gfx->set_pen(outline_pen);
        gfx->pixel(Point(x + 1, y));
        gfx->pixel(Point(x + 2, y));

        // Row 1
        gfx->set_pen(outline_pen);
        gfx->pixel(Point(x, y + 1));
        gfx->set_pen(body_pen);
        gfx->pixel(Point(x + 1, y + 1));
        gfx->pixel(Point(x + 2, y + 1));
        gfx->set_pen(beak_pen);
        gfx->pixel(Point(x + 3, y + 1));  // Beak

        // Row 2
        gfx->set_pen(outline_pen);
        gfx->pixel(Point(x, y + 2));
        gfx->set_pen(eye_pen);
        gfx->pixel(Point(x + 1, y + 2));  // Eye
        gfx->set_pen(body_pen);
        gfx->pixel(Point(x + 2, y + 2));
        gfx->set_pen(outline_pen);
        gfx->pixel(Point(x + 3, y + 2));

        // Row 3 (bottom)
        gfx->set_pen(outline_pen);
        gfx->pixel(Point(x + 1, y + 3));
        gfx->pixel(Point(x + 2, y + 3));

        // Pupil dot
        gfx->set_pen(pupil_pen);
        gfx->pixel(Point(x + 1, y + 2));

        // Wing flap animation - small accent
        if (frame_count % 10 < 5) {
            gfx->set_pen(outline_pen);
            gfx->pixel(Point(x, y + 2));
        }
    }

    void drawPipe(int x, int gap_center_y) {
        // Dark background ensures visibility
        Pen pipe_body = gfx->create_pen(50, 200, 50);  // Green pipes
        Pen pipe_dark = gfx->create_pen(30, 120, 30);  // Darker green
        Pen pipe_cap = gfx->create_pen(70, 220, 70);   // Lighter green cap

        int gap_top = gap_center_y - PIPE_GAP / 2;
        int gap_bottom = gap_center_y + PIPE_GAP / 2;

        // Draw top pipe
        for (int px = 0; px < PIPE_WIDTH; px++) {
            for (int py = 0; py < gap_top - 1; py++) {
                if (px == 0 || px == PIPE_WIDTH - 1) {
                    gfx->set_pen(pipe_dark);  // Edges darker
                } else {
                    gfx->set_pen(pipe_body);
                }
                gfx->pixel(Point(x + px, py));
            }
            // Pipe cap (slightly wider looking)
            gfx->set_pen(pipe_cap);
            gfx->pixel(Point(x + px, gap_top - 1));
        }

        // Draw bottom pipe
        for (int px = 0; px < PIPE_WIDTH; px++) {
            // Pipe cap
            gfx->set_pen(pipe_cap);
            gfx->pixel(Point(x + px, gap_bottom));

            for (int py = gap_bottom + 1; py < 32; py++) {
                if (px == 0 || px == PIPE_WIDTH - 1) {
                    gfx->set_pen(pipe_dark);  // Edges darker
                } else {
                    gfx->set_pen(pipe_body);
                }
                gfx->pixel(Point(x + px, py));
            }
        }
    }

    bool checkCollision(int bird_x, int bird_y, int pipe_x, int gap_center_y) {
        int gap_top = gap_center_y - PIPE_GAP / 2;
        int gap_bottom = gap_center_y + PIPE_GAP / 2;

        // Check if bird overlaps with pipe horizontally
        if (bird_x + BIRD_SIZE > pipe_x && bird_x < pipe_x + PIPE_WIDTH) {
            // Check if bird is outside the gap vertically
            // Even if bird is off-screen at top, still check collision with top pipe
            // Bottom of bird must be above gap_top, or top of bird must be below gap_bottom
            if (bird_y + BIRD_SIZE < gap_top || bird_y > gap_bottom) {
                // Bird is in pipe region - collision only if actually hitting pipe
                if (bird_y + BIRD_SIZE < gap_top && bird_y + BIRD_SIZE >= 0) {
                    return true;  // Hit top pipe
                }
                if (bird_y > gap_bottom) {
                    return true;  // Hit bottom pipe
                }
                // If bird is completely off screen at top (bird_y + BIRD_SIZE < 0)
                // but trying to pass through where top pipe would be, that's a collision
                if (bird_y < 0 && gap_top > 0) {
                    return true;  // Can't bypass top pipe by going off screen
                }
            }
        }
        return false;
    }

    bool shouldAIFlap() {
        // Simple AI: aim to stay in the middle-ish height and adjust for pipes

        // Find the nearest upcoming pipe
        int pipe1_x = (int)(pipe_offset);
        int pipe2_x = (int)(pipe_offset + PIPE_SPACING);
        int pipe3_x = (int)(pipe_offset + PIPE_SPACING * 2);

        int target_gap_y = 16;  // Default to middle of screen

        // Look for the next pipe we need to navigate
        if (pipe1_x > -10 && pipe1_x < 32) {
            target_gap_y = pipe1_gap_y;
        } else if (pipe2_x > -10 && pipe2_x < 32) {
            target_gap_y = pipe2_gap_y;
        } else if (pipe3_x > -10 && pipe3_x < 32) {
            target_gap_y = pipe3_gap_y;
        }

        // Simple rule: flap if bird is below the target gap center
        // Account for bird size and give a small upward bias
        float target_height = target_gap_y - 2.0f;  // Aim slightly above gap center

        // Flap if:
        // 1. Bird is below target height
        // 2. Bird is falling (positive velocity) and getting close to target
        if (bird_y > target_height || (bird_velocity > 1.0f && bird_y > target_height - 3.0f)) {
            return true;
        }

        // Emergency flap if too close to floor
        if (bird_y > 22) {
            return true;
        }

        return false;
    }

    bool update() override {
        frame_count++;
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Check for flap input
        bool button_a = cosmic->is_pressed(CosmicUnicorn::SWITCH_A);

        // Check for any input to exit demo mode
        if (button_a) {
            if (demo_mode) {
                demo_mode = false;
                reset();  // Restart game when exiting demo
            }
            last_input_time = current_time;
        }

        // Check demo mode timeout - re-enable after 30 seconds of no input
        if (!demo_mode && (current_time - last_input_time > DEMO_TIMEOUT)) {
            demo_mode = true;
            reset();
        }

        if (!game_over) {
            // Game running

            // Demo AI controls
            if (demo_mode) {
                // AI decides to flap based on predictive logic
                bool ai_wants_flap = shouldAIFlap();
                if (ai_wants_flap && !last_button_a) {
                    bird_velocity = FLAP_STRENGTH;
                    last_button_a = true;
                } else if (!ai_wants_flap) {
                    last_button_a = false;  // Reset for next flap
                }
            } else {
                // Manual controls - flap on button press
                if (button_a && !last_button_a) {
                    bird_velocity = FLAP_STRENGTH;
                }
            }

            // Apply physics
            bird_velocity += GRAVITY;
            bird_velocity *= VELOCITY_DAMPING;  // Slight air resistance

            // Clamp fall speed
            if (bird_velocity > MAX_FALL_SPEED) {
                bird_velocity = MAX_FALL_SPEED;
            }

            // Update bird position
            bird_y += bird_velocity;

            // Check floor/ceiling collision
            // Allow bird to go 4 pixels off top of screen
            if (bird_y < -4) {
                bird_y = -4;
                bird_velocity = 0;
            }
            if (bird_y + BIRD_SIZE > 32) {
                bird_y = 32 - BIRD_SIZE;
                bird_velocity = 0;
                game_over = true;
            }

            // Move pipes
            pipe_offset -= 1.2f;

            // Calculate pipe positions
            int pipe1_x = (int)(pipe_offset);
            int pipe2_x = (int)(pipe_offset + PIPE_SPACING);
            int pipe3_x = (int)(pipe_offset + PIPE_SPACING * 2);

            // Wrap pipes and randomize gaps
            if (pipe_offset < -PIPE_WIDTH) {
                pipe_offset += PIPE_SPACING;
                pipe1_gap_y = pipe2_gap_y;
                pipe2_gap_y = pipe3_gap_y;
                pipe3_gap_y = 8 + (rand() % 12);
                score++;
            }

            // Check collisions
            if (checkCollision(BIRD_X, (int)bird_y, pipe1_x, pipe1_gap_y) ||
                checkCollision(BIRD_X, (int)bird_y, pipe2_x, pipe2_gap_y) ||
                checkCollision(BIRD_X, (int)bird_y, pipe3_x, pipe3_gap_y)) {
                game_over = true;
            }

        } else {
            // Game over
            // In demo mode, auto-restart after message scrolls through at least once
            static uint32_t game_over_time = 0;
            if (game_over && game_over_time == 0) {
                game_over_time = current_time;
            }

            // Calculate how long the message needs to fully scroll
            const char* message = game_over_messages[selected_message_index];
            int width = gfx->measure_text(message, 1);
            // Time for message to scroll completely = (width + 32) pixels / 1.0 pixel per frame * 50ms per frame -- twice the speed now so half the time at 25
            uint32_t scroll_duration = (width + 32) * 25;

            if (demo_mode && (current_time - game_over_time > scroll_duration)) {
                // Auto-restart in demo mode after message has scrolled completely
                game_over_time = 0;
                reset();
            } else if (!demo_mode && button_a && !last_button_a) {
                // Manual restart on button press
                game_over_time = 0;
                reset();
            }
        }

        last_button_a = button_a;
        return true;  // Continue running
    }

    void renderGameOverShader() {
        // Cool plasma/wave effect for game over screen with random colors
        float time = frame_count * 0.05f;

        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                // Create flowing plasma effect
                float dx = x - 16.0f;
                float dy = y - 16.0f;
                float dist = sqrtf(dx * dx + dy * dy);

                // Multiple sine waves create plasma effect
                float v1 = sinf(x * 0.2f + time);
                float v2 = sinf(y * 0.2f + time * 1.3f);
                float v3 = sinf((x + y) * 0.15f + time * 0.8f);
                float v4 = sinf(dist * 0.3f - time * 2.0f);

                float combined = (v1 + v2 + v3 + v4) * 0.25f;

                // Use random hue offset to create different color palettes
                // Convert to different color schemes based on shader_hue_offset
                uint8_t r, g, b;
                if (shader_hue_offset < 85) {
                    // Red/orange/yellow palette
                    r = (uint8_t)(128 + combined * 127);
                    g = (uint8_t)(32 + combined * 64);
                    b = (uint8_t)(8 + combined * 24);
                } else if (shader_hue_offset < 170) {
                    // Green/cyan palette
                    r = (uint8_t)(8 + combined * 24);
                    g = (uint8_t)(128 + combined * 127);
                    b = (uint8_t)(64 + combined * 96);
                } else {
                    // Blue/purple/pink palette
                    r = (uint8_t)(96 + combined * 96);
                    g = (uint8_t)(8 + combined * 24);
                    b = (uint8_t)(128 + combined * 127);
                }

                Pen pixel = gfx->create_pen(r, g, b);
                gfx->set_pen(pixel);
                gfx->pixel(Point(x, y));
            }
        }
    }

    void renderSubtleBackground() {
        // Subtle animated gradient background
        float time = frame_count * 0.02f;

        for (int y = 0; y < 32; y++) {
            // Create a slow vertical gradient that shifts over time
            float wave1 = sinf(y * 0.1f + time * 0.5f);
            float wave2 = sinf(y * 0.15f - time * 0.3f);

            // Very dark blue/purple gradient
            uint8_t r = (uint8_t)(8 + wave1 * 4 + wave2 * 3);
            uint8_t g = (uint8_t)(8 + wave1 * 5 + wave2 * 4);
            uint8_t b = (uint8_t)(25 + wave1 * 8 + wave2 * 6);

            Pen row_pen = gfx->create_pen(r, g, b);
            gfx->set_pen(row_pen);

            // Draw horizontal line
            for (int x = 0; x < 32; x++) {
                gfx->pixel(Point(x, y));
            }
        }
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        if (!game_over) {
            // Subtle animated background
            renderSubtleBackground();
            // Game running - draw pipes
            int pipe1_x = (int)(pipe_offset);
            int pipe2_x = (int)(pipe_offset + PIPE_SPACING);
            int pipe3_x = (int)(pipe_offset + PIPE_SPACING * 2);

            if (pipe1_x > -PIPE_WIDTH && pipe1_x < 32) {
                drawPipe(pipe1_x, pipe1_gap_y);
            }
            if (pipe2_x > -PIPE_WIDTH && pipe2_x < 32) {
                drawPipe(pipe2_x, pipe2_gap_y);
            }
            if (pipe3_x > -PIPE_WIDTH && pipe3_x < 32) {
                drawPipe(pipe3_x, pipe3_gap_y);
            }

            // Draw bird
            drawBird(BIRD_X, (int)bird_y);

            // Draw score
            Pen white = graphics.create_pen(255, 255, 255);
            graphics.set_pen(white);
            char score_text[16];
            snprintf(score_text, sizeof(score_text), "%d", score);
            graphics.text(score_text, Point(1, 1), -1, 1);

        } else {
            // Game over - render cool shader effect
            renderGameOverShader();

            // Update scroll position - faster scrolling
            scroll_offset += 2.0f;
            const char* message = game_over_messages[selected_message_index];
            int width = graphics.measure_text(message, 1);

            // Reset scroll when message has completely scrolled off left
            if (scroll_offset > width) {
                scroll_offset = -32.0f;
            }

            // Draw scrolling message with drop shadow
            Pen shadow = graphics.create_pen(0, 0, 0);
            Pen white = graphics.create_pen(255, 255, 255);

            // Shadow
            graphics.set_pen(shadow);
            graphics.text(message, Point((int)(1 - scroll_offset), 13), -1, 1);

            // Main text
            graphics.set_pen(white);
            graphics.text(message, Point((int)(0 - scroll_offset), 12), -1, 1);

            // Score at bottom - just the number
            char score_text[16];
            snprintf(score_text, sizeof(score_text), "%d", score);
            int score_width = graphics.measure_text(score_text, 1);

            // Shadow
            graphics.set_pen(shadow);
            graphics.text(score_text, Point((32 - score_width) / 2 + 1, 25), -1, 1);

            // Main text
            graphics.set_pen(white);
            graphics.text(score_text, Point((32 - score_width) / 2, 24), -1, 1);
        }
    }
};

#endif // FLAPPY_BIRD_GAME_HPP
