# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is the Pimoroni Pico C/C++ library repository containing drivers, libraries, and examples for RP2040-based boards including Cosmic Unicorn, Galactic Unicorn, Badger 2040, Interstate 75, and various breakout boards. The repository provides both C/C++ libraries and MicroPython bindings.

## Build System

This project uses CMake with the Pico SDK. The build system is hierarchical with the main `CMakeLists.txt` at the root, and individual projects in subdirectories.

### Initial Setup Commands
```bash
# From the repository root
mkdir build
cd build
cmake .. -DPICO_BOARD=pico_w  # or pico for non-wireless boards
```

### Building Individual Projects
```bash
# Build specific target (from build directory)
make [target_name]

# Examples:
make cosmic_rainbow
make shader_effects
make cosmic_arcade_racer
make cosmic_frogger
```

### Building All Examples
```bash
# From build directory
make
```

## Deployment

Deploy compiled UF2 files to Pico devices using picotool:
Note: picotool is configured for passwordless sudo in this environment.
```bash
sudo /usr/local/bin/picotool [filename].uf2 -f
```

## Project Architecture

### Directory Structure
- **drivers/** - Low-level hardware drivers for sensors and peripherals
- **libraries/** - High-level libraries for specific boards (cosmic_unicorn, galactic_unicorn, etc.)
- **examples/** - Example projects demonstrating library usage
- **common/** - Shared utilities and common code
- **micropython/** - MicroPython bindings and modules

### Key Libraries
- **cosmic_unicorn** - 32x32 RGB LED matrix with audio capabilities
- **galactic_unicorn** - 53x11 RGB LED matrix
- **pico_graphics** - Graphics framework with multiple pen types (RGB565, RGB888, 1-bit, etc.)
- **badger2040** - E-ink display library
- **interstate75** - HUB75 matrix driver

### Graphics System
The graphics system uses PicoGraphics with different pen types:
- `PicoGraphics_PenRGB888` - 24-bit RGB (highest quality)
- `PicoGraphics_PenRGB565` - 16-bit RGB (balanced)
- `PicoGraphics_Pen1Bit` - Black/white displays

### Important Patterns

#### Pen Management
- Always create new pens instead of updating existing ones in C++
- Use `gfx.create_pen(r, g, b)` rather than `gfx.update_pen(pen, r, g, b)`
- This differs from the MicroPython API where pen updating works reliably

#### HSV to RGB Conversion
Many examples use HSV color space for smooth color transitions:
```cpp
void hsv_to_rgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b);
```

#### Animation Timing
Use consistent frame timing for animations:
```cpp
sleep_ms(50);  // 20 FPS
```

#### Button Handling
Include debouncing for button interactions:
```cpp
if (pressed && !last_pressed) {
    // Button press logic
    sleep_ms(200);  // Debounce delay
}
```

## Common Development Workflows

### Creating New Examples
1. Add source file to appropriate examples subdirectory
2. Update the subdirectory's `CMakeLists.txt` with new executable:
   ```cmake
   add_executable(
     example_name
     example_name.cpp
   )
   target_link_libraries(example_name pico_stdlib hardware_pio hardware_adc hardware_dma pico_graphics [board_library])
   pico_enable_stdio_usb(example_name 1)
   pico_add_extra_outputs(example_name)
   ```

### Working with LED Matrices
- Use `set_pixel(x, y, pen)` for individual pixels
- Call `update()` to push changes to display
- Consider using `clear()` before drawing each frame

### Porting from MicroPython
When porting MicroPython examples to C++:
- Python's pen indices become pen objects in C++
- String slicing operations need manual implementation
- List comprehensions need conversion to loops
- Python's `range()` becomes C++ `for` loops

## Hardware-Specific Notes

### Cosmic Unicorn (32x32 LED Matrix)
- Sample rate: 22050Hz for audio
- Button layout: A, B, C, D, Volume Up/Down, Brightness Up/Down
- Audio capabilities with I2S
- Brightness control built-in

### Build Targets by Hardware
- Cosmic Unicorn examples build to `build/examples/cosmic_unicorn/[target].uf2`
- Galactic Unicorn examples build to `build/examples/galactic_unicorn/[target].uf2`
- Interstate 75 examples build to `build/examples/interstate75/[target].uf2`

## Performance Considerations
- Use `static` arrays for persistent data in animations
- Minimize dynamic memory allocation in tight loops
- Consider using lookup tables for mathematical functions
- Profile using different pen types - RGB888 vs RGB565 vs lower bit depths

## Testing
- No specific test framework configured - manually test on hardware
- Use serial output (`printf`) for debugging via USB
- Test on actual hardware as simulator behavior may differ

## Troubleshooting

### Build Issues
- Ensure PICO_SDK_PATH is set correctly
- Check that git submodules are initialized: `git submodule update --init`
- Verify CMake version is 3.12 or higher

### Graphics Issues
- If colors appear wrong, verify pen creation vs pen updating approach
- For upside-down displays, check coordinate system and array indexing
- Animation flicker may indicate missing `clear()` calls or incorrect timing

## Development Environment
- This repository expects to be built in `/home/mark/Projects/pico-projects/pico-sdk/pimoroni-pico/`
- Build directory should be `/home/mark/Projects/pico-projects/pico-sdk/pimoroni-pico/build/`
- Uses Pico W by default (PICO_BOARD=pico_w)

## Recent Arcade Racer Enhancements

### Key Files Modified
- **`examples/cosmic_unicorn/cosmic_arcade_racer.cpp`** - Original arcade racing game with speed-responsive grass animation and complex road rendering
- **`examples/cosmic_launcher/games/arcade_racer_game.hpp`** - Modularized version with the following enhancements:
  - Fixed grass stripe animation to move with speed changes (lines 1908-1911)
  - Added checkered flag finish line that appears before theme changes (lines 1986-2018)
  - Maintains all original features including multiple themes, tunnels, and scenery

### Grass Animation Fix
The grass stripes now use speed-responsive movement like the original:
```cpp
float grass_frequency = 20.0f * pow(1.0f - perspective, 3);
float grass_movement = distance * 0.01f * (1.0f + speed * 0.02f);
bool useGrass1 = sin(grass_frequency + grass_movement) > 0;
```

### Checkered Flag Feature
- Appears 200 distance units before automatic theme changes
- Spans entire road width with proper perspective scaling
- Uses alternating black/white checkered pattern
- Animated approach from distance to create finish line effect

## Failed Bluetooth Controller Integration Attempt

### Overview
An attempt was made to add Bluetooth HID gamepad support to cosmic_launcher to allow wireless controller input. The integration **failed** due to fundamental conflicts between the cosmic_unicorn library and BTstack's CYW43 initialization requirements. This section documents the attempt so future work can build on these findings.

### Goal
Enable Bluetooth Classic HID gamepad pairing with the Cosmic Unicorn, mapping controller buttons to the device's input system (A, B, C, D, volume, brightness, sleep controls).

### Technical Approach

#### Architecture
- **Conditional compilation**: `ENABLE_BLUETOOTH` CMake option (default OFF)
- **Dual-core design**: BTstack runs on core1 with blocking event loop, main application on core0
- **Hardcoded MAC**: Controller address `E4:17:D8:19:72:66` to avoid complex pairing UI
- **Menu-driven init**: "BT PAIR" menu option to defer Bluetooth initialization until requested

#### Files Created
1. **bluetooth/bluetooth_controller.hpp**
   - Abstraction layer with `BluetoothButtons` structure matching Cosmic Unicorn controls
   - Stub implementation when `ENABLE_BLUETOOTH` is disabled
   - Public static callbacks for BTstack packet handlers

2. **bluetooth/bluetooth_controller.cpp**
   - BTstack HID host implementation
   - Core1 entry point: `btstack_core1_entry()`
   - Button mapping from HID report to Cosmic Unicorn controls:
     ```cpp
     buttons.button_a = (button_bits & 0x0001);  // A/Cross
     buttons.button_b = (button_bits & 0x0002);  // B/Circle
     buttons.button_c = (button_bits & 0x0004);  // X/Square
     buttons.button_d = (button_bits & 0x0008);  // Y/Triangle
     buttons.button_bright_up = (button_bits & 0x1000);    // D-Pad Up
     buttons.button_bright_down = (button_bits & 0x2000);  // D-Pad Down
     buttons.button_vol_up = (button_bits & 0x0010);    // L1/LB
     buttons.button_vol_down = (button_bits & 0x0020);  // R1/RB
     buttons.button_sleep = (button_bits & 0x0200) || (button_bits & 0x0100);
     ```

3. **bluetooth/config/btstack_config.h**
   - BTstack feature configuration (ENABLE_CLASSIC, ENABLE_LE_PERIPHERAL, etc.)
   - Buffer sizes and connection limits

4. **bluetooth/config/FreeRTOSConfig.h**
   - Minimal FreeRTOS configuration required by BTstack

5. **games/bluetooth_pair_game.hpp**
   - Menu option "BT PAIR" to trigger Bluetooth initialization on-demand
   - Displays connection status on LED matrix
   - Auto-exits after showing status

6. **pico_extras_import.cmake**
   - Import script for pico-extras (required for BTstack)

#### CMake Configuration
```cmake
option(ENABLE_BLUETOOTH "Enable Bluetooth controller support" OFF)

if(ENABLE_BLUETOOTH)
    target_link_libraries(${OUTPUT_NAME}
        pico_btstack_ble
        pico_btstack_classic
        pico_btstack_cyw43
        pico_cyw43_arch_none      # Required for manual CYW43 control
        pico_multicore
    )
    target_compile_definitions(${OUTPUT_NAME} PRIVATE
        ENABLE_BLUETOOTH=1
        CYW43_LWIP=0              # Disable lwIP to avoid netif.h dependency
    )
endif()
```

### Evolution of Attempts

#### Attempt 1: Automatic Bluetooth Init at Startup
- **Approach**: Initialize Bluetooth in `initializeLauncher()`
- **Problem**: Hung at "[BT] Initializing Bluetooth controller support..."
- **Cause**: BTstack requires blocking event loop (`btstack_run_loop_execute()`)

#### Attempt 2: Move BTstack to Core1
- **Approach**: Launch BTstack on core1 using `pico_multicore`
- **Problem**: Still hung, no "[BT] BTstack up and running" message
- **Cause**: Incorrect connection method (was trying gap_discoverable, needed hid_host_connect)

#### Attempt 3: Hardcoded MAC Address Connection
- **Approach**: Use `hid_host_connect(remote_addr, ...)` with hardcoded controller MAC
- **Problem**: BTstack event loop never started
- **Cause**: CYW43 arch library conflict

#### Attempt 4: Menu-Driven Initialization
- **Approach**: Defer Bluetooth init until user selects "BT PAIR" menu option
- **Problem**: Device froze when selecting menu option, no console output
- **Cause**: CYW43 initialization conflict with cosmic_unicorn library

#### Attempt 5: Early CYW43 Initialization
- **Approach**: Initialize CYW43 before `cosmic_unicorn.init()`:
  ```cpp
  #ifdef ENABLE_BLUETOOTH
  if (cyw43_arch_init()) {
      printf("Failed to initialize cyw43_arch\n");
  }
  #endif
  cosmic_unicorn.init();
  ```
- **Problem**: **Device completely failed to boot** - black screen, no serial output
- **Result**: Had to rebuild with `ENABLE_BLUETOOTH=OFF` to recover functionality

### Root Cause Analysis

**The fundamental incompatibility**: The CYW43 wireless chip can only be initialized once, but both the cosmic_unicorn library and BTstack need to control it.

- **cosmic_unicorn library expects**: `pico_cyw43_arch_lwip_threadsafe_background`
  - Auto-initializes CYW43 in background
  - Provides lwIP networking stack
  - Works seamlessly for WiFi/network features

- **BTstack requires**: `pico_cyw43_arch_none`
  - Manual CYW43 initialization control
  - No lwIP (conflicts with BTstack's requirements)
  - Necessary for Bluetooth Classic HID Host

- **The conflict**:
  - Can't link both arch libraries simultaneously
  - Using `pico_cyw43_arch_none` breaks cosmic_unicorn's assumptions
  - Manually calling `cyw43_arch_init()` at wrong time breaks LED matrix
  - Using `pico_cyw43_arch_threadsafe_background` doesn't properly start BTstack

### Errors Encountered

1. **Compilation error**: lwip/netif.h not found
   - Fixed by switching to `pico_cyw43_arch_none` and adding `CYW43_LWIP=0`

2. **Compilation error**: Static method access violation
   - Fixed by making `packet_handler` and `hid_host_packet_handler` public

3. **Compilation error**: Abstract class instantiation
   - Fixed by adding `getName()` and `getDescription()` to `BluetoothPairGame`

4. **Runtime error**: Hang at Bluetooth initialization
   - Attempted fix: Multicore execution (partial success)

5. **Runtime error**: BTstack not starting
   - Attempted fix: Changed connection method (no improvement)

6. **Runtime error**: Freeze when selecting BT PAIR menu
   - Attempted fix: Early CYW43 init (made it worse)

7. **Critical runtime error**: Device won't boot (black screen)
   - Recovery: Rebuild without Bluetooth support

### Lessons Learned

1. **CYW43 is a shared resource**: Libraries must coordinate on CYW43 arch selection
2. **BTstack needs blocking execution**: Core1 is necessary but not sufficient
3. **Initialization order matters**: cosmic_unicorn library has implicit CYW43 dependencies
4. **Architecture libraries are mutually exclusive**: Can't mix arch_none with arch_lwip_*

### Possible Future Approaches

If Bluetooth controller support is attempted again, consider:

1. **Rewrite cosmic_unicorn library**: Modify to work with `pico_cyw43_arch_none` and manual CYW43 init
   - High effort, but provides full control
   - Would require testing all cosmic_unicorn functionality

2. **Use BLE HID instead of Classic**:
   - BLE may have different CYW43 requirements
   - Would require BLE-capable gamepad
   - Different button mapping/pairing flow

3. **Separate Pico for Bluetooth forwarding**:
   - Second Pico W acts as Bluetooth-to-UART bridge
   - Main Pico reads controller input via UART/I2C
   - Adds hardware complexity but avoids CYW43 conflict

4. **Use BTstack's poll-based arch**:
   - Investigate `pico_cyw43_arch_poll` as middle ground
   - May provide manual control without breaking cosmic_unicorn
   - Requires testing compatibility

5. **Fork and patch pico-extras**:
   - Create custom arch library that satisfies both requirements
   - Very high effort, maintenance burden

### Files to Reference
- Controller MAC address: `E4:17:D8:19:72:66`
- Reference Bluetooth audio project: `~/Projects/pico-projects/galactic-bluetooth-audio/`
- BTstack HID host example: `pico-extras/src/rp2_common/pico_btstack/example/hid_host_demo.c`

### Conclusion
Bluetooth controller integration is **not feasible** with the current cosmic_unicorn library architecture without significant library modifications. The CYW43 initialization conflict is a fundamental blocker that cannot be resolved through build configuration alone.
