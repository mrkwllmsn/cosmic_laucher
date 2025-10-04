/**
 * Bluetooth Controller Support for Cosmic Launcher
 *
 * Implements HID gamepad/controller input via Bluetooth Classic using BTstack
 */

#include "bluetooth_controller.hpp"

#ifdef ENABLE_BLUETOOTH

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "btstack.h"

#define MAX_ATTRIBUTE_VALUE_SIZE 512

// HID descriptor storage
static uint8_t hid_descriptor_storage[MAX_ATTRIBUTE_VALUE_SIZE];

// Singleton instance
static BluetoothController* g_bluetooth_controller = nullptr;

// Controller MAC address - set to your specific controller
static const char * remote_addr_string = "E4:17:D8:19:72:66";
static bd_addr_t remote_addr;

// BTstack state
static uint16_t hid_host_cid = 0;
static bool hid_host_descriptor_available = false;
static hid_protocol_mode_t hid_host_report_mode = HID_PROTOCOL_MODE_REPORT_WITH_FALLBACK_TO_BOOT;
static char connected_device_name[64] = {0};

// App state
static enum {
    APP_IDLE,
    APP_CONNECTED
} app_state = APP_IDLE;

// HCI event callback
static btstack_packet_callback_registration_t hci_event_callback_registration;

BluetoothController::BluetoothController()
    : connected(false), initialized(false) {
}

BluetoothController::~BluetoothController() {
}

void BluetoothController::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);

    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t status;
    bd_addr_t local_addr;

    switch(hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            gap_local_bd_addr(local_addr);
            printf("[BT] BTstack up and running on %s\n", bd_addr_to_str(local_addr));

            // Start connection to controller
            if (app_state == APP_IDLE) {
                app_state = APP_CONNECTED;
                printf("[BT] Connecting to controller %s...\n", remote_addr_string);
                status = hid_host_connect(remote_addr, hid_host_report_mode, &hid_host_cid);
                if (status != ERROR_CODE_SUCCESS) {
                    printf("[BT] HID host connect failed, status 0x%02x\n", status);
                }
            }
            break;
        default:
            break;
    }
}

void BluetoothController::hid_host_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (!g_bluetooth_controller) return;

    uint8_t status;
    bd_addr_t event_addr;

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)) {
                        case HID_SUBEVENT_CONNECTION_OPENED:
                            status = hid_subevent_connection_opened_get_status(packet);
                            if (status != ERROR_CODE_SUCCESS) {
                                printf("[BT] Connection failed, status 0x%02x\n", status);
                                hid_host_cid = 0;
                                g_bluetooth_controller->connected = false;
                                return;
                            }
                            hid_host_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                            hid_subevent_connection_opened_get_bd_addr(packet, event_addr);
                            snprintf(connected_device_name, sizeof(connected_device_name),
                                    "%s", bd_addr_to_str(event_addr));
                            printf("[BT] HID controller connected: %s (cid: 0x%04x)\n",
                                   connected_device_name, hid_host_cid);
                            g_bluetooth_controller->connected = true;
                            break;

                        case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
                            status = hid_subevent_descriptor_available_get_status(packet);
                            if (status == ERROR_CODE_SUCCESS) {
                                hid_host_descriptor_available = true;
                                printf("[BT] HID descriptor available\n");
                            } else {
                                printf("[BT] HID descriptor not available (status: 0x%02x), using boot protocol\n", status);
                            }
                            break;

                        case HID_SUBEVENT_REPORT:
                            // Process HID report
                            g_bluetooth_controller->processHIDReport(
                                hid_subevent_report_get_report(packet),
                                hid_subevent_report_get_report_len(packet)
                            );
                            break;

                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            printf("[BT] HID controller disconnected\n");
                            hid_host_cid = 0;
                            hid_host_descriptor_available = false;
                            g_bluetooth_controller->connected = false;
                            connected_device_name[0] = 0;
                            break;

                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void BluetoothController::processHIDReport(const uint8_t* report, uint16_t report_len) {
    // Basic gamepad report parsing
    // Most gamepads send: [buttons_low, buttons_high, left_x, left_y, right_x, right_y, ...]
    if (report_len < 2) return;

    parseGamepadReport(report, report_len);
}

void BluetoothController::parseGamepadReport(const uint8_t* report, uint16_t report_len) {
    // Standard gamepad button mapping (works with most controllers)
    // Byte 0-1: Button bits
    // Common button masks:
    // Bit 0: A/Cross
    // Bit 1: B/Circle
    // Bit 2: X/Square
    // Bit 3: Y/Triangle
    // Bit 4: L1/LB
    // Bit 5: R1/RB
    // Bit 6: L2/LT
    // Bit 7: R2/RT
    // Bit 8: Select/Back
    // Bit 9: Start
    // Bit 12: D-Pad Up
    // Bit 13: D-Pad Down
    // Bit 14: D-Pad Left
    // Bit 15: D-Pad Right

    if (report_len < 6) return;

    uint16_t button_bits = report[0] | (report[1] << 8);

    // Map gamepad buttons to Cosmic Unicorn controls
    buttons.button_a = (button_bits & 0x0001);  // A/Cross
    buttons.button_b = (button_bits & 0x0002);  // B/Circle
    buttons.button_c = (button_bits & 0x0004);  // X/Square
    buttons.button_d = (button_bits & 0x0008);  // Y/Triangle

    // D-Pad for brightness (up/down)
    buttons.button_bright_up = (button_bits & 0x1000);    // D-Pad Up
    buttons.button_bright_down = (button_bits & 0x2000);  // D-Pad Down

    // Shoulder buttons for volume
    buttons.button_vol_up = (button_bits & 0x0010);    // L1/LB
    buttons.button_vol_down = (button_bits & 0x0020);  // R1/RB

    // Start/Select/Home for menu/sleep
    buttons.button_sleep = (button_bits & 0x0200) || (button_bits & 0x0100);  // Start or Select

    // Alternative: Use analog stick for menu navigation if available
    // Bytes 2-3: Left stick X/Y (0x80 = center)
    if (report_len >= 4) {
        int8_t left_y = report[3] - 0x80;

        // Use stick for brightness control as alternative
        if (left_y < -64) {
            buttons.button_bright_up = true;
        } else if (left_y > 64) {
            buttons.button_bright_down = true;
        }
    }
}

// BTstack runs on core1
static void btstack_core1_entry() {
    printf("[BT] Starting BTstack on core1...\n");

    // CYW43 should already be initialized on core0
    // Parse controller MAC address
    sscanf_bd_addr(remote_addr_string, remote_addr);

    // Initialize L2CAP
    l2cap_init();

#ifdef ENABLE_BLE
    // Initialize LE Security Manager. Needed for cross-transport key derivation
    sm_init();
#endif

    // Initialize HID Host
    hid_host_init(hid_descriptor_storage, sizeof(hid_descriptor_storage));
    hid_host_register_packet_handler(BluetoothController::hid_host_packet_handler);

    // Allow sniff mode and role switch for better power management
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);

    // Try to become master on incoming connections
    hci_set_master_slave_policy(HCI_ROLE_MASTER);

    // Register for HCI events
    hci_event_callback_registration.callback = &BluetoothController::packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);

    printf("[BT] BTstack initialized, running event loop...\n");

    // Run BTstack main loop (blocking, but on core1)
    btstack_run_loop_execute();
}

bool BluetoothController::init() {
    if (initialized) return true;

    printf("[BT] Initializing Bluetooth controller support...\n");

    // CYW43 should already be initialized at startup
    g_bluetooth_controller = this;

    // Launch BTstack on core1
    multicore_launch_core1(btstack_core1_entry);

    initialized = true;

    printf("[BT] Bluetooth controller running on core1\n");
    printf("[BT] Waiting for controller to connect...\n");

    return true;
}

void BluetoothController::update() {
    // BTstack runs asynchronously, no polling needed
    // Button states are updated in packet handler callbacks
}

const char* BluetoothController::getDeviceName() const {
    return connected_device_name;
}

BluetoothController* getBluetoothController() {
    static BluetoothController instance;
    return &instance;
}

#endif // ENABLE_BLUETOOTH
