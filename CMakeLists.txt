cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME lcd_dev)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Collect LVGL sources
file(GLOB_RECURSE LVGL_SOURCES "Core/App/UI/LVGL/src/*.c")

# Collect UI sources
file(GLOB_RECURSE UI_SOURCES "Core/App/UI/UI/*.c")

# Collect drv sources
file(GLOB_RECURSE DRV_SOURCES "Core/Drv/*.c")

# Collect common sources
file(GLOB_RECURSE COMN_SOURCES "Core/Common/*.c")

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # Core/Common/global.c
    ${COMN_SOURCES}
    # Core/Drv/drv_lcd_touch.c
    # Core/Drv/drv_lcd.c
    # Core/Drv/drv_lvgl_port.c
    # Core/Drv/drv_sys_tick.c
    # Core/Drv/comm_port/drv_i2c.c
    # Core/Drv/comm_port/drv_lwrb.c
    # Core/Drv/comm_port/drv_spi.c
    # Core/Drv/comm_port/drv_uart.c
    ${DRV_SOURCES}
    ${LVGL_SOURCES}
    ${UI_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/Drv
    Core/Drv/comm_port
    Core/Common
    Core/App/UI/LVGL
    Core/App/UI/UI
    Core/App/UI/UI/sdk
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
