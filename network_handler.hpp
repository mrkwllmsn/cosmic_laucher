#pragma once

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include <string.h>
#include <stdio.h>
#include "wifi_config.hpp"

struct NetworkButtons {
    bool button_a = false;
    bool button_b = false;
    bool button_c = false;
    bool button_d = false;
    bool button_vol_up = false;
    bool button_vol_down = false;
    bool button_bright_up = false;
    bool button_bright_down = false;
    bool has_new_input = false;
};

class NetworkHandler {
private:
    static NetworkHandler* instance;
    struct udp_pcb* udp_pcb_ptr;
    NetworkButtons network_buttons;
    bool wifi_connected = false;
    
    static void udp_recv_callback(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port) {
        if (instance && p) {
            instance->handle_udp_packet(p);
        }
        if (p) {
            pbuf_free(p);
        }
    }
    
    void handle_udp_packet(struct pbuf* p) {
        if (p->len > 0 && p->len < MAX_PACKET_SIZE) {
            char buffer[MAX_PACKET_SIZE];
            pbuf_copy_partial(p, buffer, p->len, 0);
            buffer[p->len] = '\0';
            
            // Reset all buttons first (only set the one that was pressed)
            network_buttons.button_a = false;
            network_buttons.button_b = false;
            network_buttons.button_c = false;
            network_buttons.button_d = false;
            network_buttons.button_vol_up = false;
            network_buttons.button_vol_down = false;
            network_buttons.button_bright_up = false;
            network_buttons.button_bright_down = false;
            
            // Parse button commands
            if (strcmp(buffer, "A") == 0) {
                network_buttons.button_a = true;
            } else if (strcmp(buffer, "B") == 0) {
                network_buttons.button_b = true;
            } else if (strcmp(buffer, "C") == 0) {
                network_buttons.button_c = true;
            } else if (strcmp(buffer, "D") == 0) {
                network_buttons.button_d = true;
            } else if (strcmp(buffer, "VOL_UP") == 0) {
                network_buttons.button_vol_up = true;
            } else if (strcmp(buffer, "VOL_DOWN") == 0) {
                network_buttons.button_vol_down = true;
            } else if (strcmp(buffer, "BRIGHT_UP") == 0) {
                network_buttons.button_bright_up = true;
            } else if (strcmp(buffer, "BRIGHT_DOWN") == 0) {
                network_buttons.button_bright_down = true;
            }
            
            network_buttons.has_new_input = true;
            printf("Received: %s\n", buffer);
        }
    }
    
public:
    NetworkHandler() : udp_pcb_ptr(nullptr) {
        instance = this;
    }
    
    ~NetworkHandler() {
        if (udp_pcb_ptr) {
            udp_remove(udp_pcb_ptr);
        }
        instance = nullptr;
    }
    
    bool init_wifi(const char* ssid, const char* password) {
        if (cyw43_arch_init()) {
            printf("Failed to initialize WiFi\n");
            return false;
        }
        
        cyw43_arch_enable_sta_mode();
        
        printf("Connecting to WiFi: %s\n", ssid);
        if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("Failed to connect to WiFi\n");
            return false;
        }
        
        printf("WiFi connected!\n");
        printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        
        wifi_connected = true;
        return true;
    }
    
    bool start_udp_server(u16_t port) {
        if (!wifi_connected) {
            printf("WiFi not connected\n");
            return false;
        }
        
        udp_pcb_ptr = udp_new();
        if (!udp_pcb_ptr) {
            printf("Failed to create UDP PCB\n");
            return false;
        }
        
        err_t err = udp_bind(udp_pcb_ptr, IP_ADDR_ANY, port);
        if (err != ERR_OK) {
            printf("Failed to bind UDP port %d, error: %d\n", port, err);
            udp_remove(udp_pcb_ptr);
            udp_pcb_ptr = nullptr;
            return false;
        }
        
        udp_recv(udp_pcb_ptr, udp_recv_callback, nullptr);
        printf("UDP server listening on port %d\n", port);
        return true;
    }
    
    NetworkButtons get_network_buttons() {
        NetworkButtons buttons = network_buttons;
        // Keep network buttons active for one frame cycle - don't clear immediately
        // This ensures the main loop can read them
        network_buttons.has_new_input = false;
        return buttons;
    }
    
    void clear_network_buttons() {
        // Call this after processing network buttons to clear them
        network_buttons.button_a = false;
        network_buttons.button_b = false;
        network_buttons.button_c = false;
        network_buttons.button_d = false;
        network_buttons.button_vol_up = false;
        network_buttons.button_vol_down = false;
        network_buttons.button_bright_up = false;
        network_buttons.button_bright_down = false;
    }
    
    bool is_wifi_connected() const {
        return wifi_connected;
    }
};

// Static member definition
NetworkHandler* NetworkHandler::instance = nullptr;