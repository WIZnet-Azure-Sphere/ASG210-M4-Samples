# This code is based on a sample from Microsoft (see license below),
# with modifications made by MediaTek.
# Modified version of CMakeLists.txt from Microsoft Azure Sphere sample code:
# https://github.com/Azure/azure-sphere-samples/blob/master/Samples/HelloWorld/HelloWorld_RTApp_MT3620_BareMetal/CMakeLists.txt

#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

# Configurations
project(BARE_METAL_RTAPP_DHCP_SERVER C)

azsphere_configure_tools(TOOLS_REVISION "20.07")

add_compile_definitions(OSAI_BARE_METAL)
add_compile_definitions(OSAI_ENABLE_DMA)
add_link_options(-specs=nano.specs -specs=nosys.specs)

# Executable
add_executable(${PROJECT_NAME}
               main.c
               ../../OS_HAL/src/os_hal_uart.c
               ../../OS_HAL/src/os_hal_gpio.c
               ../../OS_HAL/src/os_hal_dma.c
               ../../OS_HAL/src/os_hal_gpt.c
               ../../OS_HAL/src/os_hal_spim.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Ethernet/W5500/W5500.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Ethernet/wizchip_conf.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Ethernet/socket.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Internet/DHCP/dhcps.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Internet/SNTP/sntps.c
               ../../../WIZnet_Driver/ioLibrary_Driver/Application/loopback/loopback.c
               )

# Include Folders
target_include_directories(${PROJECT_NAME} PUBLIC
                           ../../OS_HAL/inc
                           ../../../WIZnet_Driver
                           ./)

# Libraries
add_subdirectory(../../../MT3620_M4_Driver ./lib/MT3620_M4_Driver)
target_link_libraries(${PROJECT_NAME} MT3620_M4_Driver)

# Linker, Image
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)
azsphere_target_add_image_package(${PROJECT_NAME})
