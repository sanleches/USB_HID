# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(Usb_hid C CXX ASM)
pico_sdk_init()

add_executable(Usb_hid
    main.c
    usb/usb_descriptors.c
    hid/hid_handler.c
    board/button.c
    modes/demo_mode.c
    modes/normal_mode.c
    modes/mode_manager.c
)

pico_set_program_name(Usb_hid "Custom Stream Deck v1")
pico_set_program_version(Usb_hid "1.0.0")

pico_enable_stdio_uart(Usb_hid 1)
pico_enable_stdio_usb(Usb_hid  0)

target_include_directories(Usb_hid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/usb
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/hid
    ${CMAKE_CURRENT_LIST_DIR}/board
    ${CMAKE_CURRENT_LIST_DIR}/modes
)

target_link_libraries(Usb_hid
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_gpio
    hardware_timer
    pico_cyw43_arch_none
)

pico_add_extra_outputs(Usb_hid)
