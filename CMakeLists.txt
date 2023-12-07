
# What CMake to start at
cmake_minimum_required(VERSION 3.12)
set(PICO_SDK_PATH "/Users/wray/Repos/rpi/pico-sdk")
# Include the subsidiary .cmake file to get the SDK
include(pico_sdk_import.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR})
# Set the name and version of the project

project(MCP VERSION 1.0.0)
 
# Link the Project to a source file (step 4.6)
add_executable(MCP
MCP_DACstuff.cpp
MCP_RP2040_Board.cpp
MCP_RP2040_Main.cpp
MCP_TimerInt.cpp)
#oops - this may have to be upgraded to a faster driver
set(PICO_BOARD "board_MCP")
# Link the Project to an extra library (pico_stdlib)
target_link_libraries(MCP pico_stdlib pico_printf hardware_pio hardware_adc pico_multicore hardware_i2c hardware_dma hardware_exception)
 
# Initalise the SDK
pico_sdk_init()
pico_generate_pio_header(MCP ${CMAKE_CURRENT_LIST_DIR}/spi.pio)
pico_generate_pio_header(MCP ${CMAKE_CURRENT_LIST_DIR}/timerint.pio)
pico_generate_pio_header(MCP ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
 
# Enable USB, UART output
# pico_enable_stdio_usb(Wobbler2 1)
# pico_enable_stdio_uart(Wobbler2 1)
 
# Enable extra outputs (SWD?)
pico_add_extra_outputs(MCP)

