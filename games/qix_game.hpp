#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <array>

#include "pico/stdlib.h"
#include "../game_base.hpp"

using namespace pimoroni;

const int QIX_FIELD_WIDTH = 30;
const int QIX_FIELD_HEIGHT = 30;
const int QIX_FIELD_OFFSET_X = 1;
const int QIX_FIELD_OFFSET_Y = 1;

enum class CellType {
    EMPTY,
    WALL,
    TRAIL,
    CLAIMED
};

enum class EnemyType {
    BUTTERFLY,
    TURTLE,
    SPIRAL,
    STAR,
    DIAMOND,
    JELLYFISH
};

struct QixSegment {
    float x, y;
    float age;
    uint8_t r, g, b;
    float alpha;
};

struct QixEnemy {
    float x, y;
    float dx, dy;
    float speed;
    EnemyType type;
    float animation_phase;
    float color_phase;
    int shape_variant;
    float size_pulse;
    float morph_phase;
    float intensity_pulse;
    std::vector<QixSegment> trail_segments;
    float segment_spawn_timer;
    float last_x, last_y;
    int stuck_counter;
    
    QixEnemy(float start_x, float start_y, float dir_x, float dir_y, float enemy_speed, EnemyType enemy_type) 
        : x(start_x), y(start_y), dx(dir_x), dy(dir_y), speed(enemy_speed), type(enemy_type) {
        animation_phase = (rand() % 1000) / 1000.0f * 6.28f;
        color_phase = (rand() % 1000) / 1000.0f * 6.28f;
        shape_variant = rand() % 3;
        size_pulse = (rand() % 1000) / 1000.0f * 6.28f;
        morph_phase = (rand() % 1000) / 1000.0f * 6.28f;
        intensity_pulse = (rand() % 1000) / 1000.0f * 6.28f;
        segment_spawn_timer = 0.0f;
        last_x = start_x;
        last_y = start_y;
        stuck_counter = 0;
        trail_segments.reserve(15); // Limit trail length
    }
    
    void update(float delta_time) {
        // Multiple animation phases for complex behavior - slower, more pleasant speeds
        animation_phase += delta_time * 3.0f;  // Slower movement animation
        color_phase += delta_time * 1.5f;     // Much slower color cycling 
        size_pulse += delta_time * 4.0f;      // Moderate size pulsing
        morph_phase += delta_time * 2.0f;     // Slower morph animation
        intensity_pulse += delta_time * 2.5f; // Much slower intensity pulse
        segment_spawn_timer += delta_time;
        
        // Wrap phases
        if (animation_phase > 6.28f) animation_phase -= 6.28f;
        if (color_phase > 6.28f) color_phase -= 6.28f;
        if (size_pulse > 6.28f) size_pulse -= 6.28f;
        if (morph_phase > 6.28f) morph_phase -= 6.28f;
        if (intensity_pulse > 6.28f) intensity_pulse -= 6.28f;
        
        // Add trailing segments more frequently for visual effect
        if (segment_spawn_timer > 0.05f) { // Spawn every 50ms
            addTrailSegment();
            segment_spawn_timer = 0.0f;
        }
        
        // Update trail segments
        for (auto it = trail_segments.begin(); it != trail_segments.end();) {
            it->age += delta_time;
            it->alpha = 1.0f - (it->age / 1.5f); // Fade over 1.5 seconds
            
            if (it->age > 1.5f || it->alpha <= 0) {
                it = trail_segments.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void addTrailSegment() {
        if (trail_segments.size() >= 15) {
            trail_segments.erase(trail_segments.begin());
        }
        
        QixSegment segment;
        segment.x = x + (rand() % 3 - 1); // Small random offset
        segment.y = y + (rand() % 3 - 1);
        segment.age = 0.0f;
        segment.alpha = 1.0f;
        getColors(segment.r, segment.g, segment.b);
        
        trail_segments.push_back(segment);
    }
    
    void getColors(uint8_t& r, uint8_t& g, uint8_t& b) const {
        // Enhanced dynamic color changes with more dramatic pulsing
        float h = color_phase + sin(animation_phase) * 0.8f + cos(morph_phase) * 0.3f;
        float s = 0.98f + 0.02f * sin(size_pulse * 3); // Maximum saturation with pulse
        float v = 0.95f + 0.05f * (sin(intensity_pulse * 1.5f) * 0.7f + 0.3f); // Maximum brightness
        
        // Clamp hue to valid range
        while (h > 6.28f) h -= 6.28f;
        while (h < 0) h += 6.28f;
        
        int i = (int)(h / 1.047f); // Divide by π/3
        float f = (h / 1.047f) - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);
        
        switch (i % 6) {
            case 0: r = v * 255; g = t * 255; b = p * 255; break;
            case 1: r = q * 255; g = v * 255; b = p * 255; break;
            case 2: r = p * 255; g = v * 255; b = t * 255; break;
            case 3: r = p * 255; g = q * 255; b = v * 255; break;
            case 4: r = t * 255; g = p * 255; b = v * 255; break;
            case 5: r = v * 255; g = p * 255; b = q * 255; break;
        }
    }
};

struct Player {
    int x, y;
    int start_x, start_y;
    int trail_start_x, trail_start_y; // Where current trail began
    bool drawing_trail;
    std::vector<std::pair<int, int>> current_trail;
};

class QixGame : public GameBase {
private:
    std::array<std::array<CellType, QIX_FIELD_HEIGHT>, QIX_FIELD_WIDTH> field;
    Player player;
    std::vector<QixEnemy> qix_enemies;
    
    uint32_t last_update_time;
    uint32_t game_start_time;
    uint32_t level_start_time;
    int score;
    int level;
    int lives;
    float claimed_percentage;
    bool game_over;
    bool level_complete;
    bool time_up;
    bool showing_game_over;
    uint32_t game_over_start_time;
    static const int LEVEL_TIME_SECONDS = 120; // 2 minutes
    static const int MAX_LIVES = 5;
    static const int GAME_OVER_DISPLAY_TIME = 5000; // 5 seconds in milliseconds
    
    // Input handling
    bool button_a_pressed = false;
    bool button_b_pressed = false;
    bool button_c_pressed = false;
    bool button_d_pressed = false;
    bool button_vol_up_pressed = false;
    bool button_vol_down_pressed = false;
    uint32_t last_move_time = 0;
    const uint32_t move_delay = 200; // Move every 200ms when button held

public:
    const char* getName() const override {
        return "Qix";
    }
    
    const char* getDescription() const override {
        return "A/B: Left/Right, Vol+/-: Up/Down, Claim 75%!";
    }
    
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) override {
        gfx = &graphics;
        cosmic = &cosmic_unicorn;
        
        resetGame();
        last_update_time = to_ms_since_boot(get_absolute_time());
        game_start_time = last_update_time;
    }
    
    void resetGame() {
        // Initialize field with walls around the border
        for (int x = 0; x < QIX_FIELD_WIDTH; x++) {
            for (int y = 0; y < QIX_FIELD_HEIGHT; y++) {
                if (x == 0 || x == QIX_FIELD_WIDTH-1 || y == 0 || y == QIX_FIELD_HEIGHT-1) {
                    field[x][y] = CellType::WALL;
                } else {
                    field[x][y] = CellType::EMPTY;
                }
            }
        }
        
        // Initialize player at bottom center
        player.x = QIX_FIELD_WIDTH / 2;
        player.y = QIX_FIELD_HEIGHT - 1;
        player.start_x = player.x;
        player.start_y = player.y;
        player.trail_start_x = player.x;
        player.trail_start_y = player.y;
        player.drawing_trail = false;
        player.current_trail.clear();
        
        printf("Player initialized at (%d, %d), cell type: %d\n", 
               player.x, player.y, (int)field[player.x][player.y]);
        
        // Initialize Qix enemies
        qix_enemies.clear();
        int num_enemies = 1 + level / 3;  // More enemies as level increases
        EnemyType enemy_types[] = {EnemyType::BUTTERFLY, EnemyType::TURTLE, EnemyType::SPIRAL, 
                                   EnemyType::STAR, EnemyType::DIAMOND, EnemyType::JELLYFISH};
        
        for (int i = 0; i < num_enemies; i++) {
            float x = QIX_FIELD_WIDTH * 0.3f + (rand() % (int)(QIX_FIELD_WIDTH * 0.4f));
            float y = QIX_FIELD_HEIGHT * 0.3f + (rand() % (int)(QIX_FIELD_HEIGHT * 0.4f));
            float dx = (rand() % 100 - 50) / 100.0f;
            float dy = (rand() % 100 - 50) / 100.0f;
            if (dx == 0 && dy == 0) { dx = 1.0f; }
            float speed = 1.5f + (level * 0.2f); // Even faster base speed
            EnemyType type = enemy_types[rand() % 6];
            qix_enemies.push_back(QixEnemy(x, y, dx, dy, speed, type));
        }
        
        score = level * 1000;
        claimed_percentage = 0.0f;
        game_over = false;
        level_complete = false;
        time_up = false;
        showing_game_over = false;
        level_start_time = to_ms_since_boot(get_absolute_time());
        
        // Only reset lives when starting a new game (level 0)
        if (level == 0) {
            lives = MAX_LIVES;
        }
    }
    
    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down, 
                    bool button_bright_up, bool button_bright_down) override {
        
        // Store current button states
        button_a_pressed = button_a;
        button_b_pressed = button_b;
        button_c_pressed = button_c;
        button_d_pressed = button_d;
        button_vol_up_pressed = button_vol_up;
        button_vol_down_pressed = button_vol_down;
    }
    
    bool update() override {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        uint32_t delta_time = current_time - last_update_time;
        
        if (delta_time < 50) {  // 20 FPS for smoother animations
            return !checkExitCondition(button_d_pressed);
        }
        last_update_time = current_time;
        
        if (game_over || level_complete || time_up) {
            // Handle automatic game over screen timeout
            if (showing_game_over) {
                if (current_time - game_over_start_time >= GAME_OVER_DISPLAY_TIME) {
                    // Auto-restart after 5 seconds
                    showing_game_over = false;
                    level = 0; // Start from beginning
                    resetGame();
                    return !checkExitCondition(button_d_pressed);
                }
                // Keep enemies animating during game over screen!
                updateQixEnemies();
                return !checkExitCondition(button_d_pressed);
            }
            
            // Check for manual restart
            static bool last_a_state = false;
            if (button_a_pressed && !last_a_state) {
                if (level_complete) {
                    level++;
                } else if (time_up || game_over) {
                    // Reset to level 0 when time up or game over
                    level = 0;
                }
                resetGame();
            }
            last_a_state = button_a_pressed;
            // Keep enemies animating during other game states too
            updateQixEnemies();
            return !checkExitCondition(button_d_pressed);
        }
        
        // Check timer
        uint32_t elapsed_time = (current_time - level_start_time) / 1000; // Convert to seconds
        if (elapsed_time >= LEVEL_TIME_SECONDS) {
            time_up = true;
            lives--;
            if (lives <= 0) {
                game_over = true;
                showing_game_over = true;
                game_over_start_time = current_time;
            }
            // Still update enemies even when time is up!
            updateQixEnemies();
            return !checkExitCondition(button_d_pressed);
        }
        
        // Player movement
        updatePlayerMovement();
        
        // Update Qix enemies
        updateQixEnemies();
        
        // Check collisions
        checkCollisions();
        
        // Check level completion
        calculateClaimedPercentage();
        if (claimed_percentage >= 75.0f) {
            level_complete = true;
            score += 5000;
        }
        
        return !checkExitCondition(button_d_pressed);
    }
    
    void updatePlayerMovement() {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Check if enough time has passed since last move
        if (current_time - last_move_time < move_delay) {
            return;
        }
        
        int new_x = player.x;
        int new_y = player.y;
        bool moved = false;
        
        // Movement with directional buttons - use volume buttons for up/down to avoid conflict with exit
        if (button_a_pressed) {  // Left
            new_x = player.x - 1;
            moved = true;
        } else if (button_b_pressed) {  // Right
            new_x = player.x + 1;
            moved = true;
        } else if (button_vol_up_pressed) {  // Up
            new_y = player.y - 1;
            moved = true;
        } else if (button_vol_down_pressed) {  // Down
            new_y = player.y + 1;
            moved = true;
        }
        
        if (!moved) return;
        
        // Update move timing
        last_move_time = current_time;
        
        // Check bounds
        if (new_x < 0 || new_x >= QIX_FIELD_WIDTH || new_y < 0 || new_y >= QIX_FIELD_HEIGHT) {
            return;
        }
        
        CellType target_cell = field[new_x][new_y];
        
        // Check if we can move to this cell
        if (target_cell == CellType::TRAIL) {
            // Can't move to our own trail
            return;
        }
        
        CellType current_cell = field[player.x][player.y];
        int old_x = player.x;
        int old_y = player.y;
        
        // Debug output
        printf("Moving from (%d,%d) to (%d,%d), current_cell: %d, target_cell: %d, drawing: %s\n", 
               old_x, old_y, new_x, new_y, (int)current_cell, (int)target_cell, 
               player.drawing_trail ? "yes" : "no");
        
        // Update position
        player.x = new_x;
        player.y = new_y;
        
        // Handle trail drawing logic
        if (current_cell == CellType::WALL && target_cell == CellType::EMPTY) {
            // Starting a new trail from wall to empty space
            player.drawing_trail = true;
            player.current_trail.clear();
            player.start_x = old_x;  // Remember the wall position we came from
            player.start_y = old_y;
            player.trail_start_x = old_x;  // Remember where this trail started
            player.trail_start_y = old_y;
            printf("Started drawing trail from wall at (%d, %d)\n", player.start_x, player.start_y);
        }
        
        if (player.drawing_trail && target_cell == CellType::EMPTY) {
            // Continue drawing trail in empty space
            field[player.x][player.y] = CellType::TRAIL;
            player.current_trail.push_back({player.x, player.y});
            printf("Added position to trail: (%d, %d)\n", player.x, player.y);
        }
        
        if (player.drawing_trail && target_cell == CellType::WALL) {
            // Completing trail by reaching any wall
            printf("Attempting to complete trail by reaching wall at (%d, %d)\n", new_x, new_y);
            completeTrail();
        }
    }
    
    void completeTrail() {
        if (!player.drawing_trail) {
            printf("completeTrail: not drawing trail\n");
            return;
        }
        
        if (player.current_trail.empty()) {
            printf("completeTrail: trail is empty\n");
            return;
        }
        
        printf("Completing trail of length: %zu\n", player.current_trail.size());
        
        // Mark trail as permanent wall
        for (auto& pos : player.current_trail) {
            field[pos.first][pos.second] = CellType::WALL;
            printf("Marked (%d, %d) as wall\n", pos.first, pos.second);
        }
        
        printf("Starting flood fill...\n");
        // Flood fill to find areas to claim
        claimEnclosedAreas();
        printf("Finished flood fill\n");
        
        // Award points for completing trail (use stored size since we're about to clear it)
        int trail_size = player.current_trail.size();
        
        player.drawing_trail = false;
        player.current_trail.clear();
        
        score += trail_size * 10;
        printf("Trail completed, awarded %d points\n", trail_size * 10);
    }
    
    void claimEnclosedAreas() {
        // Create temporary field for flood fill
        std::array<std::array<bool, QIX_FIELD_HEIGHT>, QIX_FIELD_WIDTH> visited;
        for (int x = 0; x < QIX_FIELD_WIDTH; x++) {
            for (int y = 0; y < QIX_FIELD_HEIGHT; y++) {
                visited[x][y] = false;
            }
        }
        
        // Find all empty areas and determine if they contain Qix balls
        for (int x = 1; x < QIX_FIELD_WIDTH - 1; x++) {
            for (int y = 1; y < QIX_FIELD_HEIGHT - 1; y++) {
                if (field[x][y] == CellType::EMPTY && !visited[x][y]) {
                    std::vector<std::pair<int, int>> area;
                    bool contains_qix = false;
                    
                    // Flood fill to find connected empty area
                    floodFill(x, y, area, visited, contains_qix);
                    
                    // If area doesn't contain Qix balls, claim it
                    if (!contains_qix && !area.empty()) {
                        for (auto& pos : area) {
                            field[pos.first][pos.second] = CellType::CLAIMED;
                        }
                        score += area.size() * 5;
                        // Debug output to track area claiming
                        printf("Claimed area of size: %zu\n", area.size());
                    }
                }
            }
        }
    }
    
    void floodFill(int x, int y, std::vector<std::pair<int, int>>& area, 
                   std::array<std::array<bool, QIX_FIELD_HEIGHT>, QIX_FIELD_WIDTH>& visited,
                   bool& contains_qix) {
        if (x < 0 || x >= QIX_FIELD_WIDTH || y < 0 || y >= QIX_FIELD_HEIGHT) return;
        if (visited[x][y] || field[x][y] != CellType::EMPTY) return;
        
        visited[x][y] = true;
        area.push_back({x, y});
        
        // Check if any Qix enemy is in this area (with some tolerance for floating point positions)
        for (auto& enemy : qix_enemies) {
            if (abs((int)enemy.x - x) <= 1 && abs((int)enemy.y - y) <= 1) {
                contains_qix = true;
            }
        }
        
        // Recursively fill adjacent cells
        floodFill(x-1, y, area, visited, contains_qix);
        floodFill(x+1, y, area, visited, contains_qix);
        floodFill(x, y-1, area, visited, contains_qix);
        floodFill(x, y+1, area, visited, contains_qix);
    }
    
    bool isValidPosition(float x, float y) {
        // Check bounds with safe margin
        if (x < 5.0f || x >= QIX_FIELD_WIDTH - 6.0f || y < 5.0f || y >= QIX_FIELD_HEIGHT - 6.0f) {
            return false;
        }
        
        // Check if cell is empty
        int cell_x = (int)x;
        int cell_y = (int)y;
        if (cell_x >= 0 && cell_x < QIX_FIELD_WIDTH && cell_y >= 0 && cell_y < QIX_FIELD_HEIGHT) {
            return field[cell_x][cell_y] == CellType::EMPTY;
        }
        return false;
    }
    
    void updateQixEnemies() {
        float delta_time = 0.05f; // Fixed timing for consistent animation at 20 FPS
        
        for (auto& enemy : qix_enemies) {
            // Update animation and colors
            enemy.update(delta_time);
            
            // Store current position (for future use if needed)
            (void)enemy.x; // Suppress unused variable warning
            (void)enemy.y;
            
            // Calculate next position
            float next_x = enemy.x + enemy.dx * enemy.speed;
            float next_y = enemy.y + enemy.dy * enemy.speed;
            
            // Check if next position is valid BEFORE moving
            bool can_move_x = isValidPosition(next_x, enemy.y);
            bool can_move_y = isValidPosition(enemy.x, next_y);
            
            // Move only if safe, otherwise bounce
            if (can_move_x) {
                enemy.x = next_x;
            } else {
                enemy.dx = -enemy.dx;
                // Add slight random component to prevent oscillation
                enemy.dx += (rand() % 40 - 20) / 100.0f;
            }
            
            if (can_move_y) {
                enemy.y = next_y;
            } else {
                enemy.dy = -enemy.dy;
                // Add slight random component to prevent oscillation
                enemy.dy += (rand() % 40 - 20) / 100.0f;
            }
            
            // Stuck detection - if enemy hasn't moved much
            float distance_moved = sqrt(pow(enemy.x - enemy.last_x, 2) + pow(enemy.y - enemy.last_y, 2));
            if (distance_moved < 0.1f) {
                enemy.stuck_counter++;
                if (enemy.stuck_counter > 5) {
                    // Force unstick - teleport to center and randomize direction
                    printf("Enemy stuck, teleporting to center\n");
                    enemy.x = QIX_FIELD_WIDTH / 2.0f;
                    enemy.y = QIX_FIELD_HEIGHT / 2.0f;
                    enemy.dx = (rand() % 200 - 100) / 100.0f;
                    enemy.dy = (rand() % 200 - 100) / 100.0f;
                    if (enemy.dx == 0 && enemy.dy == 0) {
                        enemy.dx = 1.0f; enemy.dy = 0.7f;
                    }
                    enemy.stuck_counter = 0;
                }
            } else {
                enemy.stuck_counter = 0;
            }
            
            // Update last position for stuck detection
            enemy.last_x = enemy.x;
            enemy.last_y = enemy.y;
            
            // Normalize velocity to maintain consistent speed
            float vel_mag = sqrt(enemy.dx * enemy.dx + enemy.dy * enemy.dy);
            if (vel_mag > 0.1f) {
                enemy.dx = (enemy.dx / vel_mag) * 1.0f; // Normalize to unit speed
                enemy.dy = (enemy.dy / vel_mag) * 1.0f;
            }
            
            // Final safety bounds
            enemy.x = fmax(5.0f, fmin(QIX_FIELD_WIDTH - 6.0f, enemy.x));
            enemy.y = fmax(5.0f, fmin(QIX_FIELD_HEIGHT - 6.0f, enemy.y));
        }
    }
    
    void checkCollisions() {
        // Check if player collides with Qix enemies while drawing trail
        if (player.drawing_trail) {
            for (auto& enemy : qix_enemies) {
                if (abs((int)enemy.x - player.x) <= 1 && abs((int)enemy.y - player.y) <= 1) {
                    handlePlayerDeath();
                    return;
                }
            }
        }
        
        // Check if Qix enemies hit the trail (including current incomplete trail)
        for (auto& enemy : qix_enemies) {
            int enemy_cell_x = (int)enemy.x;
            int enemy_cell_y = (int)enemy.y;
            
            // Check if enemy is on a completed trail segment (permanent trail)
            if (enemy_cell_x >= 0 && enemy_cell_x < QIX_FIELD_WIDTH && 
                enemy_cell_y >= 0 && enemy_cell_y < QIX_FIELD_HEIGHT) {
                if (field[enemy_cell_x][enemy_cell_y] == CellType::TRAIL) {
                    handlePlayerDeath();
                    return;
                }
            }
            
            // Also check if enemy touches any part of the current trail being drawn
            if (player.drawing_trail && !player.current_trail.empty()) {
                for (const auto& trail_pos : player.current_trail) {
                    if (abs(enemy_cell_x - trail_pos.first) <= 1 && abs(enemy_cell_y - trail_pos.second) <= 1) {
                        handlePlayerDeath();
                        return;
                    }
                }
            }
        }
    }
    
    void calculateClaimedPercentage() {
        int total_empty_cells = 0;
        int claimed_cells = 0;
        
        for (int x = 1; x < QIX_FIELD_WIDTH - 1; x++) {
            for (int y = 1; y < QIX_FIELD_HEIGHT - 1; y++) {
                if (field[x][y] == CellType::EMPTY || field[x][y] == CellType::CLAIMED) {
                    total_empty_cells++;
                    if (field[x][y] == CellType::CLAIMED) {
                        claimed_cells++;
                    }
                }
            }
        }
        
        claimed_percentage = total_empty_cells > 0 ? (float)claimed_cells / total_empty_cells * 100.0f : 0.0f;
    }
    
    void handlePlayerDeath() {
        lives--;
        
        if (lives <= 0) {
            game_over = true;
            showing_game_over = true;
            game_over_start_time = to_ms_since_boot(get_absolute_time());
            return;
        }
        
        // Clear current trail from field
        for (const auto& pos : player.current_trail) {
            field[pos.first][pos.second] = CellType::EMPTY;
        }
        
        // Teleport player back to where trail started
        player.x = player.trail_start_x;
        player.y = player.trail_start_y;
        player.drawing_trail = false;
        player.current_trail.clear();
        
        printf("Player died! Lives remaining: %d. Teleported to (%d, %d)\n", lives, player.x, player.y);
    }
    
    void render(PicoGraphics_PenRGB888& graphics) override {
        graphics.set_pen(graphics.create_pen(0, 0, 0));
        graphics.clear();
        
        // Draw Qix enemies first (behind everything)
        for (auto& enemy : qix_enemies) {
            drawQixEnemy(graphics, enemy);
        }
        
        // Draw field on top of enemies
        for (int x = 0; x < QIX_FIELD_WIDTH; x++) {
            for (int y = 0; y < QIX_FIELD_HEIGHT; y++) {
                int screen_x = QIX_FIELD_OFFSET_X + x;
                int screen_y = QIX_FIELD_OFFSET_Y + y;
                
                switch (field[x][y]) {
                    case CellType::WALL:
                        graphics.set_pen(graphics.create_pen(30, 60, 120));  // Deep blue walls
                        graphics.pixel(Point(screen_x, screen_y));
                        break;
                    case CellType::TRAIL:
                        graphics.set_pen(graphics.create_pen(255, 255, 0));  // Yellow trail
                        graphics.pixel(Point(screen_x, screen_y));
                        break;
                    case CellType::CLAIMED:
                        graphics.set_pen(graphics.create_pen(0, 150, 255));  // Blue claimed area (more visible)
                        graphics.pixel(Point(screen_x, screen_y));
                        break;
                    case CellType::EMPTY:
                    default:
                        // Don't draw empty cells - let enemies show through
                        break;
                }
            }
        }
        
        // Only draw player if game is actively running
        if (!game_over && !level_complete && !time_up && !showing_game_over) {
            // Draw player on top with red color and glow
            int player_x = QIX_FIELD_OFFSET_X + player.x;
            int player_y = QIX_FIELD_OFFSET_Y + player.y;
            
            // Glow effect around player
            graphics.set_pen(graphics.create_pen(100, 0, 0));  // Dark red glow
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx != 0 || dy != 0) { // Don't draw on center pixel yet
                        int glow_x = player_x + dx;
                        int glow_y = player_y + dy;
                        if (glow_x >= QIX_FIELD_OFFSET_X && glow_x < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            glow_y >= QIX_FIELD_OFFSET_Y && glow_y < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(glow_x, glow_y));
                        }
                    }
                }
            }
            
            // Bright red center player pixel
            graphics.set_pen(graphics.create_pen(255, 0, 0));  // Bright red player
            graphics.pixel(Point(player_x, player_y));
        }
        
        // Draw countdown timer bar in rightmost column (column 31)
        if (!game_over && !level_complete && !time_up) {
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            uint32_t elapsed_time = (current_time - level_start_time) / 1000;
            float time_remaining = (float)(LEVEL_TIME_SECONDS - elapsed_time) / LEVEL_TIME_SECONDS;
            if (time_remaining < 0) time_remaining = 0;
            
            int timer_height = (int)(time_remaining * 32); // Full column height
            
            // Draw timer bar from bottom up
            for (int y = 0; y < 32; y++) {
                if (y >= (32 - timer_height)) {
                    // Timer remaining - color based on time left
                    if (time_remaining > 0.5f) {
                        graphics.set_pen(graphics.create_pen(0, 255, 0)); // Green
                    } else if (time_remaining > 0.25f) {
                        graphics.set_pen(graphics.create_pen(255, 255, 0)); // Yellow
                    } else {
                        graphics.set_pen(graphics.create_pen(255, 0, 0)); // Red
                    }
                } else {
                    graphics.set_pen(graphics.create_pen(20, 20, 20)); // Dark background
                }
                graphics.pixel(Point(31, y));
            }
        }
        
        // Draw lives in top-left corner
        for (int i = 0; i < MAX_LIVES; i++) {
            if (i < lives) {
                graphics.set_pen(graphics.create_pen(255, 0, 0)); // Red for remaining lives
            } else {
                graphics.set_pen(graphics.create_pen(50, 0, 0)); // Dark red for lost lives
            }
            graphics.pixel(Point(i, 31)); // Top row
        }
        
        // Draw UI messages
        if (showing_game_over) {
            // Keep enemies animating in background, then overlay game over text
            // Don't clear the screen - let enemies show through!
            
            // Add semi-transparent dark overlay so text is readable
            graphics.set_pen(graphics.create_pen(0, 0, 0));
            for (int y = 8; y < 24; y++) {
                for (int x = 2; x < 30; x++) {
                    graphics.pixel(Point(x, y));
                }
            }
            
            // Draw "GAME OVER" text centered
            graphics.set_pen(graphics.create_pen(255, 0, 0));
            std::string game_over_text = "GAME OVER";
            int text_width = graphics.measure_text(game_over_text, 1.0f);
            Point text_pos((32 - text_width) / 2, 10);
            graphics.text(game_over_text, text_pos, -1, 1.0f);
            
            // Show countdown timer
            uint32_t current_time = to_ms_since_boot(get_absolute_time());
            uint32_t time_elapsed = current_time - game_over_start_time;
            int seconds_remaining = (GAME_OVER_DISPLAY_TIME - time_elapsed) / 1000 + 1;
            if (seconds_remaining < 0) seconds_remaining = 0;
            
            graphics.set_pen(graphics.create_pen(255, 255, 255));
            std::string restart_text = "Restarting: " + std::to_string(seconds_remaining);
            int restart_width = graphics.measure_text(restart_text, 0.7f);
            Point restart_pos((32 - restart_width) / 2, 20);
            graphics.text(restart_text, restart_pos, -1, 0.7f);
        } else if (game_over) {
            drawText("GAME OVER", 2, 0, 255, 0, 0);
            drawText("A: Restart", 2, 6, 255, 255, 255);
        } else if (time_up) {
            drawText("TIME UP!", 2, 0, 255, 255, 0);
            drawText("A: Restart", 2, 6, 255, 255, 255);
        } else if (level_complete) {
            drawText("LEVEL DONE!", 2, 0, 0, 255, 0);
            drawText("A: Next", 2, 6, 255, 255, 255);
        } else {
            // Show progress - draw dots representing percentage
            int dots_filled = (int)(claimed_percentage / 100.0f * 25); // Leave space for lives
            for (int i = 0; i < 25; i++) {
                if (i < dots_filled) {
                    gfx->set_pen(gfx->create_pen(0, 255, 0));  // Green for claimed
                } else {
                    gfx->set_pen(gfx->create_pen(50, 50, 50)); // Dark grey for remaining
                }
                gfx->pixel(Point(6 + i, 31)); // Top row, after lives
            }
        }
        
        cosmic->update(&graphics);
    }
    
    void drawQixEnemy(PicoGraphics_PenRGB888& graphics, const QixEnemy& enemy) {
        int center_x = QIX_FIELD_OFFSET_X + (int)enemy.x;
        int center_y = QIX_FIELD_OFFSET_Y + (int)enemy.y;
        
        // Draw enemy trail segments for enhanced visual effects
        for (const auto& segment : enemy.trail_segments) {
            uint8_t trail_r = (uint8_t)(segment.r * segment.alpha * 0.8f);
            uint8_t trail_g = (uint8_t)(segment.g * segment.alpha * 0.8f);
            uint8_t trail_b = (uint8_t)(segment.b * segment.alpha * 0.8f);
            
            if (trail_r > 10 || trail_g > 10 || trail_b > 10) { // Brighter trail visibility
                graphics.set_pen(graphics.create_pen(trail_r, trail_g, trail_b));
                int trail_x = QIX_FIELD_OFFSET_X + (int)segment.x;
                int trail_y = QIX_FIELD_OFFSET_Y + (int)segment.y;
                if (trail_x >= QIX_FIELD_OFFSET_X && trail_x < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                    trail_y >= QIX_FIELD_OFFSET_Y && trail_y < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                    graphics.pixel(Point(trail_x, trail_y));
                }
            }
        }
        
        // Get current main colors
        uint8_t r = 255, g = 255, b = 255;
        enemy.getColors(r, g, b);
        
        // Enhanced size and intensity variations with more dramatic pulsing
        float size_pulse_factor = sin(enemy.size_pulse) * sin(enemy.animation_phase * 0.5f);
        float size_mod = 2.5f + 1.2f * size_pulse_factor; // Larger size range (1.3 to 3.7)
        float intensity_mod = 0.85f + 0.15f * (sin(enemy.intensity_pulse) + cos(enemy.color_phase * 0.7f)); // More dramatic brightness
        float morph_factor = sin(enemy.morph_phase) * cos(enemy.animation_phase * 0.3f);
        
        // Apply intensity to colors
        r = (uint8_t)(r * intensity_mod);
        g = (uint8_t)(g * intensity_mod);
        b = (uint8_t)(b * intensity_mod);
        
        graphics.set_pen(graphics.create_pen(r, g, b));
        
        switch (enemy.type) {
            case EnemyType::BUTTERFLY: {
                // Enhanced butterfly with pulsing body and dramatic wing beats
                // Pulsing body core
                uint8_t body_r = (uint8_t)(r * (0.9f + 0.1f * sin(enemy.intensity_pulse * 2)));
                uint8_t body_g = (uint8_t)(g * (0.9f + 0.1f * cos(enemy.intensity_pulse * 2.3f)));
                uint8_t body_b = (uint8_t)(b * (0.9f + 0.1f * sin(enemy.intensity_pulse * 1.8f)));
                graphics.set_pen(graphics.create_pen(body_r, body_g, body_b));
                graphics.pixel(Point(center_x, center_y)); // Body
                
                // Pulsing extended body segments
                graphics.pixel(Point(center_x, center_y - 1)); // Extended body
                graphics.pixel(Point(center_x, center_y + 1));
                
                float wing_beat = sin(enemy.animation_phase * 5) * 0.6f + 0.4f; // Faster, more dramatic beat
                float wing_spread = (2.0f + sin(enemy.morph_phase) * 0.8f) * size_mod; // Even larger wings
                graphics.set_pen(graphics.create_pen(r, g, b));
                
                // Upper wings - size based on size_mod with larger base
                int max_wing_size = (int)(4 + size_mod * 2); // 6-10 pixel range
                int wing_extent = (int)(wing_spread * (1 + wing_beat));
                for (int i = 1; i <= wing_extent && i <= max_wing_size; i++) {
                    int wing_y_offset = -1 - (int)(sin(enemy.animation_phase + i) * 1.0f);
                    
                    // Main wing structure
                    graphics.pixel(Point(center_x - i, center_y + wing_y_offset));
                    graphics.pixel(Point(center_x + i, center_y + wing_y_offset));
                    
                    // Wing thickness - multiple rows
                    if (i <= 4) {
                        graphics.pixel(Point(center_x - i, center_y + wing_y_offset - 1));
                        graphics.pixel(Point(center_x + i, center_y + wing_y_offset - 1));
                    }
                    if (i <= 3) {
                        graphics.pixel(Point(center_x - i, center_y + wing_y_offset - 2));
                        graphics.pixel(Point(center_x + i, center_y + wing_y_offset - 2));
                    }
                    
                    // Wing tips with color variation
                    if (i >= 3) {
                        graphics.set_pen(graphics.create_pen(r/2, g, b));
                        graphics.pixel(Point(center_x - i, center_y + wing_y_offset - 1));
                        graphics.pixel(Point(center_x + i, center_y + wing_y_offset - 1));
                        graphics.set_pen(graphics.create_pen(r, g, b));
                    }
                }
                
                // Lower wings - larger and more prominent
                for (int i = 1; i <= wing_extent/2 + 2 && i <= 4; i++) {
                    int wing_y_offset = 1 + (int)(cos(enemy.animation_phase * 2 + i) * 0.5f);
                    graphics.pixel(Point(center_x - i, center_y + wing_y_offset));
                    graphics.pixel(Point(center_x + i, center_y + wing_y_offset));
                    
                    // Lower wing thickness
                    if (i <= 3) {
                        graphics.pixel(Point(center_x - i, center_y + wing_y_offset + 1));
                        graphics.pixel(Point(center_x + i, center_y + wing_y_offset + 1));
                    }
                }
                break;
            }
            
            case EnemyType::TURTLE: {
                // Enhanced turtle with pulsing core and dynamic shell ripples
                // Pulsing center core
                uint8_t core_r = (uint8_t)(r * (0.8f + 0.2f * sin(enemy.intensity_pulse * 3)));
                uint8_t core_g = (uint8_t)(g * (0.8f + 0.2f * cos(enemy.intensity_pulse * 2.7f)));
                uint8_t core_b = (uint8_t)(b * (0.9f + 0.1f * sin(enemy.intensity_pulse * 2.1f)));
                graphics.set_pen(graphics.create_pen(core_r, core_g, core_b));
                graphics.pixel(Point(center_x, center_y));
                
                float shell_phase = enemy.animation_phase + sin(enemy.intensity_pulse) * 0.5f;
                
                // Large dynamic shell pattern - multiple rings
                for (int ring = 1; ring <= 3; ring++) {
                    for (int angle_step = 0; angle_step < 12; angle_step++) {
                        float angle = shell_phase + angle_step * 0.52f; // 30 degree steps
                        float radius = ring * (1.0f + sin(shell_phase * 2 + angle_step) * 0.7f);
                        
                        int dx = (int)(cos(angle) * radius);
                        int dy = (int)(sin(angle) * radius);
                        
                        // Varying colors around the shell rings
                        uint8_t shell_r = (uint8_t)(r * (0.6f + 0.4f * sin(angle + ring)));
                        uint8_t shell_g = (uint8_t)(g * (0.6f + 0.4f * cos(angle - ring)));
                        uint8_t shell_b = (uint8_t)(b * (0.8f + 0.2f * sin(angle * 2)));
                        
                        graphics.set_pen(graphics.create_pen(shell_r, shell_g, shell_b));
                        
                        int px = center_x + dx;
                        int py = center_y + dy;
                        if (px >= QIX_FIELD_OFFSET_X && px < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            py >= QIX_FIELD_OFFSET_Y && py < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(px, py));
                        }
                    }
                }
                
                // Large moving limbs
                float limb_phase = enemy.animation_phase * 3;
                graphics.set_pen(graphics.create_pen(r, g/2, b/2));
                for (int limb = 0; limb < 4; limb++) {
                    if (sin(limb_phase + limb) > 0.2f) {
                        int limb_x = center_x + (limb % 2 == 0 ? -4 : 4);
                        int limb_y = center_y + (limb < 2 ? -2 : 2);
                        
                        graphics.pixel(Point(limb_x, limb_y));
                        graphics.pixel(Point(limb_x + (limb % 2 == 0 ? -1 : 1), limb_y));
                    }
                }
                break;
            }
            
            case EnemyType::SPIRAL: {
                // Enhanced spiral with pulsing center and more dramatic arm motion
                // Intense pulsing center
                uint8_t spiral_r = (uint8_t)(r * (0.7f + 0.3f * sin(enemy.intensity_pulse * 4)));
                uint8_t spiral_g = (uint8_t)(g * (0.7f + 0.3f * cos(enemy.intensity_pulse * 3.5f)));
                uint8_t spiral_b = (uint8_t)(b * (0.8f + 0.2f * sin(enemy.intensity_pulse * 2.8f)));
                graphics.set_pen(graphics.create_pen(spiral_r, spiral_g, spiral_b));
                graphics.pixel(Point(center_x, center_y));
                graphics.set_pen(graphics.create_pen(r, g, b));
                
                int num_arms = 5 + (int)(sin(enemy.morph_phase) * 3); // 5-8 arms 
                float spiral_stretch = size_mod * (1.5f + sin(enemy.size_pulse) * 0.5f); // Larger, more dramatic
                
                int max_segments = (int)(5 + size_mod * 2); // 8-10 segments - always visible
                for (int arm = 0; arm < num_arms; arm++) {
                    for (int segment = 1; segment <= max_segments; segment++) {
                        float angle = enemy.animation_phase + arm * (6.28f / num_arms) + segment * 0.4f;
                        float radius = segment * spiral_stretch;
                        
                        int dx = (int)(cos(angle) * radius);
                        int dy = (int)(sin(angle) * radius);
                        
                        // Color gradient along arms with more variation
                        float segment_intensity = 1.0f - (segment * 0.1f);
                        uint8_t arm_r = (uint8_t)(r * segment_intensity * (0.7f + 0.3f * sin(angle)));
                        uint8_t arm_g = (uint8_t)(g * segment_intensity * (0.7f + 0.3f * cos(angle * 1.3f)));
                        uint8_t arm_b = (uint8_t)(b * segment_intensity * (0.8f + 0.2f * sin(angle * 2)));
                        
                        graphics.set_pen(graphics.create_pen(arm_r, arm_g, arm_b));
                        
                        int px = center_x + dx;
                        int py = center_y + dy;
                        if (px >= QIX_FIELD_OFFSET_X && px < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH && 
                            py >= QIX_FIELD_OFFSET_Y && py < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(px, py));
                            
                            // Add thickness to arms
                            if (segment <= 5) {
                                if (px + 1 < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH)
                                    graphics.pixel(Point(px + 1, py));
                                if (py + 1 < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT)
                                    graphics.pixel(Point(px, py + 1));
                            }
                        }
                    }
                }
                break;
            }
            
            case EnemyType::STAR: {
                // Enhanced star with intensely pulsing core and dynamic rays
                // Super bright pulsing core that varies in color
                uint8_t star_r = (uint8_t)(fmin(255, r * (1.2f + 0.3f * sin(enemy.intensity_pulse * 6))));
                uint8_t star_g = (uint8_t)(fmin(255, g * (1.2f + 0.3f * cos(enemy.intensity_pulse * 5.5f))));
                uint8_t star_b = (uint8_t)(fmin(255, b * (1.1f + 0.2f * sin(enemy.intensity_pulse * 4.8f))));
                graphics.set_pen(graphics.create_pen(star_r, star_g, star_b));
                graphics.pixel(Point(center_x, center_y));
                graphics.pixel(Point(center_x - 1, center_y));
                graphics.pixel(Point(center_x + 1, center_y));
                graphics.pixel(Point(center_x, center_y - 1));
                graphics.pixel(Point(center_x, center_y + 1));
                graphics.set_pen(graphics.create_pen(r, g, b));
                
                int num_rays = 6 + (int)(sin(enemy.morph_phase * 2) * 2); // 6-8 rays
                float ray_length = 3.0f + size_mod * 2.0f; // 4.5-8 pixel rays
                
                for (int ray = 0; ray < num_rays; ray++) {
                    float angle = (ray * 6.28f / num_rays) + enemy.animation_phase * 0.5f;
                    
                    for (int len = 1; len <= (int)ray_length; len++) {
                        float pulse_offset = sin(enemy.animation_phase * 2 + len * 0.3f) * 0.5f;
                        int dx = (int)(cos(angle) * (len + pulse_offset));
                        int dy = (int)(sin(angle) * (len + pulse_offset));
                        
                        // Ray color intensity decreases with distance but more gradual
                        float ray_intensity = 1.0f - (len * 0.15f);
                        uint8_t ray_r = (uint8_t)(r * ray_intensity * (0.8f + 0.2f * sin(angle + enemy.animation_phase)));
                        uint8_t ray_g = (uint8_t)(g * ray_intensity * (0.8f + 0.2f * cos(angle * 1.5f)));
                        uint8_t ray_b = (uint8_t)(b * ray_intensity);
                        
                        graphics.set_pen(graphics.create_pen(ray_r, ray_g, ray_b));
                        
                        int px = center_x + dx;
                        int py = center_y + dy;
                        if (px >= QIX_FIELD_OFFSET_X && px < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            py >= QIX_FIELD_OFFSET_Y && py < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(px, py));
                            
                            // Add ray thickness for inner segments
                            if (len <= 4) {
                                // Side pixels for thicker rays
                                float perp_angle = angle + 1.57f; // 90 degrees
                                int side_dx = (int)(cos(perp_angle));
                                int side_dy = (int)(sin(perp_angle));
                                
                                if (px + side_dx >= QIX_FIELD_OFFSET_X && px + side_dx < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                                    py + side_dy >= QIX_FIELD_OFFSET_Y && py + side_dy < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                                    graphics.pixel(Point(px + side_dx, py + side_dy));
                                }
                            }
                        }
                    }
                }
                break;
            }
            
            case EnemyType::DIAMOND: {
                // Enhanced diamond with brilliant pulsing center
                // Brilliant center that pulses with rainbow colors
                uint8_t diamond_r = (uint8_t)(fmin(255, r * (1.3f + 0.4f * sin(enemy.intensity_pulse * 7))));
                uint8_t diamond_g = (uint8_t)(fmin(255, g * (1.3f + 0.4f * cos(enemy.intensity_pulse * 6.2f))));
                uint8_t diamond_b = (uint8_t)(fmin(255, b * (1.2f + 0.3f * sin(enemy.intensity_pulse * 5.7f))));
                graphics.set_pen(graphics.create_pen(diamond_r, diamond_g, diamond_b));
                graphics.pixel(Point(center_x, center_y));
                
                float diamond_size = 2.5f + size_mod * 2.5f; // Even larger and more dramatic
                float rotation = enemy.animation_phase + sin(enemy.morph_phase) * 0.3f; // More complex rotation
                
                // Large main diamond shape with multiple layers
                for (int layer = 1; layer <= 3; layer++) {
                    for (int i = 0; i < 8; i++) { // More points for rounder diamond
                        float angle = rotation + i * 0.785f; // 45 degrees apart
                        float layer_size = diamond_size * layer / 3.0f;
                        int dx = (int)(cos(angle) * layer_size);
                        int dy = (int)(sin(angle) * layer_size);
                        
                        // Layer color variation
                        float layer_intensity = 1.0f - (layer * 0.2f);
                        uint8_t layer_r = (uint8_t)(r * layer_intensity * (0.8f + 0.2f * sin(angle)));
                        uint8_t layer_g = (uint8_t)(g * layer_intensity * (0.8f + 0.2f * cos(angle)));
                        uint8_t layer_b = (uint8_t)(b * layer_intensity);
                        
                        graphics.set_pen(graphics.create_pen(layer_r, layer_g, layer_b));
                        
                        int px = center_x + dx;
                        int py = center_y + dy;
                        if (px >= QIX_FIELD_OFFSET_X && px < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            py >= QIX_FIELD_OFFSET_Y && py < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(px, py));
                        }
                        
                        // Inner sparkles
                        if (sin(enemy.intensity_pulse + i + layer) > 0.6f) {
                            graphics.set_pen(graphics.create_pen(255, 255, 255));
                            int spark_x = center_x + dx/2;
                            int spark_y = center_y + dy/2;
                            if (spark_x >= QIX_FIELD_OFFSET_X && spark_x < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                                spark_y >= QIX_FIELD_OFFSET_Y && spark_y < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                                graphics.pixel(Point(spark_x, spark_y));
                            }
                        }
                    }
                }
                
                // Large outer shell that appears/disappears
                if (morph_factor > 0.2f) {
                    graphics.set_pen(graphics.create_pen(r/3, g/3, b));
                    for (int i = 0; i < 6; i++) {
                        float angle = rotation * 0.5f + i * 1.047f; // 60 degrees
                        int dx = (int)(cos(angle) * (diamond_size + 2));
                        int dy = (int)(sin(angle) * (diamond_size + 2));
                        
                        int px = center_x + dx;
                        int py = center_y + dy;
                        if (px >= QIX_FIELD_OFFSET_X && px < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            py >= QIX_FIELD_OFFSET_Y && py < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(px, py));
                        }
                    }
                }
                break;
            }
            
            case EnemyType::JELLYFISH: {
                // Enhanced jellyfish with intensely pulsing dome and flowing tentacles
                // Intensely pulsing center core
                uint8_t jelly_r = (uint8_t)(r * (0.9f + 0.1f * sin(enemy.intensity_pulse * 8)));
                uint8_t jelly_g = (uint8_t)(g * (0.9f + 0.1f * cos(enemy.intensity_pulse * 7.3f)));
                uint8_t jelly_b = (uint8_t)(b * (1.0f + 0.1f * sin(enemy.intensity_pulse * 6.8f)));
                graphics.set_pen(graphics.create_pen(jelly_r, jelly_g, jelly_b));
                graphics.pixel(Point(center_x, center_y));
                
                // Larger pulsing dome with more dramatic size changes
                int dome_size = (int)(3 + size_mod * 1.2f); // Much larger dome
                for (int row = 0; row <= 2; row++) {
                    int row_width = dome_size - row;
                    for (int i = -row_width; i <= row_width; i++) {
                        int dome_x = center_x + i;
                        int dome_y = center_y - row;
                        
                        // Dome color variation
                        float dome_intensity = 1.0f - (row * 0.15f) - (abs(i) * 0.1f);
                        uint8_t dome_r = (uint8_t)(r * dome_intensity);
                        uint8_t dome_g = (uint8_t)(g * dome_intensity * (0.9f + 0.1f * sin(enemy.animation_phase + i)));
                        uint8_t dome_b = (uint8_t)(b * dome_intensity);
                        
                        graphics.set_pen(graphics.create_pen(dome_r, dome_g, dome_b));
                        
                        if (dome_x >= QIX_FIELD_OFFSET_X && dome_x < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            dome_y >= QIX_FIELD_OFFSET_Y && dome_y < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(dome_x, dome_y));
                        }
                    }
                }
                
                // Many flowing tentacles with wave motion
                int num_tentacles = 5 + (int)(sin(enemy.morph_phase) * 3); // 5-8 tentacles
                for (int t = 0; t < num_tentacles; t++) {
                    float tentacle_phase = enemy.animation_phase * 2 + t * 0.8f;
                    int base_x = center_x - dome_size + (t * 2 * dome_size / (num_tentacles - 1));
                    
                    for (int seg = 1; seg <= 7; seg++) { // Longer tentacles
                        float wave = sin(tentacle_phase + seg * 0.6f) * 2.0f; // More wave
                        float spiral = cos(tentacle_phase * 0.7f + seg * 0.3f) * 0.8f; // Spiral motion
                        
                        int tent_x = base_x + (int)(wave + spiral);
                        int tent_y = center_y + seg;
                        
                        // Tentacle color fades towards tips with variation
                        float fade = 1.0f - (seg * 0.12f);
                        uint8_t tent_r = (uint8_t)(r * fade * (0.8f + 0.2f * sin(tentacle_phase)));
                        uint8_t tent_g = (uint8_t)(g * fade * (0.9f + 0.1f * cos(tentacle_phase * 1.3f)));
                        uint8_t tent_b = (uint8_t)(b * fade);
                        
                        graphics.set_pen(graphics.create_pen(tent_r, tent_g, tent_b));
                        
                        if (tent_x >= QIX_FIELD_OFFSET_X && tent_x < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH &&
                            tent_y >= QIX_FIELD_OFFSET_Y && tent_y < QIX_FIELD_OFFSET_Y + QIX_FIELD_HEIGHT) {
                            graphics.pixel(Point(tent_x, tent_y));
                            
                            // Add tentacle thickness for inner segments
                            if (seg <= 4 && tent_x + 1 < QIX_FIELD_OFFSET_X + QIX_FIELD_WIDTH) {
                                graphics.pixel(Point(tent_x + 1, tent_y));
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    
    void drawText(const char* text, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        gfx->set_pen(gfx->create_pen(r, g, b));
        int len = strlen(text);
        for (int i = 0; i < len && x + i < 32; i++) {
            // Simple character display - just show as pixels
            gfx->pixel(Point(x + i, y));
        }
    }
};