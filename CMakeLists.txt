set(OUTPUT_NAME cosmic_launcher)
add_executable(${OUTPUT_NAME} cosmic_launcher.cpp)

target_link_libraries(${OUTPUT_NAME}
        pico_stdlib
        cosmic_unicorn
        pico_graphics
        bitmap_fonts
        pico_cyw43_arch_lwip_threadsafe_background
        )

target_include_directories(${OUTPUT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable USB output and disable UART output
pico_enable_stdio_usb(${OUTPUT_NAME} 1)
pico_enable_stdio_uart(${OUTPUT_NAME} 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(${OUTPUT_NAME})

# Set up files for the release packages
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.uf2
        DESTINATION .
        )
