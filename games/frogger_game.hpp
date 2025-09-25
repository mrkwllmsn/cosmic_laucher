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

// Lane types
enum LaneType {
    SAFE_START,
    ROAD,
    SAFE_MIDDLE, 
    WATER,
    BRIDGE
};

struct Lane {
    int y;                    // Y position on screen (0=top, 31=bottom)
    LaneType type;
    std::string pattern;      // Pattern of objects in this lane
    int speed;               // Scroll speed (positive=right, negative=left, 0=no scroll)
    
    Lane(int y_pos, LaneType lane_type, const std::string& pat, int spd) 
        : y(y_pos), type(lane_type), pattern(pat), speed(spd) {}
    
    void update(uint32_t frame_count) {
        if (speed == 0) return;
        
        if (frame_count % abs(speed) == 0) {
            if (speed > 0) {
                // Scroll right: move last char to front
                char last = pattern.back();
                pattern = last + pattern.substr(0, pattern.length() - 1);
            } else {
                // Scroll left: move first char to back
                char first = pattern[0];
                pattern = pattern.substr(1) + first;
            }
        }
    }
    
    void draw(PicoGraphics_PenRGB888& graphics, const std::vector<Pen>& pens) {
        for (int x = 0; x < 32 && x < (int)pattern.length(); x++) {
            char tile = pattern[x];
            Pen color = pens[0]; // black_pen
            
            switch (tile) {
                case '.': color = pens[0]; break;         // Safe ground (black)
                case '_': color = pens[9]; break;        // Safe zone (purple)
                case 'r': color = pens[2]; break;        // Red car
                case 'b': color = pens[3]; break;        // Blue car
                case 'c': color = pens[7]; break;        // Cyan car
                case 'y': color = pens[4]; break;        // Yellow car
                case 'O': color = pens[5]; break;        // Car highlight (white)
                case '~': color = pens[17]; break;       // Water (dark blue)
                case 'n': color = pens[6]; break;        // Log (brown)
                case 't': color = pens[6]; break;        // Log (brown)
                case 'T': 
                    if (x % 3 == 0) {
                        color = pens[15];  // Light blue
                    } else {
                        color = pens[8];   // Orange
                    }
                    break;
                case '+': color = pens[6]; break;        // Log connector (brown)
                case '1': case '2': case '3': case '4': case '5':
                    color = pens[0]; break;              // Bridge slots (black)
                case 'k': color = pens[2]; break;       // Bridge (red)
                case 'm': color = pens[2]; break;       // Bridge (red)
                case 'W': color = pens[5]; break;       // Bridge white
                case 'g': color = pens[1]; break;       // Completed slot (green)
                case 'Q': color = pens[15]; break;      // Light blue
                case 'F': color = pens[1]; break;       // Green
                case 's': color = pens[11]; break;      // Snake (striped green)
                case 'S': color = pens[9]; break;       // Snake alt (striped purple)
                case 'o': color = pens[8]; break;       // Orange
                case 'p': color = pens[12]; break;      // Pink
                case 'w': color = pens[5]; break;       // White (lowercase)
                default: color = pens[0]; break;        // Default to black
            }
            
            if (color != pens[0]) {
                graphics.set_pen(color);
                graphics.pixel(Point(x, y));
            }
            
            // Add two-tone water effect like Python version
            if (type == WATER && tile == '~' && x % 2 == 0) {
                graphics.set_pen(pens[18]);  // Lighter blue on alternating positions
                graphics.pixel(Point(x, y));
            }
        }
    }
};

enum DeathType {
    NONE,
    CAR_HIT,
    DROWNING
};

struct Frog {
    int x, y;
    int lives;
    int score;
    int level;
    bool alive;
    DeathType death_type;
    uint32_t death_animation_start;
    bool death_animation_playing;
    
    Frog() : x(15), y(30), lives(3), score(0), level(1), alive(true), 
             death_type(NONE), death_animation_start(0), death_animation_playing(false) {}
    
    void draw(PicoGraphics_PenRGB888& graphics, const std::vector<Pen>& pens) {
        if (death_animation_playing) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            uint32_t animation_time = now - death_animation_start;
            
            if (death_type == CAR_HIT) {
                // Red death animation - flash red and fade
                if (animation_time < 1000) { // 1 second animation
                    int flash_phase = (animation_time / 100) % 2; // Flash every 100ms
                    if (flash_phase == 0) {
                        graphics.set_pen(pens[2]); // red_pen
                        graphics.rectangle(Rect(x, y, 2, 2));
                    } else {
                        graphics.set_pen(pens[8]); // orange_pen
                        graphics.rectangle(Rect(x, y, 2, 2));
                    }
                }
            } else if (death_type == DROWNING) {
                // White splash animation - expanding circles
                if (animation_time < 800) { // 0.8 second animation
                    int splash_size = (animation_time / 100) + 1; // Grow every 100ms
                    graphics.set_pen(pens[5]); // white_pen
                    
                    // Draw expanding splash effect
                    for (int i = 0; i < splash_size && i < 4; i++) {
                        int splash_x = x + 1 - i;
                        int splash_y = y + 1 - i;
                        int splash_w = 2 * i;
                        int splash_h = 2 * i;
                        
                        if (splash_x >= 0 && splash_y >= 0 && 
                            splash_x + splash_w < 32 && 
                            splash_y + splash_h < 32) {
                            // Draw splash outline
                            for (int sx = 0; sx < splash_w; sx++) {
                                graphics.pixel(Point(splash_x + sx, splash_y));
                                graphics.pixel(Point(splash_x + sx, splash_y + splash_h - 1));
                            }
                            for (int sy = 0; sy < splash_h; sy++) {
                                graphics.pixel(Point(splash_x, splash_y + sy));
                                graphics.pixel(Point(splash_x + splash_w - 1, splash_y + sy));
                            }
                        }
                    }
                }
            }
        } else if (!alive) {
            // Default red square for dead frog (fallback)
            graphics.set_pen(pens[2]); // red_pen
            graphics.rectangle(Rect(x, y, 2, 2));
        } else {
            // Normal alive frog
            graphics.set_pen(pens[1]); // green_pen
            graphics.rectangle(Rect(x, y, 2, 2));
            graphics.set_pen(pens[4]); // yellow_pen
            graphics.pixel(Point(x, y));
            graphics.pixel(Point(x+1, y));
        }
    }
    
    void move(int dx, int dy) {
        if (!alive || death_animation_playing) return;
        
        x += dx;
        y += dy;
        
        // Keep frog on screen
        if (x < 0) x = 0;
        if (x > 30) x = 30;  // 32 - 2 for frog size
        if (y < 0) y = 0;
        if (y > 30) y = 30;  // 32 - 2 for frog size
    }
    
    void startDeathAnimation(DeathType type) {
        death_type = type;
        death_animation_start = to_ms_since_boot(get_absolute_time());
        death_animation_playing = true;
        alive = false;
    }
    
    bool isDeathAnimationComplete() {
        if (!death_animation_playing) return true;
        
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint32_t animation_time = now - death_animation_start;
        
        if (death_type == CAR_HIT) {
            return animation_time >= 1000; // 1 second
        } else if (death_type == DROWNING) {
            return animation_time >= 800;  // 0.8 seconds
        }
        return true;
    }
    
    void reset() {
        x = 15;
        y = 30;
        alive = true;
        death_type = NONE;
        death_animation_playing = false;
        death_animation_start = 0;
    }
};

class FroggerGame : public GameBase {
private:
    // Constants
    static const int DISPLAY_WIDTH = 32;
    static const int DISPLAY_HEIGHT = 32;
    static const int TIME_LIMIT = 60;
    
    // Game state
    uint32_t start_time;
    uint32_t frame_count = 0;
    uint32_t last_action = 0;
    
    // Game objects
    std::vector<Lane> lanes;
    Frog player;
    
    // Pen colors (stored as vector for easy access)
    std::vector<Pen> pens;
    
    // Debounce duration
    const uint32_t DEBOUNCE_DURATION = 200;
    
public:
    FroggerGame() : start_time(0), frame_count(0), last_action(0) {}
    
    virtual ~FroggerGame() = default;
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        cosmic->set_brightness(0.6);
        
        initPens();
        start_time = to_ms_since_boot(get_absolute_time());
        setupLanes();
    }
    
    void initPens() {
        pens.clear();
        pens.reserve(20);
        
        // Index 0: black_pen
        pens.push_back(gfx->create_pen(0, 0, 0));
        // Index 1: green_pen
        pens.push_back(gfx->create_pen(0, 255, 0));
        // Index 2: red_pen
        pens.push_back(gfx->create_pen(255, 0, 0));
        // Index 3: blue_pen
        pens.push_back(gfx->create_pen(0, 0, 255));
        // Index 4: yellow_pen
        pens.push_back(gfx->create_pen(255, 255, 0));
        // Index 5: white_pen
        pens.push_back(gfx->create_pen(255, 255, 255));
        // Index 6: brown_pen
        pens.push_back(gfx->create_pen(139, 69, 19));
        // Index 7: cyan_pen
        pens.push_back(gfx->create_pen(0, 255, 255));
        // Index 8: orange_pen
        pens.push_back(gfx->create_pen(255, 165, 0));
        // Index 9: purple_pen
        pens.push_back(gfx->create_pen(128, 0, 128));
        // Index 10: gray_pen
        pens.push_back(gfx->create_pen(128, 128, 128));
        // Index 11: snake_pen
        pens.push_back(gfx->create_pen(0, 155, 30));
        // Index 12: pink_pen
        pens.push_back(gfx->create_pen(255, 92, 203));
        // Index 13: frog_green_pen
        pens.push_back(gfx->create_pen(50, 255, 90));
        // Index 14: frog_eyes_pen
        pens.push_back(gfx->create_pen(255, 255, 0));
        // Index 15: light_blue_pen
        pens.push_back(gfx->create_pen(0, 155, 255));
        // Index 16: dark_blue_pen
        pens.push_back(gfx->create_pen(30, 30, 80));
        // Index 17: water_blue_pen
        pens.push_back(gfx->create_pen(0, 13, 105));
        // Index 18: water_blue2_pen
        pens.push_back(gfx->create_pen(0, 55, 245));
        // Index 19: dark_gray_pen
        pens.push_back(gfx->create_pen(2, 2, 2));
    }
    
    void setupLanes() {
        lanes.clear();
        
        // Bridge area (top)
        lanes.emplace_back(0, BRIDGE, "kmkWkmkmkWkmkmkWkmkmkWkmkmkWkmkm", 0);
        lanes.emplace_back(1, BRIDGE, "kmkWkmkmkWkmkmkWkmkmkWkmkmkWkmkm", 0);
        lanes.emplace_back(2, BRIDGE, "mk111kmk222kmk333kmk444kmk555kmk", 0);
        lanes.emplace_back(3, BRIDGE, "mk111kmk222kmk333kmk444kmk555kmk", 0);
        
        // Water area with logs (slower speeds: higher numbers = slower movement)
        lanes.emplace_back(4, WATER, "~~~~~tt+tt+tt~~~~~~~~~~~~~~~tttttt+to~~~~~~~~~~~~~~~~~~~~~~~~~", -8);
        lanes.emplace_back(5, WATER, "~~~~~TT+TT+TT~~~~~~~~~~~~~~~TTTTTT+To~~~~~~~~~~~~~~~~~~~~~~~~~", -8);
        lanes.emplace_back(6, WATER, "~~nnnno~~~~~~~~~~~~~~~~nno~~~~~~nno~~~nnnnno~~~~~~~~~~", 10);
        lanes.emplace_back(7, WATER, "~~nnnno~~~~~~~~~~~~~~~~nno~~~~~~nno~~~nnnnno~~~~~~~~~~", 10);
        lanes.emplace_back(8, WATER, "~~~~nnnno~~~~~~~~~~~~~~~~~nnno~~~~~~~~~~~~~~~nnnnno~~~~~~~~~~~~", -12);
        lanes.emplace_back(9, WATER, "~~~~nnnno~~~~~~~~~~~~~~~~~nnno~~~~~~~~~~~~~~~nnnnno~~~~~~~~~~~~", -12);
        lanes.emplace_back(10, WATER, "~~~~~tt+tto~~~~~~~~~~~~~~~~tttt+tto~~~~~~~~~~~~~~~~~", 6);
        lanes.emplace_back(11, WATER, "~~~~~TT+TTo~~~~~~~~~~~~~~~~TTTT+TTo~~~~~~~~~~~~~~~~~", 6);
        
        // Safe middle zone
        lanes.emplace_back(12, SAFE_MIDDLE, "________________________________", 0);
        lanes.emplace_back(13, SAFE_MIDDLE, "________________________________", 0);
        
        // Road area with cars (slower speeds for better gameplay)
        lanes.emplace_back(14, ROAD, "...r................c...........b....p..........c.................p...................", 8);
        lanes.emplace_back(15, ROAD, "..rOr..............cOc.........bOb..w.w........OcO...............ObO..................", 8);
        lanes.emplace_back(16, ROAD, "......y...............r..........c.....O......r..........O............................", -3);
        lanes.emplace_back(17, ROAD, ".....yOy.............rOr........cOc...bbb....OrO........ggg...........................", -3);
        lanes.emplace_back(18, ROAD, "..b........c..................r........ccc.........rrr............gwg...........................", -6);
        lanes.emplace_back(19, ROAD, ".bOb......cOc................rOr......bObO........rOrO...........gOgO...........................", -6);
        lanes.emplace_back(20, ROAD, ".........y..................b.......ppp.........c.................b.........c.........", 7);
        lanes.emplace_back(21, ROAD, "........yOy................bOb......OppO.......O.O...............ObO.......cOc........", 7);
        lanes.emplace_back(22, ROAD, "...c.........r..............bb.........ccc....cwc............yyyW...........", -12);
        lanes.emplace_back(23, ROAD, "..cOc.......rOr............bOb........bObO...bObO...........yOyyO...........", -12);
        lanes.emplace_back(24, ROAD, "................r.........y.........c....................", -5);
        lanes.emplace_back(25, ROAD, "...............rOr.......yOy.......cOc...................", -5);
        lanes.emplace_back(26, ROAD, ".bb...................c.......rrr...................", 9);
        lanes.emplace_back(27, ROAD, "ObOb.................cOc.....rOrOr..................", 9);
        
        // Safe start area (bottom)
        lanes.emplace_back(28, SAFE_START, "................................", 0);
        lanes.emplace_back(29, SAFE_START, "................................", 0);
        lanes.emplace_back(30, SAFE_START, "................................", 0);
        lanes.emplace_back(31, SAFE_START, "................................", 0);
    }
    
    void gameUpdate() {
        frame_count++;
        
        // Update all lanes
        for (auto& lane : lanes) {
            lane.update(frame_count);
        }
        
        checkCollisions();
        checkTimer();
    }
    
    void checkCollisions() {
        if (!player.alive) return;
        
        // Find the lane the player is on
        Lane* current_lane = nullptr;
        for (auto& lane : lanes) {
            if (lane.y == player.y) {
                current_lane = &lane;
                break;
            }
        }
        
        if (!current_lane) return;
        
        char tile1 = (player.x < (int)current_lane->pattern.length()) ? current_lane->pattern[player.x] : '.';
        char tile2 = (player.x + 1 < (int)current_lane->pattern.length()) ? current_lane->pattern[player.x + 1] : '.';
        
        if (current_lane->type == ROAD) {
            // On road - check if hit by car
            if ((tile1 != '.' && tile1 != ' ') || (tile2 != '.' && tile2 != ' ')) {
                player.startDeathAnimation(CAR_HIT);
                return;
            }
        } else if (current_lane->type == WATER) {
            // In water - must be on log or die
            if ((tile1 == '~' || tile1 == ' ') && (tile2 == '~' || tile2 == ' ')) {
                player.startDeathAnimation(DROWNING);
                return;
            }
            
            // Move with logs
            if (current_lane->speed != 0 && frame_count % abs(current_lane->speed) == 0) {
                if (current_lane->speed > 0) {
                    player.x += 1;
                } else {
                    player.x -= 1;
                }
                
                if (player.x < 0 || player.x > DISPLAY_WIDTH - 2) {
                    player.startDeathAnimation(DROWNING);
                }
            }
        } else if (current_lane->type == BRIDGE) {
            // Check if reached scoring position
            if (tile1 >= '1' && tile1 <= '5') {
                player.score++;
                
                // Mark bridge slot as completed
                for (int i = 0; i < (int)current_lane->pattern.length() - 2; i++) {
                    if (current_lane->pattern.substr(i, 3) == std::string(3, tile1)) {
                        current_lane->pattern.replace(i, 3, "ggg");
                        break;
                    }
                }
                
                player.reset();
                start_time = to_ms_since_boot(get_absolute_time());
                
                if (player.score % 5 == 0) {
                    player.level++;
                    setupLanes(); // Reset level
                }
            }
        }
    }
    
    void checkTimer() {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed = (now - start_time) / 1000;
        
        if (elapsed > TIME_LIMIT && player.alive) {
            // Time out - trigger drowning animation as default
            player.startDeathAnimation(DROWNING);
        }
        
        if (!player.alive && player.isDeathAnimationComplete()) {
            player.lives--;
            if (player.lives > 0) {
                player.reset();
                start_time = to_ms_since_boot(get_absolute_time());
            } else {
                // Game over
                player.lives = 3;
                player.score = 0;
                player.level = 1;
                player.reset();
                start_time = to_ms_since_boot(get_absolute_time());
                setupLanes();
            }
        }
    }
    
    bool debounce(uint32_t duration = 200) {
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
        
        // Handle movement controls
        if (button_a && debounce()) {
            player.move(0, -2);  // Move up (toward bridge)
        }
        
        if (button_b && debounce()) {
            player.move(0, 2);   // Move down (toward start)
        }
        
        if (button_vol_up && debounce()) {
            player.move(1, 0);   // Move right
        }
        
        if (button_vol_down && debounce()) {
            player.move(-1, 0);  // Move left
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
        
        // Update game logic
        gameUpdate();
        
        return true;  // Continue game
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        // Clear screen
        graphics.set_pen(pens[0]); // black_pen
        graphics.clear();
        
        // Draw all lanes
        for (auto& lane : lanes) {
            lane.draw(graphics, pens);
        }
        
        // Draw player
        player.draw(graphics, pens);
        
        // Draw UI
        drawUI(graphics);
    }
    
    void drawUI(PicoGraphics_PenRGB888& graphics) {
        // Draw lives
        for (int i = 0; i < player.lives; i++) {
            graphics.set_pen(pens[1]); // green_pen
            graphics.pixel(Point(i * 3, 0));
            graphics.pixel(Point(i * 3 + 1, 0));
        }
        
        // Draw timer
        uint32_t now = to_ms_since_boot(get_absolute_time());
        uint32_t elapsed = (now - start_time) / 1000;
        int remaining = TIME_LIMIT - elapsed;
        
        for (int i = 0; i < remaining / 2 && i < DISPLAY_WIDTH; i++) {
            graphics.set_pen(pens[4]); // yellow_pen
            graphics.pixel(Point(i, DISPLAY_HEIGHT - 1));
        }
    }
    
    const char* getName() const override {
        return "Cosmic Frogger";
    }
    
    const char* getDescription() const override {
        return "Navigate the frog across roads and rivers to reach the bridge";
    }
};