/**
 * Bluetooth Controller Support for Cosmic Launcher
 *
 * Provides HID gamepad/controller input via Bluetooth Classic
 */

#ifndef BLUETOOTH_CONTROLLER_HPP
#define BLUETOOTH_CONTROLLER_HPP

#include <stdint.h>
#include <stdbool.h>

#ifdef ENABLE_BLUETOOTH

// Button state structure matching Cosmic Unicorn button layout
struct BluetoothButtons {
    bool button_a;
    bool button_b;
    bool button_c;
    bool button_d;
    bool button_vol_up;
    bool button_vol_down;
    bool button_bright_up;
    bool button_bright_down;
    bool button_sleep;

    BluetoothButtons() :
        button_a(false), button_b(false), button_c(false), button_d(false),
        button_vol_up(false), button_vol_down(false),
        button_bright_up(false), button_bright_down(false),
        button_sleep(false) {}
};

class BluetoothController {
public:
    BluetoothController();
    ~BluetoothController();

    // Initialize Bluetooth and start accepting controller connections
    bool init();

    // Update internal state (call regularly from main loop)
    void update();

    // Get current button states
    BluetoothButtons getButtons() const { return buttons; }

    // Check if a controller is connected
    bool isConnected() const { return connected; }

    // Get device name if connected
    const char* getDeviceName() const;

    // BTstack callbacks (public static methods so they can be used by core1 entry)
    static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    static void hid_host_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

private:
    BluetoothButtons buttons;
    bool connected;
    bool initialized;

    // Internal HID packet processing
    void processHIDReport(const uint8_t* report, uint16_t report_len);
    void parseGamepadReport(const uint8_t* report, uint16_t report_len);
};

// Global instance accessor (singleton pattern)
BluetoothController* getBluetoothController();

#else // !ENABLE_BLUETOOTH

// Stub implementation when Bluetooth is disabled
struct BluetoothButtons {
    bool button_a = false;
    bool button_b = false;
    bool button_c = false;
    bool button_d = false;
    bool button_vol_up = false;
    bool button_vol_down = false;
    bool button_bright_up = false;
    bool button_bright_down = false;
    bool button_sleep = false;
};

class BluetoothController {
public:
    bool init() { return false; }
    void update() {}
    BluetoothButtons getButtons() const { return BluetoothButtons(); }
    bool isConnected() const { return false; }
    const char* getDeviceName() const { return ""; }
};

inline BluetoothController* getBluetoothController() { return nullptr; }

#endif // ENABLE_BLUETOOTH

#endif // BLUETOOTH_CONTROLLER_HPP
