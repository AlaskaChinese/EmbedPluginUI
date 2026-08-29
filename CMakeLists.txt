cmake_minimum_required(VERSION 3.16)
project(EmbedPluginUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(epui
    src/canvas.cpp
    src/page.cpp
    src/oled.cpp
    src/widgets.cpp
    src/plugin_registry.cpp
)
target_include_directories(epui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(openoledui ALIAS epui)

option(EPUI_BUILD_WINDOWS_SIM "Build the Win32 OLED simulator" ON)
option(EPUI_BUILD_LINUX_SIM "Build the Linux/X11 OLED simulator" ON)
option(EPUI_BUILD_RPI "Build the Raspberry Pi 5 application" OFF)
option(EPUI_BUILD_TESTS "Build EmbedPluginUI tests" ON)

if(WIN32 AND EPUI_BUILD_WINDOWS_SIM)
    add_executable(epui_sim WIN32 ports/windows/simulator.cpp)
    target_link_libraries(epui_sim PRIVATE epui)
    target_include_directories(epui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

if(UNIX AND NOT APPLE AND EPUI_BUILD_LINUX_SIM)
    find_package(X11 QUIET)
    if(X11_FOUND)
        add_executable(epui_sim ports/linux/simulator.cpp)
        target_link_libraries(epui_sim PRIVATE epui X11::X11)
        target_include_directories(epui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    else()
        message(WARNING "X11 development headers were not found; epui_sim will not be built. On Ubuntu 22.04: sudo apt install libx11-dev")
    endif()
endif()

if(UNIX AND NOT APPLE AND EPUI_BUILD_RPI)
    add_executable(epui_rpi
        ports/raspberry_pi/main.cpp
        ports/raspberry_pi/linux_i2c.cpp
        ports/raspberry_pi/system_monitor.cpp
        ports/raspberry_pi/rpi_pages.cpp
        ports/raspberry_pi/terminal_feed.cpp
    )
    target_link_libraries(epui_rpi PRIVATE epui)
    target_include_directories(epui_rpi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ports/raspberry_pi)
endif()

if(EPUI_BUILD_TESTS)
    enable_testing()

    add_executable(epui_plugin_registry_test tests/plugin_registry_test.cpp)
    target_link_libraries(epui_plugin_registry_test PRIVATE epui)
    add_test(NAME epui_plugin_registry COMMAND epui_plugin_registry_test)

    add_executable(epui_advanced_runtime_test tests/advanced_runtime_test.cpp)
    target_link_libraries(epui_advanced_runtime_test PRIVATE epui)
    add_test(NAME epui_advanced_runtime COMMAND epui_advanced_runtime_test)

    add_executable(epui_public_api_test tests/public_api_test.cpp)
    target_link_libraries(epui_public_api_test PRIVATE epui)
    add_test(NAME epui_public_api COMMAND epui_public_api_test)
endif()
