cmake_minimum_required(VERSION 3.16)
project(OpenOledUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(openoledui
    src/canvas.cpp
    src/page.cpp
    src/oled.cpp
    src/widgets.cpp
)
target_include_directories(openoledui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(OPENOLEDUI_BUILD_WINDOWS_SIM "Build the Win32 OLED simulator" ON)
option(OPENOLEDUI_BUILD_LINUX_SIM "Build the Linux/X11 OLED simulator" ON)
option(OPENOLEDUI_BUILD_RPI "Build the Raspberry Pi 5 application" OFF)

if(WIN32 AND OPENOLEDUI_BUILD_WINDOWS_SIM)
    add_executable(openoledui_sim WIN32 ports/windows/simulator.cpp)
    target_link_libraries(openoledui_sim PRIVATE openoledui)
    target_include_directories(openoledui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

if(UNIX AND NOT APPLE AND OPENOLEDUI_BUILD_LINUX_SIM)
    find_package(X11 QUIET)
    if(X11_FOUND)
        add_executable(openoledui_sim ports/linux/simulator.cpp)
        target_link_libraries(openoledui_sim PRIVATE openoledui X11::X11)
        target_include_directories(openoledui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    else()
        message(WARNING "X11 development headers were not found; openoledui_sim will not be built. On Ubuntu: sudo apt install libx11-dev")
    endif()
endif()

if(UNIX AND NOT APPLE AND OPENOLEDUI_BUILD_RPI)
    add_executable(openoledui_rpi
        ports/raspberry_pi/main.cpp
        ports/raspberry_pi/linux_i2c.cpp
        ports/raspberry_pi/system_monitor.cpp
        ports/raspberry_pi/rpi_pages.cpp
        ports/raspberry_pi/terminal_feed.cpp
    )
    target_link_libraries(openoledui_rpi PRIVATE openoledui)
    target_include_directories(openoledui_rpi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ports/raspberry_pi)
endif()
