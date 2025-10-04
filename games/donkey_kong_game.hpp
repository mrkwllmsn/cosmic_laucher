#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <string>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

// Game constants
const int DISPLAY_WIDTH = 32;
const int DISPLAY_HEIGHT = 32;

// Platform structure
struct Platform {
    int y;
    int x_start;
    int x_end;
    bool has_ladder_left;
    bool has_ladder_right;
    int ladder_x;  // x position of ladder (if not at edges)

    Platform(int y_pos, int x1, int x2, bool ladder_l = false, bool ladder_r = false, int ladder_pos = -1)
        : y(y_pos), x_start(x1), x_end(x2), has_ladder_left(ladder_l), has_ladder_right(ladder_r), ladder_x(ladder_pos) {}

    bool contains(int x, int check_y) const {
        return check_y == y && x >= x_start && x <= x_end;
    }

    bool hasLadderAt(int x) const {
        if (has_ladder_left && x >= x_start && x <= x_start + 2) return true;
        if (has_ladder_right && x >= x_end - 2 && x <= x_end) return true;
        if (ladder_x >= 0 && x >= ladder_x - 1 && x <= ladder_x + 1) return true;
        return false;
    }
};

// Barrel object
struct Barrel {
    float x, y;
    float vx;
    bool active;
    int current_platform;
    bool falling;
    float fall_timer;

    Barrel() : x(0), y(0), vx(0), active(false), current_platform(0), falling(false), fall_timer(0) {}

    void spawn(float spawn_x, float spawn_y, float velocity, int platform_idx) {
        x = spawn_x;
        y = spawn_y;
        vx = velocity;
        active = true;
        current_platform = platform_idx;
        falling = false;
        fall_timer = 0;
    }

    void update(const std::vector<Platform>& platforms) {
        if (!active) return;

        // Validate platform index
        if (current_platform < 0 || current_platform >= (int)platforms.size()) {
            active = false;
            return;
        }

        const Platform& current = platforms[current_platform];

        // If falling between platforms
        if (falling) {
            y += 1.5f;  // Fall downward
            fall_timer += 1.0f;

            // Check if landed on next platform
            if (current_platform > 0) {
                const Platform& next_platform = platforms[current_platform - 1];
                if (y >= next_platform.y) {
                    // Landed on next platform
                    current_platform--;
                    y = platforms[current_platform].y;
                    falling = false;
                    fall_timer = 0;
                    // Always reverse direction after falling
                    vx = -vx;
                }
            } else if (y >= platforms[0].y) {
                // Reached bottom
                falling = false;
                fall_timer = 0;
                y = platforms[0].y;
                // Reverse direction on bottom too
                vx = -vx;
            }

            // Continue moving horizontally while falling
            x += vx * 0.5f;  // Slower horizontal movement while falling
        } else {
            // Normal rolling on platform
            x += vx;

            // Keep barrel at platform height
            y = current.y;

            // Check if barrel should start falling down ladder OR went off platform edge
            if (current_platform > 0) {
                bool should_fall = false;

                // Check if barrel reached any ladder position on the platform
                if (current.hasLadderAt((int)x)) {
                    // 30% chance to fall at any ladder
                    if (rand() % 100 < 30) {
                        should_fall = true;
                    }
                }

                // Check if barrel went off platform edges (more lenient)
                if (vx > 0 && x >= current.x_end) {
                    should_fall = true;
                } else if (vx < 0 && x <= current.x_start) {
                    should_fall = true;
                }

                // Also check if completely off screen
                if (x < -2 || x > DISPLAY_WIDTH + 2) {
                    should_fall = true;
                }

                if (should_fall) {
                    // Start falling
                    falling = true;
                    fall_timer = 0;
                }
            } else {
                // On bottom platform, deactivate when far off screen
                if (x < -3 || x > DISPLAY_WIDTH + 3) {
                    active = false;
                }
            }
        }
    }
};

// Player structure
struct KongPlayer {
    int x, y;
    float vy;
    bool jumping;
    bool on_ladder;
    bool alive;
    int current_platform;
    uint32_t death_animation_start;
    bool death_animation_playing;
    uint32_t jump_start_time;

    KongPlayer() : x(2), y(28), vy(0), jumping(false), on_ladder(false),
               alive(true), current_platform(0), death_animation_start(0),
               death_animation_playing(false), jump_start_time(0) {}

    void reset() {
        x = 2;
        y = 28;
        vy = 0;
        jumping = false;
        on_ladder = false;
        alive = true;
        current_platform = 0;
        death_animation_playing = false;
        jump_start_time = 0;
    }

    void startDeathAnimation() {
        death_animation_start = to_ms_since_boot(get_absolute_time());
        death_animation_playing = true;
        alive = false;
    }

    bool isDeathAnimationComplete() {
        if (!death_animation_playing) return true;
        uint32_t now = to_ms_since_boot(get_absolute_time());
        return (now - death_animation_start) >= 1000; // 1 second animation
    }

    void startJump() {
        if (!jumping && !on_ladder) {
            jumping = true;
            vy = -1.2f; // Initial upward velocity (reduced for lower jump)
            jump_start_time = to_ms_since_boot(get_absolute_time());
        }
    }

    void updatePhysics(const std::vector<Platform>& platforms) {
        if (jumping) {
            // Apply vertical movement
            y += (int)vy;
            vy += 0.3f; // Gravity

            // Check if landed on current platform
            const Platform& p = platforms[current_platform];
            if (y >= p.y - 2) {
                y = p.y - 2; // Stand on platform (2 pixels high)
                jumping = false;
                vy = 0;
            }
        }
    }
};

class DonkeyKongGame : public GameBase {
private:
    // Game state
    uint32_t frame_count;
    uint32_t last_action;
    uint32_t last_barrel_spawn;
    int score;
    int lives;
    int level;

    // Game objects
    std::vector<Platform> platforms;
    KongPlayer player;
    std::vector<Barrel> barrels;

    // Princess position
    int princess_x, princess_y;

    // Donkey Kong position
    int dk_x, dk_y;

    // Pen colors
    std::vector<Pen> pens;

    // Debounce
    const uint32_t DEBOUNCE_DURATION = 150;
    const uint32_t BARREL_SPAWN_INTERVAL = 2500; // 2.5 seconds

public:
    DonkeyKongGame() : frame_count(0), last_action(0), last_barrel_spawn(0),
                       score(0), lives(3), level(1),
                       princess_x(26), princess_y(3), dk_x(2), dk_y(3) {
        barrels.resize(10); // Pool of barrels
    }

    virtual ~DonkeyKongGame() = default;

    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;

        cosmic->set_brightness(0.6);

        initPens();
        setupLevel();
        player.reset();

        // Reset barrels
        for (auto& barrel : barrels) {
            barrel.active = false;
        }

        last_barrel_spawn = to_ms_since_boot(get_absolute_time());
    }

    void initPens() {
        pens.clear();
        pens.reserve(15);

        pens.push_back(gfx->create_pen(0, 0, 0));       // 0: black
        pens.push_back(gfx->create_pen(0, 255, 0));     // 1: green
        pens.push_back(gfx->create_pen(255, 0, 0));     // 2: red
        pens.push_back(gfx->create_pen(0, 0, 255));     // 3: blue
        pens.push_back(gfx->create_pen(255, 255, 0));   // 4: yellow
        pens.push_back(gfx->create_pen(255, 255, 255)); // 5: white
        pens.push_back(gfx->create_pen(139, 69, 19));   // 6: brown
        pens.push_back(gfx->create_pen(0, 255, 255));   // 7: cyan
        pens.push_back(gfx->create_pen(255, 165, 0));   // 8: orange
        pens.push_back(gfx->create_pen(255, 192, 203)); // 9: pink
        pens.push_back(gfx->create_pen(128, 128, 128)); // 10: gray
        pens.push_back(gfx->create_pen(200, 100, 50));  // 11: light brown
        pens.push_back(gfx->create_pen(150, 75, 0));    // 12: dark brown
    }

    void setupLevel() {
        platforms.clear();

        // Helper function to generate random ladder position within platform bounds
        auto randomLadderX = [](int x_start, int x_end) -> int {
            // Keep ladder at least 3 pixels from edges for visibility
            int min_x = x_start + 3;
            int max_x = x_end - 3;
            if (max_x <= min_x) return (x_start + x_end) / 2;
            return min_x + (rand() % (max_x - min_x + 1));
        };

        // Generate random ladder positions for each platform
        // Each platform gets exactly one ladder at a random position
        // Level 1 uses edges, level 2+ uses random middle positions

        int p0_ladder_x = (level > 1) ? randomLadderX(0, 31) : -1;
        int p1_ladder_x = (level > 1) ? randomLadderX(0, 29) : -1;
        int p2_ladder_x = (level > 1) ? randomLadderX(2, 31) : -1;
        int p3_ladder_x = (level > 1) ? randomLadderX(0, 29) : -1;
        int p4_ladder_x = (level > 1) ? randomLadderX(2, 31) : -1;

        // For level 1, use edge ladders for simplicity
        // For level 2+, NO edge ladders (only middle)
        bool p0_has_left = (level == 1) ? false : false;
        bool p0_has_right = (level == 1) ? true : false;
        bool p1_has_left = (level == 1) ? false : false;
        bool p1_has_right = (level == 1) ? true : false;
        bool p2_has_left = (level == 1) ? true : false;
        bool p2_has_right = (level == 1) ? false : false;
        bool p3_has_left = (level == 1) ? false : false;
        bool p3_has_right = (level == 1) ? true : false;
        bool p4_has_left = (level == 1) ? true : false;
        bool p4_has_right = (level == 1) ? false : false;

        // Classic Donkey Kong zigzag layout with random ladder positions
        // Bottom platform (ground) - index 0
        platforms.emplace_back(30, 0, 31, p0_has_left, p0_has_right, p0_ladder_x);

        // Platform 1 - index 1
        platforms.emplace_back(24, 0, 29, p1_has_left, p1_has_right, p1_ladder_x);

        // Platform 2 - index 2
        platforms.emplace_back(18, 2, 31, p2_has_left, p2_has_right, p2_ladder_x);

        // Platform 3 - index 3
        platforms.emplace_back(12, 0, 29, p3_has_left, p3_has_right, p3_ladder_x);

        // Platform 4 (below DK) - index 4
        platforms.emplace_back(8, 2, 31, p4_has_left, p4_has_right, p4_ladder_x);

        // Top platform where princess and DK are - index 5
        platforms.emplace_back(3, 0, 31, false, false, -1);
    }

    void spawnBarrel() {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_barrel_spawn < BARREL_SPAWN_INTERVAL) return;

        // Find inactive barrel
        for (auto& barrel : barrels) {
            if (!barrel.active) {
                // Spawn barrel on platform 4 (below DK)
                const Platform& spawn_platform = platforms[4];

                float spawn_x, velocity;
                int target_x = -1;

                // Determine ladder position
                if (spawn_platform.has_ladder_left) {
                    target_x = spawn_platform.x_start;
                } else if (spawn_platform.has_ladder_right) {
                    target_x = spawn_platform.x_end;
                } else if (spawn_platform.ladder_x >= 0) {
                    target_x = spawn_platform.ladder_x;
                }

                // Spawn from opposite side of ladder
                if (target_x >= 0) {
                    int platform_center = (spawn_platform.x_start + spawn_platform.x_end) / 2;
                    if (target_x < platform_center) {
                        // Ladder on left side, spawn from right
                        spawn_x = spawn_platform.x_end - 4.0f;
                        velocity = -(0.3f + (level * 0.05f));
                    } else {
                        // Ladder on right side, spawn from left
                        spawn_x = spawn_platform.x_start + 4.0f;
                        velocity = 0.3f + (level * 0.05f);
                    }
                } else {
                    // Fallback
                    spawn_x = spawn_platform.x_start + 4.0f;
                    velocity = 0.3f + (level * 0.05f);
                }

                barrel.spawn(spawn_x, spawn_platform.y, velocity, 4);
                last_barrel_spawn = now;
                break;
            }
        }
    }

    bool debounce(uint32_t duration = 150) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_action > duration) {
            last_action = now;
            return true;
        }
        return false;
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

        if (!player.alive || player.death_animation_playing) return;

        // Button A: Up (climb ladder up or jump)
        if (button_a && debounce()) {
            // First check if on ladder - climb up
            bool climbed = false;
            if (player.current_platform < (int)platforms.size() - 1) {
                const Platform& p = platforms[player.current_platform];
                int player_platform_y = p.y - 2; // Where player should be standing

                if (player.y == player_platform_y) {
                    // Check if at any ladder position
                    if (p.hasLadderAt(player.x)) {
                        // Move up to next platform
                        player.current_platform++;
                        player.y = platforms[player.current_platform].y - 2;
                        climbed = true;
                    }
                }
            }

            // If not climbing, jump
            if (!climbed) {
                player.startJump();
            }
        }

        // Button B: Down (climb down ladder)
        if (button_b && debounce()) {
            if (player.current_platform > 0 && !player.jumping) {
                const Platform& p = platforms[player.current_platform];
                int player_platform_y = p.y - 2;

                if (player.y == player_platform_y) {
                    // Check if at any ladder position
                    if (p.hasLadderAt(player.x)) {
                        // Move down to previous platform
                        player.current_platform--;
                        player.y = platforms[player.current_platform].y - 2;
                    }
                }
            }
        }

        // Volume Up: Right
        if (button_vol_up && debounce(100)) {
            const Platform& p = platforms[player.current_platform];
            player.x += 2;
            // Keep player on current platform
            if (player.x > p.x_end - 1) player.x = p.x_end - 1;
        }

        // Volume Down: Left
        if (button_vol_down && debounce(100)) {
            const Platform& p = platforms[player.current_platform];
            player.x -= 2;
            // Keep player on current platform
            if (player.x < p.x_start) player.x = p.x_start;
        }
    }

    void checkCollisions() {
        if (!player.alive) return;

        // Check barrel collisions (tight collision detection)
        for (const auto& barrel : barrels) {
            if (barrel.active && !barrel.falling) {
                int bx = (int)barrel.x;
                int by = (int)barrel.y - 2;  // Barrel is drawn 2 pixels above platform

                // Only check collision if player is not jumping high enough
                // Player can jump over barrels if at least 3 pixels above them
                int vertical_distance = abs(player.y - by);

                // If player is jumping and high enough above barrel, no collision
                if (player.jumping && player.y < by - 2) {
                    continue;  // Player jumped over barrel
                }

                // Both player and barrel are 2x2 pixels - check overlap
                if (abs(player.x - bx) <= 1 && vertical_distance <= 1) {
                    player.startDeathAnimation();
                    return;
                }
            }
        }

        // Check if reached princess (must be on top platform near her)
        if (player.current_platform == 5 && abs(player.x - princess_x) <= 3) {
            score += 100 * level;
            level++;
            setupLevel();
            player.reset();

            // Reset barrels
            for (auto& barrel : barrels) {
                barrel.active = false;
            }

            last_barrel_spawn = to_ms_since_boot(get_absolute_time());
        }
    }

    bool update() override {
        // Check for exit condition
        bool button_d = cosmic->is_pressed(CosmicUnicorn::SWITCH_D);
        if (checkExitCondition(button_d)) {
            return false;
        }

        // Handle input
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

        frame_count++;

        // Update player physics (jumping)
        player.updatePhysics(platforms);

        // Spawn barrels
        spawnBarrel();

        // Update barrels
        for (auto& barrel : barrels) {
            if (barrel.active) {
                barrel.update(platforms);
            }
        }

        // Check collisions
        checkCollisions();

        // Handle death
        if (!player.alive && player.isDeathAnimationComplete()) {
            lives--;
            if (lives <= 0) {
                // Game over - reset
                lives = 3;
                score = 0;
                level = 1;
                setupLevel();
            }
            player.reset();

            // Clear barrels
            for (auto& barrel : barrels) {
                barrel.active = false;
            }
        }

        return true;
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        // Clear screen
        graphics.set_pen(pens[0]);
        graphics.clear();

        // Draw platforms with better visuals - 2 pixels thick
        for (const auto& platform : platforms) {
            graphics.set_pen(pens[2]); // red top
            for (int x = platform.x_start; x <= platform.x_end; x++) {
                graphics.pixel(Point(x, platform.y));
            }
            graphics.set_pen(pens[8]); // orange bottom
            for (int x = platform.x_start; x <= platform.x_end; x++) {
                graphics.pixel(Point(x, platform.y + 1));
            }
        }

        // Draw ladders with better visuals
        for (size_t i = 0; i < platforms.size() - 1; i++) {
            const Platform& p = platforms[i];
            const Platform& p_next = platforms[i + 1];

            // Left ladder
            if (p.has_ladder_left) {
                for (int ly = p_next.y + 2; ly < p.y; ly++) {
                    if (ly % 2 == 0) {
                        graphics.set_pen(pens[7]); // cyan
                        graphics.pixel(Point(p.x_start, ly));
                        graphics.pixel(Point(p.x_start + 1, ly));
                    } else {
                        graphics.set_pen(pens[3]); // blue
                        graphics.pixel(Point(p.x_start, ly));
                        graphics.pixel(Point(p.x_start + 1, ly));
                    }
                }
            }

            // Right ladder
            if (p.has_ladder_right) {
                for (int ly = p_next.y + 2; ly < p.y; ly++) {
                    if (ly % 2 == 0) {
                        graphics.set_pen(pens[7]); // cyan
                        graphics.pixel(Point(p.x_end - 1, ly));
                        graphics.pixel(Point(p.x_end, ly));
                    } else {
                        graphics.set_pen(pens[3]); // blue
                        graphics.pixel(Point(p.x_end - 1, ly));
                        graphics.pixel(Point(p.x_end, ly));
                    }
                }
            }

            // Middle ladder (at random position)
            if (p.ladder_x >= 0) {
                for (int ly = p_next.y + 2; ly < p.y; ly++) {
                    if (ly % 2 == 0) {
                        graphics.set_pen(pens[7]); // cyan
                        graphics.pixel(Point(p.ladder_x, ly));
                        graphics.pixel(Point(p.ladder_x + 1, ly));
                    } else {
                        graphics.set_pen(pens[3]); // blue
                        graphics.pixel(Point(p.ladder_x, ly));
                        graphics.pixel(Point(p.ladder_x + 1, ly));
                    }
                }
            }
        }

        // Draw Donkey Kong at top left - bigger and better
        graphics.set_pen(pens[12]); // dark brown body
        graphics.rectangle(Rect(dk_x, dk_y, 3, 2));
        graphics.set_pen(pens[6]); // brown face
        graphics.rectangle(Rect(dk_x + 1, dk_y - 1, 2, 2));
        graphics.set_pen(pens[2]); // red eyes
        graphics.pixel(Point(dk_x + 1, dk_y - 1));

        // Draw princess - better sprite
        graphics.set_pen(pens[9]); // pink dress
        graphics.rectangle(Rect(princess_x, princess_y + 1, 3, 2));
        graphics.set_pen(pens[5]); // white face
        graphics.pixel(Point(princess_x + 1, princess_y));
        graphics.set_pen(pens[4]); // yellow hair
        graphics.pixel(Point(princess_x, princess_y));
        graphics.pixel(Point(princess_x + 2, princess_y));

        // Draw barrels with animation
        for (const auto& barrel : barrels) {
            if (barrel.active) {
                int bx = (int)barrel.x;
                int by = (int)barrel.y - 2;  // Draw barrel 2 pixels above platform

                // Barrel body - 2x2 pixels
                graphics.set_pen(pens[6]); // brown
                graphics.pixel(Point(bx, by));
                graphics.pixel(Point(bx + 1, by));
                graphics.pixel(Point(bx, by + 1));
                graphics.pixel(Point(bx + 1, by + 1));

                // Barrel highlight (animated while rolling)
                if (!barrel.falling && (frame_count / 4) % 2 == 0) {
                    graphics.set_pen(pens[8]); // orange highlight
                    graphics.pixel(Point(bx, by));
                    graphics.pixel(Point(bx + 1, by + 1));
                }
            }
        }

        // Draw player - better Mario sprite
        if (player.death_animation_playing) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            uint32_t animation_time = now - player.death_animation_start;

            // Explosion animation
            if ((animation_time / 80) % 2 == 0) {
                graphics.set_pen(pens[2]); // red
                graphics.rectangle(Rect(player.x - 1, player.y - 1, 4, 4));
            } else {
                graphics.set_pen(pens[4]); // yellow
                graphics.pixel(Point(player.x, player.y));
                graphics.pixel(Point(player.x + 1, player.y));
                graphics.pixel(Point(player.x, player.y + 1));
                graphics.pixel(Point(player.x + 1, player.y + 1));
            }
        } else if (player.alive) {
            // Mario body - red
            graphics.set_pen(pens[2]);
            graphics.rectangle(Rect(player.x, player.y, 2, 2));

            // Face - beige/white
            graphics.set_pen(pens[5]);
            graphics.pixel(Point(player.x, player.y + 1));
            graphics.pixel(Point(player.x + 1, player.y + 1));

            // Hat - red highlights
            graphics.set_pen(pens[2]);
            graphics.pixel(Point(player.x, player.y));
            graphics.pixel(Point(player.x + 1, player.y));
        }

        // Draw UI - lives with better icons
        for (int i = 0; i < lives; i++) {
            graphics.set_pen(pens[2]); // red
            graphics.rectangle(Rect(i * 4, 0, 2, 2));
        }

        // Draw level indicator
        for (int i = 0; i < level && i < 8; i++) {
            graphics.set_pen(pens[4]); // yellow
            graphics.pixel(Point(31 - i, 0));
        }
    }

    const char* getName() const override {
        return "Cosmic Kong";
    }

    const char* getDescription() const override {
        return "Climb to save the princess!";
    }
};
