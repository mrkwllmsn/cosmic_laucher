#ifndef BLUETOOTH_PAIR_GAME_HPP
#define BLUETOOTH_PAIR_GAME_HPP

#include "game_base.hpp"

#ifdef ENABLE_BLUETOOTH
#include "../bluetooth/bluetooth_controller.hpp"
#endif

class BluetoothPairGame : public GameBase {
public:
    void init(PicoGraphics_PenRGB888& graphics, CosmicUnicorn& unicorn) override {
        this->graphics = &graphics;
        this->unicorn = &unicorn;

#ifdef ENABLE_BLUETOOTH
        // Initialize Bluetooth when this menu option is selected
        BluetoothController* bt_controller = getBluetoothController();
        if (bt_controller && bt_controller->init()) {
            status_message = "BT INIT OK";
            printf("[BT PAIR] Bluetooth initialized\n");
        } else {
            status_message = "BT INIT FAIL";
            printf("[BT PAIR] Bluetooth initialization failed\n");
        }
#else
        status_message = "BT DISABLED";
#endif
        frame_count = 0;
    }

    bool update() override {
        if (should_exit) {
            return false;
        }

        frame_count++;

#ifdef ENABLE_BLUETOOTH
        BluetoothController* bt_controller = getBluetoothController();
        if (bt_controller && bt_controller->isConnected()) {
            status_message = "CONNECTED!";
        }
#endif

        // Auto-exit after showing status for a few seconds
        if (frame_count > 60) { // ~3 seconds at 20fps
            return false;
        }

        return true;
    }

    void render(PicoGraphics_PenRGB888& graphics) override {
        graphics.set_pen(graphics.create_pen(0, 0, 0));
        graphics.clear();

        // Display status message
        graphics.set_pen(graphics.create_pen(0, 255, 0));
        graphics.text(status_message, Point(2, 12), 32, 1);
    }

    void handleInput(bool button_a, bool button_b, bool button_c, bool button_d,
                    bool button_vol_up, bool button_vol_down,
                    bool button_bright_up, bool button_bright_down) override {
        // Any button press returns to menu
        if (button_a || button_b || button_c || button_d) {
            should_exit = true;
        }
    }

    void cleanup() override {
        // Nothing to clean up
    }

    const char* getName() const override {
        return "BT PAIR";
    }

    const char* getDescription() const override {
        return "Pair Bluetooth controller";
    }

private:
    PicoGraphics_PenRGB888* graphics = nullptr;
    CosmicUnicorn* unicorn = nullptr;
    std::string status_message = "PAIRING...";
    int frame_count = 0;
    bool should_exit = false;
};

#endif // BLUETOOTH_PAIR_GAME_HPP
