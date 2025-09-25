#pragma once

#include <vector>
#include <memory>
#include <cstring>
#include <cmath>
#include "pico/stdlib.h"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "libraries/bitmap_fonts/bitmap_fonts.hpp"
#include "libraries/bitmap_fonts/font6_data.hpp"
#include "cosmic_unicorn.hpp"
#include "game_base.hpp"

using namespace pimoroni;

struct MenuItem {
    const char* name;
    const char* description;
    std::unique_ptr<GameBase> game;
    
    MenuItem(const char* n, const char* d, std::unique_ptr<GameBase> g) 
        : name(n), description(d), game(std::move(g)) {}
};

class GameMenu {
private:
    std::vector<MenuItem> menu_items;
    int selected_index = 0;
    bool button_a_pressed = false;
    bool button_vol_up_pressed = false;
    bool button_vol_down_pressed = false;
    uint32_t last_input_time = 0;
    const uint32_t input_debounce_ms = 200;
    
    // Visual properties
    Pen bg_pen, text_pen, selected_pen, title_pen;
    int scroll_offset = 0;
    float time_counter = 0.0f;
    
    void drawText(PicoGraphics_PenRGB888& gfx, const char* text, int x, int y, float scale = 1.0f) {
        gfx.text(text, Point(x, y), -1, scale);
    }
    
    void drawScrollingBackground(PicoGraphics_PenRGB888& gfx) {
        time_counter += 0.1f;
        
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                float wave = sinf((x + y) * 0.3f + time_counter) * 0.3f + 0.7f;
                int intensity = (int)(wave * 40);
                
                Pen bg = gfx.create_pen(intensity, intensity / 2, intensity / 4);
                gfx.set_pen(bg);
                gfx.pixel({x, y});
            }
        }
    }

public:
    GameMenu() {
        last_input_time = to_ms_since_boot(get_absolute_time());
    }
    
    void addGame(const char* name, const char* description, std::unique_ptr<GameBase> game) {
        menu_items.emplace_back(name, description, std::move(game));
    }
    
    void init(PicoGraphics_PenRGB888& gfx) {
        bg_pen = gfx.create_pen(0, 0, 20);
        text_pen = gfx.create_pen(100, 100, 255);
        selected_pen = gfx.create_pen(255, 255, 100);
        title_pen = gfx.create_pen(255, 150, 50);
        
        // Set a readable font
        gfx.set_font(&font6);
    }
    
    // Returns pointer to selected game if one was chosen, nullptr otherwise
    GameBase* update(bool button_a, bool button_vol_up, bool button_vol_down) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        if (current_time - last_input_time < input_debounce_ms) {
            return nullptr;
        }
        
        // Navigation
        if (button_vol_up && !button_vol_up_pressed) {
            button_vol_up_pressed = true;
            selected_index = (selected_index - 1 + menu_items.size()) % menu_items.size();
            last_input_time = current_time;
        } else if (!button_vol_up) {
            button_vol_up_pressed = false;
        }
        
        if (button_vol_down && !button_vol_down_pressed) {
            button_vol_down_pressed = true;
            selected_index = (selected_index + 1) % menu_items.size();
            last_input_time = current_time;
        } else if (!button_vol_down) {
            button_vol_down_pressed = false;
        }
        
        // Selection
        if (button_a && !button_a_pressed) {
            button_a_pressed = true;
            last_input_time = current_time;
            if (selected_index >= 0 && selected_index < (int)menu_items.size()) {
                return menu_items[selected_index].game.get();
            }
        } else if (!button_a) {
            button_a_pressed = false;
        }
        
        return nullptr;
    }
    
    void render(PicoGraphics_PenRGB888& gfx) {
        // Clear screen with animated background
        drawScrollingBackground(gfx);
        
        // Spread menu items across the full 32x32 display
        int item_height = 7;  // Increased spacing for better readability
        int start_y = 2;      // Start near top
        
        for (size_t i = 0; i < menu_items.size() && i < 4; i++) {
            int y_pos = start_y + i * item_height;
            
            if ((int)i == selected_index) {
                gfx.set_pen(selected_pen);
                // Draw selection background rectangle
                for (int x = 0; x < 32; x++) {
                    for (int y = y_pos; y < y_pos + 6; y++) {
                        Pen bg = gfx.create_pen(60, 60, 20);
                        gfx.set_pen(bg);
                        gfx.pixel({x, y});
                    }
                }
                gfx.set_pen(selected_pen);
            } else {
                gfx.set_pen(text_pen);
            }
            
            // Center each game name horizontally
            const char* name = menu_items[i].name;
            int name_width = gfx.measure_text(name, 1.0f);
            int name_x = (32 - name_width) / 2;
            drawText(gfx, name, name_x, y_pos + 1, 1.0f);
        }
    }
    
    size_t getItemCount() const {
        return menu_items.size();
    }
    
    const char* getSelectedGameName() const {
        if (selected_index >= 0 && selected_index < (int)menu_items.size()) {
            return menu_items[selected_index].name;
        }
        return "";
    }
};