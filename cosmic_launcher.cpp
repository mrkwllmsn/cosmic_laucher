#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "pico/stdlib.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "cosmic_unicorn.hpp"

#include "menu.hpp"
#include "games/arcade_racer_game.hpp"
#include "games/frogger_game.hpp"
#include "games/tetris_game.hpp"
#include "games/shader_effects_game.hpp"
#include "games/halloween_game.hpp"
#include "games/side_scroller_game.hpp"
#include "games/qix_game.hpp"
#include "wifi_config.hpp"

using namespace pimoroni;

PicoGraphics_PenRGB888 graphics(32, 32, nullptr);
CosmicUnicorn cosmic_unicorn;

enum class LauncherState {
    MENU,
    PLAYING_GAME,
    EXITING_GAME
};

LauncherState current_state = LauncherState::MENU;
GameMenu menu;
GameBase* current_game = nullptr;
uint32_t last_frame_time = 0;
const uint32_t target_frame_time = 50; // 20 FPS

void initializeLauncher() {
    stdio_init_all();
    cosmic_unicorn.init();
    cosmic_unicorn.set_brightness(0.5f);
    
    // Initialize graphics
    graphics.set_pen(graphics.create_pen(0, 0, 0));
    graphics.clear();
    
    // Initialize menu
    menu.init(graphics);
    
    // Add all games to menu
    menu.addGame("SPOOK", "Halloween spookiness", std::make_unique<HalloweenGame>());
    menu.addGame("P-TYPE", "Side-scrolling space shooter", std::make_unique<SideScrollerGame>());
    menu.addGame("RACE", "Fast-paced racing game", std::make_unique<ArcadeRacerGame>());
    menu.addGame("FROG", "Cross roads and rivers", std::make_unique<FroggerGame>());
    menu.addGame("QIX", "Claim territory while avoiding the Qix!", std::make_unique<QixGame>());
    menu.addGame("BLOCKS", "Classic block puzzle", std::make_unique<TetrisGame>());
    menu.addGame("PRETTY", "Visual shader effects", std::make_unique<ShaderEffectsGame>());
}

void readInputs(bool& button_a, bool& button_b, bool& button_c, bool& button_d,
                bool& button_vol_up, bool& button_vol_down, 
                bool& button_bright_up, bool& button_bright_down) {
    // Read physical buttons
    bool physical_a = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_A);
    bool physical_b = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_B);
    bool physical_c = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_C);
    bool physical_d = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_D);
    bool physical_vol_up = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_VOLUME_UP);
    bool physical_vol_down = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_VOLUME_DOWN);
    bool physical_bright_up = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_UP);
    bool physical_bright_down = cosmic_unicorn.is_pressed(CosmicUnicorn::SWITCH_BRIGHTNESS_DOWN);
    button_a = physical_a;
    button_b = physical_b;
    button_c = physical_c;
    button_d = physical_d;
    button_vol_up = physical_vol_up;
    button_vol_down = physical_vol_down;
    button_bright_up = physical_bright_up;
    button_bright_down = physical_bright_down;

    // Read network buttons
    // NetworkButtons network_buttons = network_handler.get_network_buttons();
   /* 
    // Combine physical and network inputs (OR logic)
    button_a = physical_a || network_buttons.button_a;
    button_b = physical_b || network_buttons.button_b;
    button_c = physical_c || network_buttons.button_c;
    button_d = physical_d || network_buttons.button_d;
    button_vol_up = physical_vol_up || network_buttons.button_vol_up;
    button_vol_down = physical_vol_down || network_buttons.button_vol_down;
    button_bright_up = physical_bright_up || network_buttons.button_bright_up;
    button_bright_down = physical_bright_down || network_buttons.button_bright_down;
    
    // Clear network buttons after reading them to prevent them from sticking
    network_handler.clear_network_buttons();
    */
}

void handleBrightnessControls(bool button_bright_up, bool button_bright_down) {
    static bool bright_up_pressed = false;
    static bool bright_down_pressed = false;
    static uint32_t last_brightness_change = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (current_time - last_brightness_change > 200) {
        if (button_bright_up && !bright_up_pressed) {
            cosmic_unicorn.adjust_brightness(0.1f);
            bright_up_pressed = true;
            last_brightness_change = current_time;
        } else if (!button_bright_up) {
            bright_up_pressed = false;
        }
        
        if (button_bright_down && !bright_down_pressed) {
            cosmic_unicorn.adjust_brightness(-0.1f);
            bright_down_pressed = true;
            last_brightness_change = current_time;
        } else if (!button_bright_down) {
            bright_down_pressed = false;
        }
    }
}

void updateLauncher() {
    bool button_a, button_b, button_c, button_d;
    bool button_vol_up, button_vol_down, button_bright_up, button_bright_down;
    
    readInputs(button_a, button_b, button_c, button_d,
               button_vol_up, button_vol_down, button_bright_up, button_bright_down);
    
    // Handle brightness controls globally
    handleBrightnessControls(button_bright_up, button_bright_down);
    
    switch (current_state) {
        case LauncherState::MENU: {
            GameBase* selected_game = menu.update(button_a, button_b, button_c);
            if (selected_game) {
                current_game = selected_game;
                current_game->init(graphics, cosmic_unicorn);
                current_state = LauncherState::PLAYING_GAME;
            }
            break;
        }
        
        case LauncherState::PLAYING_GAME: {
            if (current_game) {
                // Pass input to current game
                current_game->handleInput(button_a, button_b, button_c, button_d,
                                         button_vol_up, button_vol_down,
                                         button_bright_up, button_bright_down);
                
                // Update game state
                bool continue_game = current_game->update();
                
                if (!continue_game) {
                    current_state = LauncherState::EXITING_GAME;
                }
            }
            break;
        }
        
        case LauncherState::EXITING_GAME: {
            if (current_game) {
                current_game->cleanup();
                current_game = nullptr;
            }
            current_state = LauncherState::MENU;
            break;
        }
    }
}

void renderLauncher() {
    switch (current_state) {
        case LauncherState::MENU:
            menu.render(graphics);
            break;
            
        case LauncherState::PLAYING_GAME:
            if (current_game) {
                current_game->render(graphics);
            }
            break;
            
        case LauncherState::EXITING_GAME:
            // Clear screen during transition
            graphics.set_pen(graphics.create_pen(0, 0, 0));
            graphics.clear();
            break;
    }
    
    cosmic_unicorn.update(&graphics);
}

void showSplashScreen() {
    // Show a simple splash screen
    graphics.set_pen(graphics.create_pen(0, 0, 0));
    graphics.clear();
    
    // Draw simple "COSMIC LAUNCHER" text using pixels
    Pen splash_pen = graphics.create_pen(100, 150, 255);
    graphics.set_pen(splash_pen);
    
    // Draw a simple pattern to indicate loading
    for (int i = 0; i < 32; i += 4) {
        for (int j = 0; j < 32; j += 4) {
            if ((i + j) % 8 == 0) {
                graphics.pixel({i, j});
                graphics.pixel({i+1, j});
                graphics.pixel({i, j+1});
                graphics.pixel({i+1, j+1});
            }
        }
    }
    
    cosmic_unicorn.update(&graphics);
    sleep_ms(1000); // Show splash for 1 second
}

int main() {
    stdio_init_all();  // <-- this enables USB serial
    showSplashScreen();
    initializeLauncher();
    
    printf("Cosmic Launcher started! Menu items: %zu\n", menu.getItemCount());
    
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        if (current_time - last_frame_time >= target_frame_time) {
            updateLauncher();
            renderLauncher();
            last_frame_time = current_time;
        }
        
        sleep_ms(10); // Small delay to prevent CPU hogging
    }
    
    return 0;
}
