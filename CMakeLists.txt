# Option to enable Bluetooth controller support
option(ENABLE_BLUETOOTH "Enable Bluetooth controller support" OFF)

set(OUTPUT_NAME cosmic_launcher)

# Source files
set(LAUNCHER_SOURCES cosmic_launcher.cpp)

# Conditionally add Bluetooth support
if(ENABLE_BLUETOOTH)
    message(STATUS "Bluetooth controller support ENABLED")

    # Import pico-extras for BTstack
    include(pico_extras_import.cmake)

    if(PICO_EXTRAS_PATH)
        # Add Bluetooth source files
        list(APPEND LAUNCHER_SOURCES bluetooth/bluetooth_controller.cpp)

        # Set BTstack config path
        set(BTSTACK_CONFIG_PATH ${CMAKE_CURRENT_LIST_DIR}/bluetooth/config)

        add_executable(${OUTPUT_NAME} ${LAUNCHER_SOURCES})

        target_compile_definitions(${OUTPUT_NAME} PRIVATE
                ENABLE_BLUETOOTH=1
                CYW43_LWIP=0
        )

        target_link_libraries(${OUTPUT_NAME}
                pico_stdlib
                cosmic_unicorn
                pico_graphics
                bitmap_fonts
                pico_btstack_ble
                pico_btstack_classic
                pico_btstack_cyw43
                pico_cyw43_arch_none
                pico_multicore
                )

        target_include_directories(${OUTPUT_NAME} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${BTSTACK_CONFIG_PATH}
                )
    else()
        message(WARNING "PICO_EXTRAS_PATH not found. Building without Bluetooth support.")
        add_executable(${OUTPUT_NAME} cosmic_launcher.cpp)

        target_link_libraries(${OUTPUT_NAME}
                pico_stdlib
                cosmic_unicorn
                pico_graphics
                bitmap_fonts
                pico_cyw43_arch_lwip_threadsafe_background
                )

        target_include_directories(${OUTPUT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
else()
    message(STATUS "Bluetooth controller support DISABLED (use -DENABLE_BLUETOOTH=ON to enable)")

    add_executable(${OUTPUT_NAME} cosmic_launcher.cpp)

    target_link_libraries(${OUTPUT_NAME}
            pico_stdlib
            cosmic_unicorn
            pico_graphics
            bitmap_fonts
            pico_cyw43_arch_lwip_threadsafe_background
            )

    target_include_directories(${OUTPUT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

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
