cmake_minimum_required(VERSION 3.16)
project(OpenMonoUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(openmonoui
    src/canvas.cpp
    src/page.cpp
    src/oled.cpp
    src/widgets.cpp
)
target_include_directories(openmonoui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compatibility alias while the public headers still use the historical
# openoledui include path/namespace during the v0.1 transition.
add_library(openoledui ALIAS openmonoui)

option(OPENMONOUI_BUILD_WINDOWS_SIM "Build the Win32 OLED simulator" ON)
option(OPENMONOUI_BUILD_LINUX_SIM "Build the Linux/X11 OLED simulator" ON)
option(OPENMONOUI_BUILD_RPI "Build the Raspberry Pi 5 application" OFF)

if(WIN32 AND OPENMONOUI_BUILD_WINDOWS_SIM)
    add_executable(openmonoui_sim WIN32 ports/windows/simulator.cpp)
    target_link_libraries(openmonoui_sim PRIVATE openmonoui)
    target_include_directories(openmonoui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

if(UNIX AND NOT APPLE AND OPENMONOUI_BUILD_LINUX_SIM)
    find_package(X11 QUIET)
    if(X11_FOUND)
        add_executable(openmonoui_sim ports/linux/simulator.cpp)
        target_link_libraries(openmonoui_sim PRIVATE openmonoui X11::X11)
        target_include_directories(openmonoui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    else()
        message(WARNING "X11 development headers were not found; openmonoui_sim will not be built. On Ubuntu: sudo apt install libx11-dev")
    endif()
endif()

if(UNIX AND NOT APPLE AND OPENMONOUI_BUILD_RPI)
    add_executable(openmonoui_rpi
        ports/raspberry_pi/main.cpp
        ports/raspberry_pi/linux_i2c.cpp
        ports/raspberry_pi/system_monitor.cpp
        ports/raspberry_pi/rpi_pages.cpp
        ports/raspberry_pi/terminal_feed.cpp
    )
    target_link_libraries(openmonoui_rpi PRIVATE openmonoui)
    target_include_directories(openmonoui_rpi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ports/raspberry_pi)
endif()
