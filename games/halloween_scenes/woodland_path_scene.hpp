#pragma once

#include "../../game_base.hpp"
#include "../animated_eyes.hpp"
#include "../../effects/lightning.hpp"
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>

struct TreeNode {
    float x, y;
    float angle;
    float length;
    int depth;
    bool visible;
};

struct Boid {
    float x, y;
    float vx, vy;
    float wing_phase;
    float max_speed = 1.2f;
    float max_force = 0.03f;
    
    Boid(float start_x, float start_y) : x(start_x), y(start_y), 
         vx((rand() % 100 - 50) / 100.0f), vy((rand() % 100 - 50) / 100.0f), wing_phase(0) {}
};

struct Theme {
    // Store RGB values directly instead of pen colors
    uint8_t sky_top_r, sky_top_g, sky_top_b;
    uint8_t sky_bottom_r, sky_bottom_g, sky_bottom_b;
    uint8_t dark_land_r, dark_land_g, dark_land_b;
    uint8_t lighter_land_r, lighter_land_g, lighter_land_b;
    uint8_t tree_trunk_r, tree_trunk_g, tree_trunk_b;
    uint8_t tree_dark_r, tree_dark_g, tree_dark_b;
    uint8_t tree_leaves_r, tree_leaves_g, tree_leaves_b;
    uint8_t tree_dark_leaves_r, tree_dark_leaves_g, tree_dark_leaves_b;
    uint8_t moon_glow_r, moon_glow_g, moon_glow_b;
    uint8_t path_color_r, path_color_g, path_color_b;
    uint8_t bat_color_r, bat_color_g, bat_color_b;
    std::string name;
};

class WoodlandPathScene {
private:
    static constexpr int MAX_TREES = 15;
    static constexpr int MAX_TREE_NODES = 150;
    static constexpr float PATH_WIDTH = 8.0f;
    static constexpr float SPEED = 3.0f;
    static constexpr int MAX_BATS = 8;
    static constexpr float THEME_CHANGE_TIME = 15.0f; // Change theme every 15 seconds
    
    struct Tree {
        float roadY;           // Distance from viewer (0=horizon, 1=foreground)
        float trackPosition;   // Left/right position relative to road center
        float base_angle;
        float size_multiplier; // Tree size variation factor (0.5 to 2.0)
        std::vector<TreeNode> nodes;
        bool active;
    };
    
    std::vector<Tree> trees;
    std::vector<Boid> boids;
    std::vector<Theme> themes;
    int current_theme_index;
    float theme_timer;
    bool last_c_pressed;
    
    float distance;           // Total distance traveled
    float road_curve;         // Current road curve amount
    float animation_phase;
    uint32_t last_update_time;
    
    // Boid spreading behavior
    float spreading_timer;
    bool spreading_mode;
    static constexpr float FLOCK_TIME = 4.0f;     // 4 seconds of flocking
    static constexpr float SPREAD_TIME = 2.0f;    // 2 seconds of spreading
    
    // Variable speed system
    enum SpeedState { STOPPED, WALKING, RUNNING };
    SpeedState current_speed_state;
    float speed_timer;
    float current_speed;
    static constexpr float STOP_SPEED = 0.0f;
    static constexpr float WALK_SPEED = 1.5f;
    static constexpr float RUN_SPEED = 4.5f;
    
    // Tree angle adjustment system
    float tree_angle_offset;
    bool last_volume_up_pressed;
    bool last_volume_down_pressed;
    static constexpr float ANGLE_STEP = 5.0f;  // Degrees to adjust per button press
    static constexpr float MIN_ANGLE = 10.0f;  // Minimum branch angle
    static constexpr float MAX_ANGLE = 45.0f;  // Maximum branch angle
    
    // Spooky eyes system
    AnimatedEye tree_eyes;
    bool eyes_visible;
    float eyes_timer;
    static constexpr float EYES_APPEAR_CHANCE = 0.3f;  // 30% chance when stopped
    static constexpr float EYES_DISPLAY_TIME = 3.0f;   // Show eyes for 3 seconds
    
    // Lightning system
    Lightning lightning;
    float tree_flash_timer;
    bool tree_flash_active;
    float lightning_x, lightning_y;  // Position of last lightning strike
    static constexpr float TREE_FLASH_DURATION = 0.15f;  // Brief momentary flash
    static constexpr float LIGHTNING_TREE_RANGE = 20.0f;  // Larger range to catch more trees
    
    // Current theme colors (will be updated from themes vector)
    Theme current_theme;
    
public:
    void init(PicoGraphics* graphics) {
        trees.clear();
        trees.reserve(MAX_TREES);
        boids.clear();
        
        distance = 0.0f;
        road_curve = 0.0f;
        animation_phase = 0.0f;
        theme_timer = 0.0f;
        current_theme_index = 0;
        last_c_pressed = false;
        last_update_time = to_ms_since_boot(get_absolute_time());
        
        // Initialize spreading behavior
        spreading_timer = 0.0f;
        spreading_mode = false;  // Start in flocking mode
        
        // Initialize speed system
        current_speed_state = WALKING;  // Start walking
        speed_timer = 0.0f;
        current_speed = WALK_SPEED;
        
        // Initialize tree angle system
        tree_angle_offset = 0.0f;  // Start with default angle
        last_volume_up_pressed = false;
        last_volume_down_pressed = false;
        
        // Initialize eyes system
        tree_eyes.init(static_cast<PicoGraphics_PenRGB888&>(*graphics));

        tree_eyes.disableRepositioning();
        eyes_visible = false;
        eyes_timer = 0.0f;
        
        // Initialize lightning system
        lightning.init();
        lightning.setSpawnChance(0.005f);  // Less frequent for atmospheric effect
        lightning.setStartArea(5.0f, 27.0f, 0.0f, 8.0f);  // Sky area
        lightning.setTargetArea(20.0f, 32.0f);  // Ground area
        lightning.setLightningColor(255, 255, 255);
        lightning.setLightningGlowColor(200, 220, 255);
        
        // Set up callback for tree flash effect
        lightning.setStrikeCallback([this](float x, float y, float intensity) {
            lightning_x = x;
            lightning_y = y;
            tree_flash_active = true;
            tree_flash_timer = TREE_FLASH_DURATION;
        });
        
        tree_flash_active = false;
        tree_flash_timer = 0.0f;
        
        loadThemes(graphics);
        initializeBoids();
        generateInitialTrees();
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
                theme_timer = 0.0f; // Reset automatic timer when manually changed
            }
        }
        last_c_pressed = c_pressed;
        
        // Check for tree angle adjustment with volume buttons
        if (cosmic) {
            bool volume_up_pressed = cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_UP);
            bool volume_down_pressed = cosmic->is_pressed(CosmicUnicorn::SWITCH_VOLUME_DOWN);
            
            if (volume_up_pressed && !last_volume_up_pressed) {
                tree_angle_offset = std::min(tree_angle_offset + ANGLE_STEP, MAX_ANGLE - 25.0f);
            }
            
            if (volume_down_pressed && !last_volume_down_pressed) {
                tree_angle_offset = std::max(tree_angle_offset - ANGLE_STEP, MIN_ANGLE - 25.0f);
            }
            
            last_volume_up_pressed = volume_up_pressed;
            last_volume_down_pressed = volume_down_pressed;
        }
        
        // Update speed state system
        speed_timer += dt;
        updateSpeedState();
        
        // Update lightning system
        lightning.update(dt);
        
        // Update tree flash effect
        if (tree_flash_active) {
            tree_flash_timer -= dt;
            if (tree_flash_timer <= 0.0f) {
                tree_flash_active = false;
            }
        }
        
        // Update spooky eyes system
        updateEyesSystem(dt);
        
        // Move forward with variable speed
        distance += current_speed * dt;
        animation_phase += dt * 0.5f;
        theme_timer += dt;
        
        // Update theme periodically (automatic cycling)
        if (theme_timer >= THEME_CHANGE_TIME && !themes.empty()) {
            theme_timer = 0.0f;
            current_theme_index = (current_theme_index + 1) % themes.size();
            current_theme = themes[current_theme_index];
        }
        
        // Update spreading behavior cycle
        spreading_timer += dt;
        if (spreading_mode) {
            // Currently in spreading mode
            if (spreading_timer >= SPREAD_TIME) {
                spreading_mode = false;
                spreading_timer = 0.0f;
            }
        } else {
            // Currently in flocking mode
            if (spreading_timer >= FLOCK_TIME) {
                spreading_mode = true;
                spreading_timer = 0.0f;
            }
        }
        
        // Update road curve based on distance
        road_curve = sin(distance * 0.15f) * 3.0f;
        
        // Update boids
        updateBoids();
        
        // Update tree positions - move them toward viewer
        for (auto& tree : trees) {
            if (tree.active) {
                tree.roadY += current_speed * dt * 0.3f;
                
                // If tree has moved past viewer, respawn it at horizon
                if (tree.roadY > 1.2f) {
                    respawnTree(tree);
                }
                
                // Regenerate tree branches based on new distance
                generateTreeBranches(tree);
            }
        }
        
        // Occasionally spawn new trees
        if ((rand() % 100) < 2) {
            spawnNewTree();
        }
    }
    
    void render(PicoGraphics* graphics) {
        drawGradientSky(graphics);
        drawLandscape(graphics);
        drawMoon(graphics);
        drawPath(graphics);
        drawBats(graphics);  // Draw bats before trees so they appear behind
        lightning.render(graphics);  // Draw lightning on top of trees but below eyes
        drawTrees(graphics);
        drawEyes(graphics);  // Draw eyes on top of everything when visible
    }
    
private:
    // Helper function to create darkened pen based on distance (like arcade racer)
    uint32_t createDarkenedPen(PicoGraphics* graphics, uint8_t r, uint8_t g, uint8_t b, float perspective) {
        // Same brightness formula as arcade racer: darker at horizon, brighter in foreground
        float brightness = 0.2f + 0.8f * perspective;
        
        int dark_r = (int)(r * brightness);
        int dark_g = (int)(g * brightness);
        int dark_b = (int)(b * brightness);
        
        return graphics->create_pen(dark_r, dark_g, dark_b);
    }

    void loadThemes(PicoGraphics* graphics) {
        themes.clear();
        
        // Hardcode themes for now to test functionality
        Theme classic_halloween;
        classic_halloween.sky_top_r = 25; classic_halloween.sky_top_g = 15; classic_halloween.sky_top_b = 45;
        classic_halloween.sky_bottom_r = 15; classic_halloween.sky_bottom_g = 5; classic_halloween.sky_bottom_b = 25;
        classic_halloween.dark_land_r = 40; classic_halloween.dark_land_g = 80; classic_halloween.dark_land_b = 30;
        classic_halloween.lighter_land_r = 60; classic_halloween.lighter_land_g = 120; classic_halloween.lighter_land_b = 45;
        classic_halloween.tree_trunk_r = 140; classic_halloween.tree_trunk_g = 90; classic_halloween.tree_trunk_b = 50;
        classic_halloween.tree_dark_r = 100; classic_halloween.tree_dark_g = 60; classic_halloween.tree_dark_b = 30;
        classic_halloween.tree_leaves_r = 40; classic_halloween.tree_leaves_g = 150; classic_halloween.tree_leaves_b = 40;
        classic_halloween.tree_dark_leaves_r = 20; classic_halloween.tree_dark_leaves_g = 100; classic_halloween.tree_dark_leaves_b = 20;
        classic_halloween.moon_glow_r = 255; classic_halloween.moon_glow_g = 255; classic_halloween.moon_glow_b = 200;
        classic_halloween.path_color_r = 160; classic_halloween.path_color_g = 140; classic_halloween.path_color_b = 100;
        classic_halloween.bat_color_r = 80; classic_halloween.bat_color_g = 40; classic_halloween.bat_color_b = 80;
        classic_halloween.name = "Classic Halloween";
        themes.push_back(classic_halloween);
        
        Theme blood_moon;
        blood_moon.sky_top_r = 40; blood_moon.sky_top_g = 15; blood_moon.sky_top_b = 15;
        blood_moon.sky_bottom_r = 20; blood_moon.sky_bottom_g = 5; blood_moon.sky_bottom_b = 5;
        blood_moon.dark_land_r = 60; blood_moon.dark_land_g = 20; blood_moon.dark_land_b = 20;
        blood_moon.lighter_land_r = 90; blood_moon.lighter_land_g = 30; blood_moon.lighter_land_b = 30;
        blood_moon.tree_trunk_r = 120; blood_moon.tree_trunk_g = 60; blood_moon.tree_trunk_b = 40;
        blood_moon.tree_dark_r = 80; blood_moon.tree_dark_g = 40; blood_moon.tree_dark_b = 20;
        blood_moon.tree_leaves_r = 80; blood_moon.tree_leaves_g = 40; blood_moon.tree_leaves_b = 40;
        blood_moon.tree_dark_leaves_r = 60; blood_moon.tree_dark_leaves_g = 20; blood_moon.tree_dark_leaves_b = 20;
        blood_moon.moon_glow_r = 255; blood_moon.moon_glow_g = 255; blood_moon.moon_glow_b = 200;
        blood_moon.path_color_r = 140; blood_moon.path_color_g = 100; blood_moon.path_color_b = 80;
        blood_moon.bat_color_r = 120; blood_moon.bat_color_g = 60; blood_moon.bat_color_b = 60;
        blood_moon.name = "Blood Moon";
        themes.push_back(blood_moon);
        
        Theme toxic_swamp;
        toxic_swamp.sky_top_r = 20; toxic_swamp.sky_top_g = 40; toxic_swamp.sky_top_b = 15;
        toxic_swamp.sky_bottom_r = 10; toxic_swamp.sky_bottom_g = 20; toxic_swamp.sky_bottom_b = 5;
        toxic_swamp.dark_land_r = 30; toxic_swamp.dark_land_g = 60; toxic_swamp.dark_land_b = 20;
        toxic_swamp.lighter_land_r = 50; toxic_swamp.lighter_land_g = 100; toxic_swamp.lighter_land_b = 30;
        toxic_swamp.tree_trunk_r = 100; toxic_swamp.tree_trunk_g = 120; toxic_swamp.tree_trunk_b = 60;
        toxic_swamp.tree_dark_r = 60; toxic_swamp.tree_dark_g = 80; toxic_swamp.tree_dark_b = 40;
        toxic_swamp.tree_leaves_r = 60; toxic_swamp.tree_leaves_g = 150; toxic_swamp.tree_leaves_b = 40;
        toxic_swamp.tree_dark_leaves_r = 40; toxic_swamp.tree_dark_leaves_g = 100; toxic_swamp.tree_dark_leaves_b = 20;
        toxic_swamp.moon_glow_r = 250; toxic_swamp.moon_glow_g = 255; toxic_swamp.moon_glow_b = 200;
        toxic_swamp.path_color_r = 120; toxic_swamp.path_color_g = 140; toxic_swamp.path_color_b = 80;
        toxic_swamp.bat_color_r = 100; toxic_swamp.bat_color_g = 120; toxic_swamp.bat_color_b = 60;
        toxic_swamp.name = "Toxic Swamp";
        themes.push_back(toxic_swamp);
        
        // RED theme (inspired by arcade racer RED theme)
        Theme red_world;
        red_world.sky_top_r = 60; red_world.sky_top_g = 10; red_world.sky_top_b = 10;
        red_world.sky_bottom_r = 30; red_world.sky_bottom_g = 5; red_world.sky_bottom_b = 5;
        red_world.dark_land_r = 139; red_world.dark_land_g = 0; red_world.dark_land_b = 0;      // Dark red grass
        red_world.lighter_land_r = 178; red_world.lighter_land_g = 34; red_world.lighter_land_b = 34;  // Fire brick grass
        red_world.tree_trunk_r = 139; red_world.tree_trunk_g = 69; red_world.tree_trunk_b = 19;  // Saddle brown
        red_world.tree_dark_r = 100; red_world.tree_dark_g = 50; red_world.tree_dark_b = 15;
        red_world.tree_leaves_r = 205; red_world.tree_leaves_g = 92; red_world.tree_leaves_b = 92;  // Indian red leaves
        red_world.tree_dark_leaves_r = 139; red_world.tree_dark_leaves_g = 69; red_world.tree_dark_leaves_b = 19;
        red_world.moon_glow_r = 255; red_world.moon_glow_g = 0; red_world.moon_glow_b = 0;      // Red moon
        red_world.path_color_r = 139; red_world.path_color_g = 69; red_world.path_color_b = 19;  // Brown path
        red_world.bat_color_r = 255; red_world.bat_color_g = 99; red_world.bat_color_b = 71;   // Tomato colored bats
        red_world.name = "Red World";
        themes.push_back(red_world);
        
        // VICE theme (inspired by arcade racer VICE theme)
        Theme vice_city;
        vice_city.sky_top_r = 51; vice_city.sky_top_g = 51; vice_city.sky_top_b = 68;
        vice_city.sky_bottom_r = 25; vice_city.sky_bottom_g = 25; vice_city.sky_bottom_b = 35;
        vice_city.dark_land_r = 255; vice_city.dark_land_g = 20; vice_city.dark_land_b = 147;   // Deep pink grass
        vice_city.lighter_land_r = 255; vice_city.lighter_land_g = 0; vice_city.lighter_land_b = 255;  // Magenta grass
        vice_city.tree_trunk_r = 255; vice_city.tree_trunk_g = 20; vice_city.tree_trunk_b = 147; // Deep pink trunks
        vice_city.tree_dark_r = 180; vice_city.tree_dark_g = 15; vice_city.tree_dark_b = 100;
        vice_city.tree_leaves_r = 75; vice_city.tree_leaves_g = 0; vice_city.tree_leaves_b = 130;  // Indigo leaves
        vice_city.tree_dark_leaves_r = 50; vice_city.tree_dark_leaves_g = 0; vice_city.tree_dark_leaves_b = 80;
        vice_city.moon_glow_r = 255; vice_city.moon_glow_g = 255; vice_city.moon_glow_b = 255;    // Yellow moon
        vice_city.path_color_r = 51; vice_city.path_color_g = 51; vice_city.path_color_b = 68;   // Dark blue path
        vice_city.bat_color_r = 0; vice_city.bat_color_g = 255; vice_city.bat_color_b = 255;    // Cyan bats
        vice_city.name = "Vice City";
        themes.push_back(vice_city);
        
        // OCEAN theme (inspired by arcade racer OCEAN theme)
        Theme ocean_depths;
        ocean_depths.sky_top_r = 0; ocean_depths.sky_top_g = 30; ocean_depths.sky_top_b = 50;
        ocean_depths.sky_bottom_r = 0; ocean_depths.sky_bottom_g = 15; ocean_depths.sky_bottom_b = 25;
        ocean_depths.dark_land_r = 0; ocean_depths.dark_land_g = 100; ocean_depths.dark_land_b = 100;  // Dark teal grass
        ocean_depths.lighter_land_r = 0; ocean_depths.lighter_land_g = 150; ocean_depths.lighter_land_b = 150; // Medium teal grass
        ocean_depths.tree_trunk_r = 0; ocean_depths.tree_trunk_g = 100; ocean_depths.tree_trunk_b = 100; // Dark teal trunks
        ocean_depths.tree_dark_r = 0; ocean_depths.tree_dark_g = 70; ocean_depths.tree_dark_b = 70;
        ocean_depths.tree_leaves_r = 0; ocean_depths.tree_leaves_g = 150; ocean_depths.tree_leaves_b = 150; // Medium teal leaves
        ocean_depths.tree_dark_leaves_r = 0; ocean_depths.tree_dark_leaves_g = 120; ocean_depths.tree_dark_leaves_b = 120;
        ocean_depths.moon_glow_r = 100; ocean_depths.moon_glow_g = 255; ocean_depths.moon_glow_b = 255; // Light aqua moon
        ocean_depths.path_color_r = 0; ocean_depths.path_color_g = 80; ocean_depths.path_color_b = 80;   // Dark teal path
        ocean_depths.bat_color_r = 0; ocean_depths.bat_color_g = 200; ocean_depths.bat_color_b = 200;  // Aqua bats
        ocean_depths.name = "Ocean Depths";
        themes.push_back(ocean_depths);
        
        // NEON theme (inspired by arcade racer NEON theme)
        Theme neon_lights;
        neon_lights.sky_top_r = 30; neon_lights.sky_top_g = 0; neon_lights.sky_top_b = 50;
        neon_lights.sky_bottom_r = 15; neon_lights.sky_bottom_g = 0; neon_lights.sky_bottom_b = 25;
        neon_lights.dark_land_r = 255; neon_lights.dark_land_g = 0; neon_lights.dark_land_b = 255;  // Magenta grass
        neon_lights.lighter_land_r = 75; neon_lights.lighter_land_g = 0; neon_lights.lighter_land_b = 130;  // Indigo grass
        neon_lights.tree_trunk_r = 255; neon_lights.tree_trunk_g = 0; neon_lights.tree_trunk_b = 255; // Magenta trunks
        neon_lights.tree_dark_r = 180; neon_lights.tree_dark_g = 0; neon_lights.tree_dark_b = 180;
        neon_lights.tree_leaves_r = 75; neon_lights.tree_leaves_g = 0; neon_lights.tree_leaves_b = 130;  // Indigo leaves
        neon_lights.tree_dark_leaves_r = 50; neon_lights.tree_dark_leaves_g = 0; neon_lights.tree_dark_leaves_b = 100;
        neon_lights.moon_glow_r = 0; neon_lights.moon_glow_g = 255; neon_lights.moon_glow_b = 0;     // Neon green moon
        neon_lights.path_color_r = 50; neon_lights.path_color_g = 0; neon_lights.path_color_b = 100;  // Dark purple path
        neon_lights.bat_color_r = 255; neon_lights.bat_color_g = 0; neon_lights.bat_color_b = 255;   // Magenta bats
        neon_lights.name = "Neon Lights";
        themes.push_back(neon_lights);
        
        // DARK SPOOKY theme - black trees, purple grass, contrasting sky
        Theme dark_nightmare;
        dark_nightmare.sky_top_r = 70; dark_nightmare.sky_top_g = 50; dark_nightmare.sky_top_b = 120;    // Dark purple-blue
        dark_nightmare.sky_bottom_r = 40; dark_nightmare.sky_bottom_g = 30; dark_nightmare.sky_bottom_b = 80;  // Darker purple-blue
        dark_nightmare.dark_land_r = 60; dark_nightmare.dark_land_g = 20; dark_nightmare.dark_land_b = 80;     // Dark purple grass
        dark_nightmare.lighter_land_r = 90; dark_nightmare.lighter_land_g = 40; dark_nightmare.lighter_land_b = 120;  // Purple grass
        dark_nightmare.tree_trunk_r = 10; dark_nightmare.tree_trunk_g = 10; dark_nightmare.tree_trunk_b = 10;   // Nearly black trunks
        dark_nightmare.tree_dark_r = 5; dark_nightmare.tree_dark_g = 5; dark_nightmare.tree_dark_b = 5;         // Pure black branches
        dark_nightmare.tree_leaves_r = 100; dark_nightmare.tree_leaves_g = 40; dark_nightmare.tree_leaves_b = 120;  // Dark grey leaves
        dark_nightmare.tree_dark_leaves_r = 120; dark_nightmare.tree_dark_leaves_g = 20; dark_nightmare.tree_dark_leaves_b = 125;  // Darker grey leaves
        dark_nightmare.moon_glow_r = 255; dark_nightmare.moon_glow_g = 255; dark_nightmare.moon_glow_b = 255;   // Pale purple moon
        dark_nightmare.path_color_r = 30; dark_nightmare.path_color_g = 25; dark_nightmare.path_color_b = 40;   // Dark grey path
        dark_nightmare.bat_color_r = 80; dark_nightmare.bat_color_g = 60; dark_nightmare.bat_color_b = 120;    // Purple-tinted bats
        dark_nightmare.name = "Dark Nightmare";
        themes.push_back(dark_nightmare);
        
        // GOTHIC MIST theme - another spooky variant with cool tones
        Theme gothic_mist;
        gothic_mist.sky_top_r = 30; gothic_mist.sky_top_g = 40; gothic_mist.sky_top_b = 60;      // Dark steel blue
        gothic_mist.sky_bottom_r = 15; gothic_mist.sky_bottom_g = 20; gothic_mist.sky_bottom_b = 30;  // Darker steel blue
        gothic_mist.dark_land_r = 20; gothic_mist.dark_land_g = 30; gothic_mist.dark_land_b = 20;     // Dark mossy green
        gothic_mist.lighter_land_r = 35; gothic_mist.lighter_land_g = 50; gothic_mist.lighter_land_b = 35;  // Mossy green
        gothic_mist.tree_trunk_r = 60; gothic_mist.tree_trunk_g = 50; gothic_mist.tree_trunk_b = 40;   // Dark weathered brown
        gothic_mist.tree_dark_r = 30; gothic_mist.tree_dark_g = 25; gothic_mist.tree_dark_b = 20;     // Very dark brown
        gothic_mist.tree_leaves_r = 40; gothic_mist.tree_leaves_g = 60; gothic_mist.tree_leaves_b = 40;  // Sickly green leaves
        gothic_mist.tree_dark_leaves_r = 25; gothic_mist.tree_dark_leaves_g = 40; gothic_mist.tree_dark_leaves_b = 25;  // Dark sickly green
        gothic_mist.moon_glow_r = 220; gothic_mist.moon_glow_g = 220; gothic_mist.moon_glow_b = 180;   // Pale sickly yellow moon
        gothic_mist.path_color_r = 50; gothic_mist.path_color_g = 45; gothic_mist.path_color_b = 40;   // Dark muddy path
        gothic_mist.bat_color_r = 60; gothic_mist.bat_color_g = 70; gothic_mist.bat_color_b = 60;     // Murky green bats
        gothic_mist.name = "Gothic Mist";
        themes.push_back(gothic_mist);
        
        current_theme = themes[1];
    }
    
    std::vector<int> parseRGB(const std::string& line) {
        std::vector<int> rgb = {0, 0, 0};
        size_t start = line.find('[');
        size_t end = line.find(']');
        if (start != std::string::npos && end != std::string::npos) {
            std::string values = line.substr(start + 1, end - start - 1);
            std::stringstream ss(values);
            std::string token;
            int index = 0;
            while (getline(ss, token, ',') && index < 3) {
                rgb[index] = std::stoi(token);
                index++;
            }
        }
        return rgb;
    }
    
    void initializeBoids() {
        boids.clear();
        for (int i = 0; i < MAX_BATS; i++) {
            // Spread boids across expanded area
            float x = -5 + rand() % 42; // -5 to 37
            float y = -2 + rand() % 12; // -2 to 10
            boids.emplace_back(x, y);
        }
    }
    
    void updateBoids() {
        for (auto& boid : boids) {
            // Calculate forces
            auto sep = separate(boid);
            auto ali = align(boid);
            auto coh = cohesion(boid);
            auto bounds = boundaryForce(boid);
            
            // Apply forces with different weights based on spreading mode
            if (spreading_mode) {
                // In spreading mode: stronger separation, weaker cohesion/alignment
                boid.vx += sep.first * 3.0f + ali.first * 0.2f + coh.first * 0.1f + bounds.first;
                boid.vy += sep.second * 3.0f + ali.second * 0.2f + coh.second * 0.1f + bounds.second;
            } else {
                // In flocking mode: normal behavior
                boid.vx += sep.first + ali.first + coh.first + bounds.first;
                boid.vy += sep.second + ali.second + coh.second + bounds.second;
            }
            
            // Limit speed
            float speed = sqrt(boid.vx * boid.vx + boid.vy * boid.vy);
            if (speed > boid.max_speed) {
                boid.vx = (boid.vx / speed) * boid.max_speed;
                boid.vy = (boid.vy / speed) * boid.max_speed;
            }
            
            // Update position
            boid.x += boid.vx;
            boid.y += boid.vy;
            
            // Update wing animation
            boid.wing_phase += 0.2f;
        }
    }
    
    std::pair<float, float> separate(const Boid& boid) {
        float desired_separation = 4.0f;
        float steer_x = 0, steer_y = 0;
        int count = 0;
        
        // Add occasional stronger repulsion to help spread out clustered boids
        float repulsion_chance = 0.02f; // 2% chance per frame
        float repulsion_multiplier = (rand() % 100) < (repulsion_chance * 100) ? 3.0f : 1.0f;
        
        for (const auto& other : boids) {
            float dx = boid.x - other.x;
            float dy = boid.y - other.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            // Use larger separation distance when applying repulsion
            float current_separation = desired_separation * repulsion_multiplier;
            
            if (distance > 0 && distance < current_separation) {
                // Calculate steering force away from neighbor
                float norm_x = dx / distance;
                float norm_y = dy / distance;
                norm_x /= distance; // Weight by distance
                norm_y /= distance;
                steer_x += norm_x * repulsion_multiplier;
                steer_y += norm_y * repulsion_multiplier;
                count++;
            }
        }
        
        if (count > 0) {
            steer_x /= count;
            steer_y /= count;
            
            // Normalize and scale
            float mag = sqrt(steer_x * steer_x + steer_y * steer_y);
            if (mag > 0) {
                steer_x = (steer_x / mag) * boid.max_force * repulsion_multiplier;
                steer_y = (steer_y / mag) * boid.max_force * repulsion_multiplier;
            }
        }
        
        return {steer_x, steer_y};
    }
    
    std::pair<float, float> align(const Boid& boid) {
        float neighbor_radius = 8.0f;
        float sum_vx = 0, sum_vy = 0;
        int count = 0;
        
        for (const auto& other : boids) {
            float dx = boid.x - other.x;
            float dy = boid.y - other.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance > 0 && distance < neighbor_radius) {
                sum_vx += other.vx;
                sum_vy += other.vy;
                count++;
            }
        }
        
        if (count > 0) {
            sum_vx /= count;
            sum_vy /= count;
            
            // Normalize and scale
            float mag = sqrt(sum_vx * sum_vx + sum_vy * sum_vy);
            if (mag > 0) {
                sum_vx = (sum_vx / mag) * boid.max_force;
                sum_vy = (sum_vy / mag) * boid.max_force;
            }
        }
        
        return {sum_vx, sum_vy};
    }
    
    std::pair<float, float> cohesion(const Boid& boid) {
        float neighbor_radius = 8.0f;
        float sum_x = 0, sum_y = 0;
        int count = 0;
        
        for (const auto& other : boids) {
            float dx = boid.x - other.x;
            float dy = boid.y - other.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance > 0 && distance < neighbor_radius) {
                sum_x += other.x;
                sum_y += other.y;
                count++;
            }
        }
        
        if (count > 0) {
            sum_x /= count;
            sum_y /= count;
            
            // Seek towards center
            float seek_x = sum_x - boid.x;
            float seek_y = sum_y - boid.y;
            
            // Normalize and scale
            float mag = sqrt(seek_x * seek_x + seek_y * seek_y);
            if (mag > 0) {
                seek_x = (seek_x / mag) * boid.max_force;
                seek_y = (seek_y / mag) * boid.max_force;
            }
            
            return {seek_x, seek_y};
        }
        
        return {0, 0};
    }
    
    std::pair<float, float> boundaryForce(const Boid& boid) {
        float boundary_distance = 8.0f;
        float force_x = 0, force_y = 0;
        
        // Expanded flight area: -10 to 42 horizontally, -5 to 12 vertically
        float left_boundary = -10.0f;
        float right_boundary = 42.0f;
        float top_boundary = -5.0f;
        float bottom_boundary = 12.0f; // Keep above horizon
        
        // Apply boundary forces when approaching edges
        if (boid.x < left_boundary + boundary_distance) {
            force_x += (left_boundary + boundary_distance - boid.x) * 0.05f;
        }
        
        if (boid.x > right_boundary - boundary_distance) {
            force_x -= (boid.x - (right_boundary - boundary_distance)) * 0.05f;
        }
        
        if (boid.y < top_boundary + boundary_distance) {
            force_y += (top_boundary + boundary_distance - boid.y) * 0.05f;
        }
        
        if (boid.y > bottom_boundary - boundary_distance) {
            force_y -= (boid.y - (bottom_boundary - boundary_distance)) * 0.1f;
        }
        
        return {force_x, force_y};
    }
    
    void drawGradientSky(PicoGraphics* graphics) {
        const int horizon_y = 14;
        
        // Draw gradient sky from top to horizon
        for (int y = 0; y < horizon_y; y++) {
            float gradient_factor = (float)y / (float)horizon_y;
            
            // Interpolate between sky_top and sky_bottom colors
            uint8_t r = current_theme.sky_top_r + (current_theme.sky_bottom_r - current_theme.sky_top_r) * gradient_factor;
            uint8_t g = current_theme.sky_top_g + (current_theme.sky_bottom_g - current_theme.sky_top_g) * gradient_factor;
            uint8_t b = current_theme.sky_top_b + (current_theme.sky_bottom_b - current_theme.sky_top_b) * gradient_factor;
            
            uint32_t gradient_color = graphics->create_pen(r, g, b);
            graphics->set_pen(gradient_color);
            
            for (int x = 0; x < 32; x++) {
                graphics->pixel(Point(x, y));
            }
        }
    }
    
    void drawBats(PicoGraphics* graphics) {
        uint32_t bat_color = graphics->create_pen(current_theme.bat_color_r, current_theme.bat_color_g, current_theme.bat_color_b);
        graphics->set_pen(bat_color);
        
        for (const auto& boid : boids) {
            int bat_x = (int)boid.x;
            int bat_y = (int)boid.y;
            
            // Only draw bats that are visible on screen (with small margin for wing animation)
            if (bat_x >= -1 && bat_x <= 32 && bat_y >= -1 && bat_y <= 32) {
                // Draw bat body (only if body is on screen)
                if (bat_x >= 0 && bat_x < 32 && bat_y >= 0 && bat_y < 32) {
                    graphics->pixel(Point(bat_x, bat_y));
                }
                
                // Draw animated wings (check bounds for each wing pixel)
                bool wing_up = sin(boid.wing_phase) > 0;
                if (wing_up) {
                    // Wings up position
                    if (bat_x - 1 >= 0 && bat_x - 1 < 32 && bat_y - 1 >= 0 && bat_y - 1 < 32) {
                        graphics->pixel(Point(bat_x - 1, bat_y - 1));
                    }
                    if (bat_x + 1 >= 0 && bat_x + 1 < 32 && bat_y - 1 >= 0 && bat_y - 1 < 32) {
                        graphics->pixel(Point(bat_x + 1, bat_y - 1));
                    }
                } else {
                    // Wings down position
                    if (bat_x - 1 >= 0 && bat_x - 1 < 32 && bat_y >= 0 && bat_y < 32) {
                        graphics->pixel(Point(bat_x - 1, bat_y));
                    }
                    if (bat_x + 1 >= 0 && bat_x + 1 < 32 && bat_y >= 0 && bat_y < 32) {
                        graphics->pixel(Point(bat_x + 1, bat_y));
                    }
                }
            }
        }
    }
    void generateInitialTrees() {
        trees.clear();
        
        for (int i = 0; i < MAX_TREES; i++) {
            Tree tree;
            tree.roadY = 0.1f + (rand() % 100) * 0.01f; // Start at horizon (0.1-1.0)
            tree.trackPosition = ((rand() % 2) ? -1.0f : 1.0f) * (1.2f + (rand() % 50) * 0.02f); // Left or right side
            tree.base_angle = M_PI_2 + (rand() % 60 - 30) * M_PI / 180.0f; // Slight angle variation
            tree.size_multiplier = 0.5f + (rand() % 150) * 0.01f; // Random size from 0.5x to 2.0x
            tree.active = true;
            tree.nodes.clear();
            tree.nodes.reserve(MAX_TREE_NODES / MAX_TREES);
            
            generateTreeBranches(tree);
            trees.push_back(tree);
        }
    }
    
    void respawnTree(Tree& tree) {
        tree.roadY = 0.05f + (rand() % 20) * 0.01f; // Respawn at horizon
        tree.trackPosition = ((rand() % 2) ? -1.0f : 1.0f) * (1.2f + (rand() % 50) * 0.02f);
        tree.base_angle = M_PI_2 + (rand() % 60 - 30) * M_PI / 180.0f;
        tree.size_multiplier = 0.5f + (rand() % 150) * 0.01f; // New random size
        tree.nodes.clear();
    }
    
    void spawnNewTree() {
        for (auto& tree : trees) {
            if (!tree.active) {
                tree.roadY = 0.05f + (rand() % 20) * 0.01f;
                tree.trackPosition = ((rand() % 2) ? -1.0f : 1.0f) * (1.2f + (rand() % 50) * 0.02f);
                tree.base_angle = M_PI_2 + (rand() % 60 - 30) * M_PI / 180.0f;
                tree.size_multiplier = 0.5f + (rand() % 150) * 0.01f; // New random size
                tree.active = true;
                tree.nodes.clear();
                break;
            }
        }
    }
    
    void generateTreeBranches(Tree& tree) {
        tree.nodes.clear();
        
        // Calculate perspective based on distance from viewer
        float perspective = tree.roadY;
        if (perspective > 1.0f) perspective = 1.0f;
        if (perspective < 0.05f) return; // Too far to see
        
        // Calculate screen position using arcade racer style with proper horizon
        const int horizon_y = 14;
        float middlepoint = 0.5f + (road_curve / 10.0f) * pow(1 - perspective, 3);
        int screen_x = (int)(32 * (middlepoint + tree.trackPosition * 0.3f * perspective));
        int screen_y = horizon_y + (int)((32 - horizon_y) * perspective);
        
        // Scale and complexity based on distance and tree size multiplier
        float base_scale = 0.4f + 1.8f * perspective;
        float scale = base_scale * tree.size_multiplier;
        
        // Adjust max depth based on tree size - larger trees get more branches
        int base_max_depth = (perspective > 0.3f) ? 5 : (perspective > 0.15f) ? 3 : 2;
        int max_depth = base_max_depth;
        if (tree.size_multiplier > 1.5f) {
            max_depth += 1; // Large trees get extra branch depth
        } else if (tree.size_multiplier < 0.8f) {
            max_depth = std::max(1, max_depth - 1); // Small trees get less depth
        }
        
        // Generate tree recursively with size variation and subtle time-based growth
        float time_variation = 0.9f + 0.2f * sin(distance * 0.05f + tree.trackPosition * 3.14f); // Gentle 10% size fluctuation
        float trunk_length = 6.0f * scale * time_variation;
        addBranch(tree, screen_x, screen_y, tree.base_angle - ((scale * time_variation) * 0.05f), trunk_length, 0, max_depth, scale);
    }
    
    void addBranch(Tree& tree, float x, float y, float angle, float length, int depth, int max_depth, float scale) {
        if (depth >= max_depth || length < 0.5f || tree.nodes.size() >= MAX_TREE_NODES / MAX_TREES) {
            return;
        }
        
        float end_x = x + cos(angle) * length;
        float end_y = y - sin(angle) * length;
        
        // Add this branch to the tree
        TreeNode node;
        node.x = x;
        node.y = y;
        node.angle = angle;
        node.length = length;
        node.depth = depth;
        node.visible = (x >= 0 && x < 32 && y >= 0 && y < 32 && 
                       end_x >= 0 && end_x < 32 && end_y >= 0 && end_y < 32);
        tree.nodes.push_back(node);
        
        // Dan Shiffman's algorithm: branch angle varies with animation and user adjustment
        float base_branch_angle = 25.0f + tree_angle_offset;
        float branch_angle = base_branch_angle + sin(animation_phase + depth) * 10.0f;
        float new_length = length * 0.66f; // Classic 66% reduction
        
        // Create two branches
        addBranch(tree, end_x, end_y, angle + branch_angle * M_PI / 180.0f, 
                 new_length, depth + 1, max_depth, scale);
        addBranch(tree, end_x, end_y, angle - branch_angle * M_PI / 180.0f, 
                 new_length, depth + 1, max_depth, scale);
    }
    
    void drawLandscape(PicoGraphics* graphics) {
        const int horizon_y = 14;
        
        // Draw landscape with moving stripes similar to arcade racer
        for (int y = horizon_y; y < 32; y++) {
            // Calculate perspective: 0 at horizon, 1 at bottom of screen
            float perspective = (float)(y - horizon_y) / (32 - horizon_y);
            if (perspective > 1.0f) perspective = 1.0f;
            
            // Calculate road curvature for landscape positioning
            float middlepoint = 0.5f + (road_curve / 10.0f) * pow(1 - perspective, 3);
            float roadWidth = PATH_WIDTH * (0.1f + 0.9f * perspective) / 32.0f;
            
            int center_x = (int)(32 * middlepoint);
            int road_left = center_x - (int)(32 * roadWidth / 2);
            int road_right = center_x + (int)(32 * roadWidth / 2);
            
            // Draw landscape stripes using arcade racer formula with enhanced movement
            float grass_frequency = 15.0f * pow(1.0f - perspective, 3);
            float grass_movement = distance * 0.3f * (1.0f + current_speed * 0.5f); // Much more responsive movement
            bool use_light_stripe = sin(grass_frequency + grass_movement) > 0;
            
            // Apply distance-based brightness to grass colors
            uint32_t land_color;
            if (use_light_stripe) {
                land_color = createDarkenedPen(graphics, current_theme.lighter_land_r, current_theme.lighter_land_g, current_theme.lighter_land_b, perspective);
            } else {
                land_color = createDarkenedPen(graphics, current_theme.dark_land_r, current_theme.dark_land_g, current_theme.dark_land_b, perspective);
            }
            graphics->set_pen(land_color);
            
            // Draw left side of landscape
            for (int x = 0; x < road_left; x++) {
                graphics->pixel(Point(x, y));
            }
            
            // Draw right side of landscape  
            for (int x = road_right + 1; x < 32; x++) {
                graphics->pixel(Point(x, y));
            }
        }
    }
    
    void drawPath(PicoGraphics* graphics) {
        const int horizon_y = 14; // Horizon line - road starts here
        
        // Draw road using arcade racer perspective, starting from horizon
        for (int y = horizon_y; y < 32; y++) {
            // Calculate perspective: 0 at horizon, 1 at bottom of screen
            float perspective = (float)(y - horizon_y) / (32 - horizon_y);
            if (perspective > 1.0f) perspective = 1.0f;
            
            // Apply distance-based brightness to road color
            uint32_t path_color = createDarkenedPen(graphics, current_theme.path_color_r, current_theme.path_color_g, current_theme.path_color_b, perspective);
            graphics->set_pen(path_color);
            
            // Calculate road curvature and position
            float middlepoint = 0.5f + (road_curve / 10.0f) * pow(1 - perspective, 3);
            float roadWidth = PATH_WIDTH * (0.1f + 0.9f * perspective) / 32.0f;
            
            int center_x = (int)(32 * middlepoint);
            int left = center_x - (int)(32 * roadWidth / 2);
            int right = center_x + (int)(32 * roadWidth / 2);
            
            // Draw road surface
            for (int x = left; x <= right; x++) {
                if (x >= 0 && x < 32) {
                    graphics->pixel(Point(x, y));
                }
            }
        }
    }
    
    void drawTrees(PicoGraphics* graphics) {
        for (const auto& tree : trees) {
            if (!tree.active) continue;
            
            // Calculate perspective for this tree based on its distance
            float perspective = tree.roadY;
            if (perspective > 1.0f) perspective = 1.0f;
            
            // Check if this tree should be affected by lightning flash
            bool should_flash = tree_flash_active;
            float flash_intensity = 0.0f;
            if (tree_flash_active) {
                // For now, make all trees flash when lightning strikes (for debugging)
                flash_intensity = tree_flash_timer / TREE_FLASH_DURATION;
                if (flash_intensity > 1.0f) flash_intensity = 1.0f;
            }
            
            for (const auto& node : tree.nodes) {
                if (!node.visible) continue;
                
                float end_x = node.x + cos(node.angle) * node.length;
                float end_y = node.y - sin(node.angle) * node.length;
                
                // Choose branch color based on depth with distance-based brightness
                uint32_t branch_color;
                uint8_t base_r, base_g, base_b;
                
                // Get the base colors first
                if (node.depth < 2) {
                    base_r = current_theme.tree_trunk_r; base_g = current_theme.tree_trunk_g; base_b = current_theme.tree_trunk_b;
                } else if (node.depth < 3) {
                    base_r = current_theme.tree_dark_r; base_g = current_theme.tree_dark_g; base_b = current_theme.tree_dark_b;
                } else if (node.depth < 4) {
                    base_r = current_theme.tree_leaves_r; base_g = current_theme.tree_leaves_g; base_b = current_theme.tree_leaves_b;
                } else {
                    base_r = current_theme.tree_dark_leaves_r; base_g = current_theme.tree_dark_leaves_g; base_b = current_theme.tree_dark_leaves_b;
                }
                
                if (should_flash && flash_intensity > 0.01f) {
                    // Brighten existing colors towards white with moderate intensity
                    float brighten_factor = flash_intensity * 0.6f; // Cap at 60% brightening
                    uint8_t bright_r = (uint8_t)(base_r + (255 - base_r) * brighten_factor);
                    uint8_t bright_g = (uint8_t)(base_g + (255 - base_g) * brighten_factor);
                    uint8_t bright_b = (uint8_t)(base_b + (255 - base_b) * brighten_factor);
                    
                    // Apply perspective darkening to the brightened color
                    float brightness = 0.2f + 0.8f * perspective;
                    int final_r = (int)(bright_r * brightness);
                    int final_g = (int)(bright_g * brightness);
                    int final_b = (int)(bright_b * brightness);
                    
                    branch_color = graphics->create_pen(final_r, final_g, final_b);
                } else {
                    // Normal tree colors with perspective darkening
                    branch_color = createDarkenedPen(graphics, base_r, base_g, base_b, perspective);
                }
                
                graphics->set_pen(branch_color);
                drawLine(graphics, node.x, node.y, end_x, end_y);
                
                // Add small leaves on outer branches with distance-based brightness
                if (node.depth >= 3 && (rand() % 4) == 0) {
                    uint32_t leaf_color;
                    if (should_flash && flash_intensity > 0.01f) {
                        // Brighten leaf colors towards white
                        float brighten_factor = flash_intensity * 0.6f;
                        uint8_t bright_r = (uint8_t)(current_theme.tree_dark_leaves_r + (255 - current_theme.tree_dark_leaves_r) * brighten_factor);
                        uint8_t bright_g = (uint8_t)(current_theme.tree_dark_leaves_g + (255 - current_theme.tree_dark_leaves_g) * brighten_factor);
                        uint8_t bright_b = (uint8_t)(current_theme.tree_dark_leaves_b + (255 - current_theme.tree_dark_leaves_b) * brighten_factor);
                        
                        // Apply perspective darkening
                        float brightness = 0.2f + 0.8f * perspective;
                        int final_r = (int)(bright_r * brightness);
                        int final_g = (int)(bright_g * brightness);
                        int final_b = (int)(bright_b * brightness);
                        
                        leaf_color = graphics->create_pen(final_r, final_g, final_b);
                    } else {
                        leaf_color = createDarkenedPen(graphics, current_theme.tree_dark_leaves_r, current_theme.tree_dark_leaves_g, current_theme.tree_dark_leaves_b, perspective);
                    }
                    graphics->set_pen(leaf_color);
                    graphics->pixel(Point((int)end_x, (int)end_y));
                }
            }
        }
    }
    
    void drawMoon(PicoGraphics* graphics) {
        float moon_x = 26.0f;
        float moon_y = 5.0f;
        float moon_radius = 2.5f;
        
        uint32_t moon_color = graphics->create_pen(current_theme.moon_glow_r, current_theme.moon_glow_g, current_theme.moon_glow_b);
        graphics->set_pen(moon_color);
        
        // Draw crescent moon
        for (int y = (int)(moon_y - moon_radius); y <= (int)(moon_y + moon_radius); y++) {
            for (int x = (int)(moon_x - moon_radius); x <= (int)(moon_x + moon_radius); x++) {
                if (x >= 0 && x < 32 && y >= 0 && y < 32) {
                    float dx = x - moon_x;
                    float dy = y - moon_y;
                    float dist = sqrt(dx * dx + dy * dy);
                    
                    if (dist <= moon_radius) {
                        // Create crescent by excluding a circle offset to the right
                        float crescent_x = moon_x + 1.2f;
                        float crescent_dx = x - crescent_x;
                        float crescent_dist = sqrt(crescent_dx * crescent_dx + dy * dy);
                        
                        if (crescent_dist > moon_radius - 0.3f) {
                            graphics->pixel(Point(x, y));
                        }
                    }
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
    
    void updateSpeedState() {
        // Random durations for each state to make movement feel natural
        float state_duration = 0.0f;
        SpeedState next_state = current_speed_state;
        
        switch (current_speed_state) {
            case STOPPED:
                state_duration = 3.0f + (rand() % 200) / 100.0f;  // 3+ seconds stopped
                if (speed_timer >= state_duration) {
                    // After stopping, randomly choose walking or running
                    next_state = (rand() % 3 == 0) ? RUNNING : WALKING;
                    speed_timer = 0.0f;
                }
                break;
                
            case WALKING:
                state_duration = 3.0f + (rand() % 400) / 100.0f;  // 3-7 seconds walking
                if (speed_timer >= state_duration) {
                    // From walking, can stop or run
                    int choice = rand() % 4;
                    if (choice == 0) {
                        next_state = STOPPED;
                    } else if (choice == 1) {
                        next_state = RUNNING;
                    }
                    // else stay walking (50% chance)
                    speed_timer = 0.0f;
                }
                break;
                
            case RUNNING:
                state_duration = 2.0f + (rand() % 300) / 100.0f;  // 2-5 seconds running
                if (speed_timer >= state_duration) {
                    // After running, usually slow down to walking or stop to rest
                    next_state = (rand() % 3 == 0) ? STOPPED : WALKING;
                    speed_timer = 0.0f;
                }
                break;
        }
        
        // Update speed if state changed
        if (next_state != current_speed_state) {
            current_speed_state = next_state;
            switch (current_speed_state) {
                case STOPPED:
                    current_speed = STOP_SPEED;
                    break;
                case WALKING:
                    current_speed = WALK_SPEED;
                    break;
                case RUNNING:
                    current_speed = RUN_SPEED;
                    break;
            }
        }
    }
    
    void updateEyesSystem(float dt) {
        // Handle eyes visibility based on speed state
        if (current_speed_state == STOPPED) {
            if (!eyes_visible) {
                // Chance to show eyes when we first stop
                if ((rand() % 100) < (EYES_APPEAR_CHANCE * 100)) {
                    eyes_visible = true;
                    eyes_timer = 0.0f;
                    generateSpookyEyes();
                }
            } else {
                // Eyes are visible, update timer
                eyes_timer += dt;
                if (eyes_timer >= EYES_DISPLAY_TIME) {
                    eyes_visible = false;
                    tree_eyes.clear();
                }
            }
        } else {
            // Moving, hide eyes if they were visible
            if (eyes_visible) {
                eyes_visible = false;
                tree_eyes.clear();
            }
        }
        
        // Update eyes animation if visible
        if (eyes_visible) {
            tree_eyes.update();
        }
    }
    
    void generateSpookyEyes() {
        tree_eyes.clear();
        
        // Choose left or right side of screen
        bool on_left_side = (rand() % 2) == 0;
        
        // Position eyes in tree area on chosen side
        float eye_x, eye_y;
        if (on_left_side) {
            eye_x = 2 + (rand() % 6);  // Left side (2-8)
        } else {
            eye_x = 24 + (rand() % 6); // Right side (24-30)
        }
        eye_y = 8 + (rand() % 8);  // Middle height (8-16)
        
        // Create spooky POINT eyes that fade from leaf color to red
        AnimatedEye::EyeConfig left_eye;
        left_eye.x = eye_x - 1.5f;
        left_eye.y = eye_y;
        // Use leaf color from current theme as starting color
        left_eye.r = current_theme.tree_leaves_r;
        left_eye.g = current_theme.tree_leaves_g;
        left_eye.b = current_theme.tree_leaves_b;
        left_eye.radiusX = 1.0f;
        left_eye.radiusY = 0.8f;
        left_eye.type = AnimatedEye::POINT;  // POINT eyes for subtle spook
        left_eye.is_triangle = false;  // Set for backwards compatibility
        left_eye.glow_intensity = 0.8f;
        
        AnimatedEye::EyeConfig right_eye;
        right_eye.x = eye_x + 1.5f;
        right_eye.y = eye_y;
        // Use leaf color from current theme as starting color
        right_eye.r = current_theme.tree_leaves_r;
        right_eye.g = current_theme.tree_leaves_g;
        right_eye.b = current_theme.tree_leaves_b;
        right_eye.radiusX = 1.0f;
        right_eye.radiusY = 0.8f;
        right_eye.type = AnimatedEye::POINT;  // POINT eyes for subtle spook
        right_eye.is_triangle = false;  // Set for backwards compatibility
        right_eye.glow_intensity = 0.8f;
        
        tree_eyes.addEyePair(left_eye, right_eye);
        tree_eyes.disableRepositioning(); // Keep eyes in one spot
    }
    
    void drawEyes(PicoGraphics* graphics) {
        if (eyes_visible) {
            tree_eyes.draw(animation_phase);
        }
    }
};
