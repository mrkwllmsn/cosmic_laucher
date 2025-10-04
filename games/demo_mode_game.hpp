#pragma once

#include "../game_base.hpp"
#include "pico/stdlib.h"
#include <vector>

class DemoModeGame : public GameBase {
private:
    std::vector<GameBase*> demo_games;
    int current_demo_index = 0;
    uint32_t demo_start_time = 0;
    uint32_t demo_duration_ms = 30000; // 30 seconds default
    GameBase* current_demo_game = nullptr;
    bool demo_game_initialized = false;
    bool running = true;
    PicoGraphics_PenRGB888* gfx_ptr = nullptr;
    CosmicUnicorn* cu_ptr = nullptr;

public:
    DemoModeGame() {}

    const char* getName() const override {
        return "DEMO";
    }

    const char* getDescription() const override {
        return "Cycle through all games";
    }

    void setDemoGames(const std::vector<GameBase*>& games) {
        demo_games = games;
    }

    void setDemoDuration(uint32_t seconds) {
        demo_duration_ms = seconds * 1000;
    }

    void init(PicoGraphics_PenRGB888& gfx, CosmicUnicorn& cu) override {
        gfx_ptr = &gfx;
        cu_ptr = &cu;
        current_demo_index = 0;
        demo_start_time = to_ms_since_boot(get_absolute_time());
        demo_game_initialized = false;
        running = true;

        if (!demo_games.empty()) {
            startCurrentDemoGame();
        }
    }

    void startCurrentDemoGame() {
        if (current_demo_index >= 0 && current_demo_index < (int)demo_games.size()) {
            current_demo_game = demo_games[current_demo_index];
            if (current_demo_game && gfx_ptr && cu_ptr) {
                current_demo_game->init(*gfx_ptr, *cu_ptr);
                demo_game_initialized = true;
                demo_start_time = to_ms_since_boot(get_absolute_time());
            }
        }
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down,
                    bool button_bright_up, bool button_bright_down) override {
        // Button D exits demo mode
        static bool button_d_pressed = false;
        if (button_d && !button_d_pressed) {
            button_d_pressed = true;
            running = false;
            return;
        } else if (!button_d) {
            button_d_pressed = false;
        }

        // Pass input to current demo game
        if (demo_game_initialized && current_demo_game) {
            current_demo_game->handleInput(button_a, button_b, button_c, button_d,
                                          button_vol_up, button_vol_down,
                                          button_bright_up, button_bright_down);
        }
    }

    bool update() override {
        if (demo_games.empty()) {
            return false;
        }

        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Check if it's time to switch to next game
        if (current_time - demo_start_time >= demo_duration_ms) {
            // Cleanup current demo game
            if (demo_game_initialized && current_demo_game) {
                current_demo_game->cleanup();
                demo_game_initialized = false;
            }

            // Move to next game
            current_demo_index = (current_demo_index + 1) % demo_games.size();
            startCurrentDemoGame();
        }

        // Update current demo game
        if (demo_game_initialized && current_demo_game) {
            // Don't care about return value - we control when to exit
            current_demo_game->update();
        }

        return running;
    }

    void render(PicoGraphics_PenRGB888& gfx) override {
        if (demo_game_initialized && current_demo_game) {
            current_demo_game->render(gfx);
        } else {
            // Fallback: black screen
            gfx.set_pen(gfx.create_pen(0, 0, 0));
            gfx.clear();
        }
    }

    void cleanup() override {
        if (demo_game_initialized && current_demo_game) {
            current_demo_game->cleanup();
            demo_game_initialized = false;
        }
        current_demo_game = nullptr;
    }
};
