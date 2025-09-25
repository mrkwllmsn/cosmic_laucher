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
