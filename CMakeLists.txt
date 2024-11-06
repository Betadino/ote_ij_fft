# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.30)

set(SDK_PATH $ENV{SDK_PATH})

# Change your executable name to something creative!
set(NAME picoindiana) # <-- Name your project/executable here!

include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ote_ij_fft C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# Add executable. Default name is the project name, version 0.1

add_executable(
    ${NAME}
    ote_ij_fft.c 
    fft.c 
    kiss_fft.c 
    kiss_fftr.c 
    usb_descriptors.c
    )
    


    target_include_directories(${NAME} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/tinyusb/src 
    ${PICO_SDK_PATH}/lib/tinyusb/src/class/hid 
    ${PICO_SDK_PATH}/lib/tinyusb/src/device 
    ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/rp2040 
    ${PICO_SDK_PATH}/lib/hw/
    )
# Add the standard library to the build
target_link_libraries(
    ${NAME}
    pico_stdlib
    hardware_adc 
    hardware_dma 
    hardware_spi 
    tinyusb_device 
    tinyusb_board
    )




pico_add_extra_outputs(${NAME})

