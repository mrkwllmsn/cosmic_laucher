#pragma once

#include "pico_graphics.hpp"
#include "cosmic_unicorn.hpp"
#include "pico/time.h"
#include <vector>
#include <cmath>

using namespace pimoroni;

class AnimatedEye {
public:
    enum EyeType {
        OVAL,
        TRIANGLE,
        POINT
    };

    struct EyeConfig {
        float x, y;           // Center position of the eye
        uint8_t r, g, b;      // Color components (start color for POINT type)
        float radiusX;        // Horizontal radius (width/2)
        float radiusY;        // Vertical radius (height/2)
        EyeType type;         // Eye type: OVAL, TRIANGLE, or POINT
        float glow_intensity; // Base glow intensity (0.0 to 1.0)
        int pair_id;          // Which pair this eye belongs to (-1 for independent)
        
        // Backwards compatibility
        bool is_triangle;     // Deprecated: kept for backwards compatibility
    };

    struct EyePairState {
        uint32_t blink_timer;
        uint32_t blink_interval;
        bool is_blinking;
        float blink_phase;          // 0.0 to 1.0 for smooth animation
        bool is_double_blink;       // True for double blink sequence
        int blink_count;           // Count in double blink (0, 1, or 2)
        uint32_t double_blink_delay; // Delay between blinks in double blink
        
        // Simplified pupil movement system
        float pupil_x, pupil_y;     // Current pupil position (-1 to 1, representing movement within eye)
        float pupil_target_x, pupil_target_y; // Target pupil position
        uint32_t pupil_change_timer;
        uint32_t pupil_change_interval;
        float movement_speed;       // How fast pupils move to target (0.0 to 1.0)
        
        // Position repositioning
        uint32_t reposition_timer;
        uint32_t reposition_interval;
        bool is_repositioning;
        uint32_t closed_start_time;
        uint32_t closed_duration;    // How long eyes stay closed during repositioning
        float new_x, new_y;         // New position for both eyes in the pair
        bool position_changed;      // Flag to indicate position has been updated
        bool can_reposition;        // Flag to enable/disable repositioning behavior
        
        // Color fading for POINT type eyes
        float color_fade_phase;     // 0.0 to 1.0 for color fade animation
        uint32_t color_fade_timer;
        uint32_t color_fade_interval;
        bool fading_to_red;         // Direction of fade: true = to red, false = to start color
    };

private:
    std::vector<EyeConfig> eyes;
    std::vector<EyePairState> pair_states;
    PicoGraphics_PenRGB888* gfx;
    int next_pair_id;

public:
    AnimatedEye() : gfx(nullptr), next_pair_id(0) {}

    void init(PicoGraphics_PenRGB888& graphics) {
        gfx = &graphics;
    }

    // Add a synchronized pair of eyes
    void addEyePair(const EyeConfig& left_eye, const EyeConfig& right_eye) {
        // Assign the same pair ID to both eyes
        EyeConfig left = left_eye;
        EyeConfig right = right_eye;
        left.pair_id = next_pair_id;
        right.pair_id = next_pair_id;
        
        eyes.push_back(left);
        eyes.push_back(right);
        
        // Initialize corresponding pair state
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        EyePairState state;
        state.blink_timer = current_time;
        state.blink_interval = 1500 + (rand() % 3500); // 1.5-5 seconds
        state.is_blinking = false;
        state.blink_phase = 0.0f;
        state.is_double_blink = false;
        state.blink_count = 0;
        state.double_blink_delay = 200; // 200ms between blinks in double blink
        
        // Initialize simplified pupil movement system
        state.pupil_x = 0.0f;
        state.pupil_y = 0.0f;
        state.pupil_target_x = 0.0f;
        state.pupil_target_y = 0.0f;
        state.pupil_change_timer = current_time;
        state.pupil_change_interval = 800 + (rand() % 1700); // 0.8-2.5 seconds between moves (faster)
        state.movement_speed = 0.25f; // Faster movement speed for more darting effect
        
        // Initialize repositioning (disabled by default)
        state.reposition_timer = current_time;
        state.reposition_interval = 8000 + (rand() % 7000); // 8-15 seconds
        state.is_repositioning = false;
        state.closed_start_time = 0;
        state.closed_duration = 1000; // 1 second closed
        state.new_x = left_eye.x;
        state.new_y = left_eye.y;
        state.position_changed = false;
        state.can_reposition = false; // Disabled by default
        
        // Initialize color fading for POINT type eyes
        state.color_fade_phase = 0.0f;
        state.color_fade_timer = current_time;
        state.color_fade_interval = 1000 + (rand() % 1500); // 1-2.5 seconds between color changes
        state.fading_to_red = true; // Start by fading to red
        
        pair_states.push_back(state);
        next_pair_id++;
    }
    
    // Add a single independent eye (for backwards compatibility)
    void addEye(const EyeConfig& config) {
        EyeConfig eye = config;
        eye.pair_id = -1; // Independent eye
        eyes.push_back(eye);
        
        // Create a pair state just for this eye
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        EyePairState state;
        state.blink_timer = current_time;
        state.blink_interval = 1500 + (rand() % 3500);
        state.is_blinking = false;
        state.blink_phase = 0.0f;
        state.is_double_blink = false;
        state.blink_count = 0;
        state.double_blink_delay = 200;
        // Initialize simplified pupil movement system
        state.pupil_x = 0.0f;
        state.pupil_y = 0.0f;
        state.pupil_target_x = 0.0f;
        state.pupil_target_y = 0.0f;
        state.pupil_change_timer = current_time;
        state.pupil_change_interval = 800 + (rand() % 1700); // 0.8-2.5 seconds between moves (faster)
        state.movement_speed = 0.25f; // Faster movement speed for more darting effect
        
        // Initialize repositioning for individual eyes (disabled by default)
        state.reposition_timer = current_time;
        state.reposition_interval = 8000 + (rand() % 7000); // 8-15 seconds
        state.is_repositioning = false;
        state.closed_start_time = 0;
        state.closed_duration = 1000; // 1 second closed
        state.new_x = config.x;
        state.new_y = config.y;
        state.position_changed = false;
        state.can_reposition = false; // Disabled by default
        
        // Initialize color fading for POINT type eyes
        state.color_fade_phase = 0.0f;
        state.color_fade_timer = current_time;
        state.color_fade_interval = 1000 + (rand() % 1500); // 1-2.5 seconds between color changes
        state.fading_to_red = true; // Start by fading to red
        
        pair_states.push_back(state);
        next_pair_id++;
    }

    // Add multiple eyes at once
    void addEyes(const std::vector<EyeConfig>& configs) {
        for (const auto& config : configs) {
            addEye(config);
        }
    }

    // Clear all eyes
    void clear() {
        eyes.clear();
        pair_states.clear();
        next_pair_id = 0;
    }

    // Enable repositioning for all eyes (for creepy eyes animation)
    void enableRepositioning() {
        for (auto& state : pair_states) {
            state.can_reposition = true;
        }
    }

    // Disable repositioning for all eyes
    void disableRepositioning() {
        for (auto& state : pair_states) {
            state.can_reposition = false;
        }
    }

    // Update animation states (call once per frame)
    void update() {
        if (!gfx) return;
        
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        for (size_t i = 0; i < pair_states.size(); i++) {
            // Update blinking
            uint32_t elapsed = current_time - pair_states[i].blink_timer;
            
            if (!pair_states[i].is_blinking) {
                // Check if it's time to start a blink
                if (elapsed > pair_states[i].blink_interval) {
                    pair_states[i].is_blinking = true;
                    pair_states[i].blink_timer = current_time;
                    pair_states[i].blink_phase = 0.0f;
                    
                    // 20% chance for double blink
                    if (rand() % 5 == 0) {
                        pair_states[i].is_double_blink = true;
                        pair_states[i].blink_count = 0;
                    } else {
                        pair_states[i].is_double_blink = false;
                        pair_states[i].blink_count = 0;
                    }
                }
            } else {
                // Update blink animation
                uint32_t blink_duration = pair_states[i].is_double_blink ? 120 : 150; // Shorter for double blinks
                
                if (elapsed < blink_duration) {
                    // Smooth easing: fast close, slow open
                    float progress = (float)elapsed / blink_duration;
                    if (progress < 0.3f) {
                        // Fast close (0 to 1)
                        pair_states[i].blink_phase = progress / 0.3f;
                    } else {
                        // Slow open (1 to 0)
                        pair_states[i].blink_phase = 1.0f - ((progress - 0.3f) / 0.7f);
                    }
                } else {
                    // Blink finished
                    pair_states[i].blink_phase = 0.0f;
                    
                    if (pair_states[i].is_double_blink && pair_states[i].blink_count == 0) {
                        // First blink of double blink finished, start delay
                        pair_states[i].blink_count = 1;
                        pair_states[i].blink_timer = current_time;
                        pair_states[i].blink_interval = pair_states[i].double_blink_delay;
                    } else if (pair_states[i].is_double_blink && pair_states[i].blink_count == 1) {
                        // Delay finished, start second blink
                        pair_states[i].blink_count = 2;
                        pair_states[i].blink_timer = current_time;
                        pair_states[i].blink_phase = 0.0f;
                    } else {
                        // All blinks finished, set new random interval
                        pair_states[i].is_blinking = false;
                        pair_states[i].is_double_blink = false;
                        pair_states[i].blink_count = 0;
                        pair_states[i].blink_timer = current_time;
                        pair_states[i].blink_interval = 1500 + (rand() % 4000); // 1.5-5.5 seconds
                    }
                }
            }
            
            // Simplified pupil movement system
            updatePupilMovement(pair_states[i], current_time);
            
            // Update color fading for POINT type eyes
            updateColorFading(pair_states[i], current_time);
            
            // Update repositioning logic (only if enabled)
            if (pair_states[i].can_reposition) {
                uint32_t reposition_elapsed = current_time - pair_states[i].reposition_timer;
                
                if (!pair_states[i].is_repositioning) {
                    // Check if it's time to start repositioning
                    if (reposition_elapsed > pair_states[i].reposition_interval) {
                        pair_states[i].is_repositioning = true;
                        pair_states[i].closed_start_time = current_time;
                        pair_states[i].reposition_timer = current_time;
                        
                        // Find safe position that doesn't overlap with other eyes
                        float safe_x, safe_y;
                        if (findSafePosition(i, safe_x, safe_y)) {
                            pair_states[i].new_x = safe_x;
                            pair_states[i].new_y = safe_y;
                        } else {
                            // Fallback to original random position if no safe spot found
                            pair_states[i].new_x = 4 + (rand() % 20);
                            pair_states[i].new_y = 4 + (rand() % 24);
                        }
                        pair_states[i].position_changed = false;
                    }
                } else {
                    uint32_t closed_elapsed = current_time - pair_states[i].closed_start_time;
                    
                    if (closed_elapsed < pair_states[i].closed_duration) {
                        // Eyes are staying closed - force full blink
                        pair_states[i].blink_phase = 1.0f;
                        pair_states[i].is_blinking = true;
                        
                        // Update positions halfway through closed period if not done yet
                        if (!pair_states[i].position_changed && closed_elapsed > pair_states[i].closed_duration / 2) {
                            updateEyePairPositions(i, pair_states[i].new_x, pair_states[i].new_y);
                            pair_states[i].position_changed = true;
                        }
                    } else {
                        // Closed period finished, allow eyes to open and set new interval
                        pair_states[i].is_repositioning = false;
                        pair_states[i].is_blinking = false;
                        pair_states[i].blink_phase = 0.0f;
                        pair_states[i].reposition_timer = current_time;
                        pair_states[i].reposition_interval = 8000 + (rand() % 7000); // 8-15 seconds
                    }
                }
            }
        }
    }

    // Draw all eyes with current animation states
    void draw(float global_glow_phase = 0.0f) {
        if (!gfx) return;
        
        for (size_t i = 0; i < eyes.size(); i++) {
            // Find the correct pair state for this eye
            int pair_id = eyes[i].pair_id;
            if (pair_id == -1) {
                // Independent eye - use its own state
                if (i < pair_states.size()) {
                    drawSingleEye(eyes[i], pair_states[i], global_glow_phase, i);
                }
            } else {
                // Paired eye - use the pair state
                if (pair_id < (int)pair_states.size()) {
                    drawSingleEye(eyes[i], pair_states[pair_id], global_glow_phase, i);
                }
            }
        }
    }

private:
    // Color fading for POINT type eyes
    void updateColorFading(EyePairState& state, uint32_t current_time) {
        uint32_t elapsed = current_time - state.color_fade_timer;
        
        // Check if it's time to update the fade
        if (elapsed > state.color_fade_interval) {
            state.color_fade_timer = current_time;
            state.color_fade_interval = 1000 + (rand() % 1500); // 1-2.5 seconds
            
            // Toggle fade direction
            state.fading_to_red = !state.fading_to_red;
            state.color_fade_phase = 0.0f;
        } else {
            // Update fade phase (0.0 to 1.0)
            state.color_fade_phase = (float)elapsed / state.color_fade_interval;
            if (state.color_fade_phase > 1.0f) state.color_fade_phase = 1.0f;
        }
    }

    // Simplified pupil movement system
    void updatePupilMovement(EyePairState& state, uint32_t current_time) {
        uint32_t elapsed = current_time - state.pupil_change_timer;
        
        // Check if it's time to pick a new target
        if (elapsed > state.pupil_change_interval) {
            // Generate new target position
            generateNewPupilTarget(state);
            state.pupil_change_timer = current_time;
            state.pupil_change_interval = 800 + (rand() % 1700); // 0.8-2.5 seconds (faster darting)
        }
        
        // Smoothly move toward target
        float dx = state.pupil_target_x - state.pupil_x;
        float dy = state.pupil_target_y - state.pupil_y;
        
        state.pupil_x += dx * state.movement_speed;
        state.pupil_y += dy * state.movement_speed;
    }
    
    void generateNewPupilTarget(EyePairState& state) {
        // Enhanced pupil target generation for more dramatic "looking around" effect
        // Common looking directions with more extreme positions for better darting effect
        int direction = rand() % 9; // Added more directions
        
        switch (direction) {
            case 0: // Center/forward - less likely by using smaller case
                state.pupil_target_x = 0.0f;
                state.pupil_target_y = 0.0f;
                break;
            case 1: // Far Left
                state.pupil_target_x = -1.2f;
                state.pupil_target_y = (rand() % 40 - 20) / 100.0f; // Slight vertical variation
                break;
            case 2: // Far Right
                state.pupil_target_x = 1.2f;
                state.pupil_target_y = (rand() % 40 - 20) / 100.0f; // Slight vertical variation
                break;
            case 3: // Up-left corner
                state.pupil_target_x = -1.0f;
                state.pupil_target_y = -1.0f;
                break;
            case 4: // Up-right corner
                state.pupil_target_x = 1.0f;
                state.pupil_target_y = -1.0f;
                break;
            case 5: // Down-left corner
                state.pupil_target_x = -1.0f;
                state.pupil_target_y = 1.0f;
                break;
            case 6: // Down-right corner
                state.pupil_target_x = 1.0f;
                state.pupil_target_y = 1.0f;
                break;
            case 7: // Straight up
                state.pupil_target_x = (rand() % 40 - 20) / 100.0f; // Slight horizontal variation
                state.pupil_target_y = -1.2f;
                break;
            case 8: // Straight down
                state.pupil_target_x = (rand() % 40 - 20) / 100.0f; // Slight horizontal variation
                state.pupil_target_y = 1.2f;
                break;
        }
        
        // Add more randomness for more natural, less predictable movement
        state.pupil_target_x += (rand() % 30 - 15) / 100.0f; // ±0.15 random offset
        state.pupil_target_y += (rand() % 30 - 15) / 100.0f; // ±0.15 random offset
        
        // Clamp to extended range for more dramatic movement
        state.pupil_target_x = fmax(-1.5f, fmin(1.5f, state.pupil_target_x));
        state.pupil_target_y = fmax(-1.5f, fmin(1.5f, state.pupil_target_y));
    }

    // Helper method to check if a position would overlap with existing eyes
    bool checkCollision(float test_x, float test_y, int exclude_pair_id = -1) {
        const float min_distance = 8.0f; // Minimum distance between eye centers
        
        for (size_t i = 0; i < eyes.size(); i++) {
            // Skip eyes from the same pair (they're moving together)
            if (eyes[i].pair_id == exclude_pair_id) {
                continue;
            }
            
            // Calculate distance to this eye
            float dx = eyes[i].x - test_x;
            float dy = eyes[i].y - test_y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance < min_distance) {
                return true; // Collision detected
            }
            
            // Also check collision with the right eye position (test_x + 4)
            dx = eyes[i].x - (test_x + 4);
            distance = sqrt(dx * dx + dy * dy);
            
            if (distance < min_distance) {
                return true; // Collision with right eye position
            }
        }
        
        return false; // No collision
    }
    
    // Helper method to find a safe position for eye pair repositioning
    bool findSafePosition(int exclude_pair_id, float& safe_x, float& safe_y) {
        const int max_attempts = 50; // Limit attempts to prevent infinite loops
        
        for (int attempt = 0; attempt < max_attempts; attempt++) {
            // Generate random position with margins
            safe_x = 4 + (rand() % 20); // 4 to 23 (leaving margin for pair width)
            safe_y = 4 + (rand() % 24); // 4 to 27
            
            // Check if this position is safe
            if (!checkCollision(safe_x, safe_y, exclude_pair_id)) {
                return true; // Found safe position
            }
        }
        
        return false; // Couldn't find safe position after max attempts
    }

    // Helper method to update the positions of both eyes in a pair
    void updateEyePairPositions(int pair_state_index, float new_x, float new_y) {
        // Find eyes that belong to this pair and update their positions
        for (size_t i = 0; i < eyes.size(); i++) {
            if (eyes[i].pair_id == pair_state_index || 
                (eyes[i].pair_id == -1 && (int)i == pair_state_index)) {
                
                // For paired eyes, maintain relative offset
                if (eyes[i].pair_id == pair_state_index) {
                    // This is a paired eye - determine if it's left or right based on vector order
                    // Since addEyePair() adds left eye first, then right eye,
                    // we need to find which eye in this pair was added first
                    bool is_left_eye = true;
                    for (size_t j = 0; j < i; j++) {
                        if (eyes[j].pair_id == pair_state_index) {
                            // Found an earlier eye with same pair_id, so this must be the right eye
                            is_left_eye = false;
                            break;
                        }
                    }
                    
                    // Set position with eye spacing
                    if (is_left_eye) {
                        eyes[i].x = new_x;
                        eyes[i].y = new_y;
                    } else {
                        eyes[i].x = new_x + 4; // Standard 4-pixel spacing between eyes
                        eyes[i].y = new_y;
                    }
                } else {
                    // Independent eye
                    eyes[i].x = new_x;
                    eyes[i].y = new_y;
                }
            }
        }
    }

    void drawSingleEye(const EyeConfig& config, const EyePairState& state, float global_glow_phase, int eye_index) {
        // Calculate eye openness (0.0 = fully closed, 1.0 = fully open)
        float eye_openness = state.is_blinking ? (1.0f - state.blink_phase) : 1.0f;
        
        if (eye_openness < 0.1f) return; // Don't draw if nearly closed
        
        // Calculate effective radius based on openness
        float effective_radiusY = config.radiusY * eye_openness;
        
        // Main eye color
        Pen eye_pen = gfx->create_pen(config.r, config.g, config.b);
        gfx->set_pen(eye_pen);
        
        // Handle backwards compatibility
        EyeType actualType = config.type;
        if (config.is_triangle && actualType == OVAL) {
            actualType = TRIANGLE;
        }
        
        switch (actualType) {
            case TRIANGLE:
                drawTriangleEye(config, state, effective_radiusY, eye_openness);
                break;
            case POINT:
                drawPointEye(config, state, effective_radiusY, eye_openness);
                break;
            case OVAL:
            default:
                drawOvalEye(config, state, effective_radiusY, eye_openness);
                break;
        }
        
        // Enhanced glow effect - scale with eye openness and global phase
        float glow_intensity = config.glow_intensity * (0.7f + 0.3f * sin(global_glow_phase * 3.0f + eye_index * 1.5f)) * eye_openness;
        Pen glow_pen = gfx->create_pen(
            (uint8_t)(config.r * glow_intensity), 
            (uint8_t)(config.g * glow_intensity), 
            (uint8_t)(config.b * glow_intensity)
        );
        gfx->set_pen(glow_pen);
        
        // Draw glow around the eye (skip for POINT type as it handles its own color)
        
        if (actualType != POINT) {
            // Draw glow around the eye
            drawGlow(config, effective_radiusY, glow_intensity);
            
            // Outer glow for more dramatic effect
            Pen outer_glow = gfx->create_pen(
                (uint8_t)(config.r * glow_intensity * 0.3f), 
                (uint8_t)(config.g * glow_intensity * 0.3f), 
                (uint8_t)(config.b * glow_intensity * 0.3f)
            );
            gfx->set_pen(outer_glow);
            
            // Wider outer glow
            drawOuterGlow(config, effective_radiusY);
        }
    }
    
    void drawTriangleEye(const EyeConfig& config, const EyePairState& state, float effective_radiusY, float eye_openness) {
        int center_x = (int)config.x;
        int center_y = (int)config.y;
        
        // Draw triangle-shaped eyes with vertical scaling based on original code
        // Main triangle pixels
        gfx->pixel({center_x, center_y});
        gfx->pixel({center_x + 1, center_y});
        gfx->pixel({center_x + 2, center_y});
        
        if (eye_openness > 0.6f) {
            gfx->pixel({center_x + 1, center_y - 1});
            gfx->pixel({center_x + 1, center_y + 1});
        }
        
        // Triangle pupils (black dots) - only show when eyes are open enough
        if (eye_openness > 0.4f) {
            gfx->set_pen(gfx->create_pen(0, 0, 0));
            // Enhanced pupil movement - pupils can dart to edges of eye area
            int pupil_x = center_x + 1 + (int)(state.pupil_x * 1.2f); // Increased range for more dramatic movement
            int pupil_y = center_y + (int)(state.pupil_y * 0.8f); // Increased vertical movement
            // Clamp to ensure pupils stay within reasonable bounds
            pupil_x = fmax(center_x, fmin(center_x + 2, pupil_x));
            pupil_y = fmax(center_y - 1, fmin(center_y + 1, pupil_y));
            gfx->pixel({pupil_x, pupil_y});
        }
    }
    
    void drawPointEye(const EyeConfig& config, const EyePairState& state, float effective_radiusY, float eye_openness) {
        int center_x = (int)config.x;
        int center_y = (int)config.y;
        
        // Calculate faded color between start color and red
        uint8_t current_r, current_g, current_b;
        
        if (state.fading_to_red) {
            // Fade from start color to red
            float fade_factor = state.color_fade_phase;
            current_r = config.r + (255 - config.r) * fade_factor;
            current_g = config.g + (0 - config.g) * fade_factor;
            current_b = config.b + (0 - config.b) * fade_factor;
        } else {
            // Fade from red back to start color
            float fade_factor = state.color_fade_phase;
            current_r = 255 + (config.r - 255) * fade_factor;
            current_g = 0 + (config.g - 0) * fade_factor;
            current_b = 0 + (config.b - 0) * fade_factor;
        }
        
        // Clamp values to valid range
        current_r = std::max(0, std::min(255, (int)current_r));
        current_g = std::max(0, std::min(255, (int)current_g));
        current_b = std::max(0, std::min(255, (int)current_b));
        
        // Set the color and draw a single pixel
        gfx->set_pen(gfx->create_pen(current_r, current_g, current_b));
        gfx->pixel({center_x, center_y});
    }
    
    void drawOvalEye(const EyeConfig& config, const EyePairState& state, float effective_radiusY, float eye_openness) {
        int center_x = (int)config.x;
        int center_y = (int)config.y;
        
        // Draw larger round eyes with vertical scaling based on original code
        // Center row always visible when open
        gfx->pixel({center_x, center_y});
        gfx->pixel({center_x + 1, center_y});
        gfx->pixel({center_x + 2, center_y});
        
        // Top and bottom rows only when sufficiently open
        if (eye_openness > 0.6f) {
            gfx->pixel({center_x, center_y + 1});
            gfx->pixel({center_x + 1, center_y + 1});
            gfx->pixel({center_x + 2, center_y + 1});
        }
        
        // Round pupils (black dots in center) - only show when eyes are open enough
        if (eye_openness > 0.4f) {
            gfx->set_pen(gfx->create_pen(0, 0, 0));
            // Enhanced pupil movement - pupils can dart to edges of eye area
            int pupil_x = center_x + 1 + (int)(state.pupil_x * 1.2f); // Increased range for more dramatic movement
            int pupil_y = center_y + (int)(state.pupil_y * 0.8f); // Increased vertical movement
            // Clamp to ensure pupils stay within reasonable bounds for oval eyes
            pupil_x = fmax(center_x, fmin(center_x + 2, pupil_x));
            pupil_y = fmax(center_y, fmin(center_y + 1, pupil_y));
            gfx->pixel({pupil_x, pupil_y});
        }
    }
    
    void drawGlow(const EyeConfig& config, float effective_radiusY, float glow_intensity) {
        int center_x = (int)config.x;
        int center_y = (int)config.y;
        
        if (config.is_triangle) {
            // Triangle glow pattern from original
            gfx->pixel({center_x - 1, center_y});
            gfx->pixel({center_x + 3, center_y});
            gfx->pixel({center_x + 1, center_y - 2});
            gfx->pixel({center_x + 1, center_y + 2});
        } else {
            // Round eye glow pattern from original
            gfx->pixel({center_x - 1, center_y});
            gfx->pixel({center_x - 1, center_y + 1});
            gfx->pixel({center_x + 3, center_y});
            gfx->pixel({center_x + 3, center_y + 1});
            gfx->pixel({center_x, center_y - 1});
            gfx->pixel({center_x + 1, center_y - 1});
            gfx->pixel({center_x + 2, center_y - 1});
            gfx->pixel({center_x, center_y + 2});
            gfx->pixel({center_x + 1, center_y + 2});
            gfx->pixel({center_x + 2, center_y + 2});
        }
    }
    
    void drawOuterGlow(const EyeConfig& config, float effective_radiusY) {
        int center_x = (int)config.x;
        int center_y = (int)config.y;
        
        // Wider outer glow from original
        gfx->pixel({center_x - 2, center_y});
        gfx->pixel({center_x - 2, center_y + 1});
        gfx->pixel({center_x + 4, center_y});
        gfx->pixel({center_x + 4, center_y + 1});
        gfx->pixel({center_x + 1, center_y - 2});
        gfx->pixel({center_x + 1, center_y + 3});
    }
};