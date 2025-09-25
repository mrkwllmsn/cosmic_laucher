#pragma once

#include "pico/stdlib.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "cosmic_unicorn.hpp"

using namespace pimoroni;

class GameBase {
public:
    virtual ~GameBase() = default;
    
    // Core game lifecycle methods
    virtual void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& cosmic_unicorn) = 0;
    virtual bool update() = 0;  // Returns false if game should exit
    virtual void render(PicoGraphics_PenRGB888& graphics) = 0;
    virtual void cleanup() {}
    
    // Game information
    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
    
    // Input handling
    virtual void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                            bool button_vol_up, bool button_vol_down, 
                            bool button_bright_up, bool button_bright_down) {}
    
    // Check if the exit button combination is pressed (D button long press)
    bool checkExitCondition(bool button_d) {
        static uint32_t d_press_start = 0;
        static bool d_was_pressed = false;
        
        if (button_d && !d_was_pressed) {
            d_press_start = to_ms_since_boot(get_absolute_time());
            d_was_pressed = true;
        } else if (!button_d && d_was_pressed) {
            d_was_pressed = false;
        } else if (button_d && d_was_pressed) {
            uint32_t press_duration = to_ms_since_boot(get_absolute_time()) - d_press_start;
            if (press_duration > 1000) {  // 1 second long press
                return true;
            }
        }
        
        return false;
    }

protected:
    PicoGraphics_PenRGB888* gfx = nullptr;
    CosmicUnicorn* cosmic = nullptr;
};