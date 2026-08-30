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

set(EPUI_SIM_UI_SOURCES
    examples/simulator_ui/app.cpp
    examples/simulator_ui/home_page.cpp
    examples/simulator_ui/sensor_page.cpp
    examples/simulator_ui/about_page.cpp
    examples/simulator_ui/menu_demo.cpp
    examples/simulator_ui/terminal_page.cpp
)

option(EPUI_BUILD_WINDOWS_SIM "Build the Win32 OLED simulator" ON)
option(EPUI_BUILD_LINUX_SIM "Build the Linux/X11 OLED simulator" ON)
option(EPUI_BUILD_RPI "Build the Raspberry Pi 5 application" OFF)
option(EPUI_BUILD_RPI_SIM "Build the Raspberry Pi 5 application in the X11 simulator" ON)
option(EPUI_BUILD_TESTS "Build EmbedPluginUI tests" ON)

if(WIN32 AND EPUI_BUILD_WINDOWS_SIM)
    add_executable(epui_sim WIN32
        ports/windows/simulator.cpp
        ${EPUI_SIM_UI_SOURCES}
    )
    target_link_libraries(epui_sim PRIVATE epui Psapi)
    target_include_directories(epui_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples/simulator_ui)
endif()

if(UNIX AND NOT APPLE AND (EPUI_BUILD_LINUX_SIM OR EPUI_BUILD_RPI_SIM))
    find_package(X11 QUIET)
endif()

if(UNIX AND NOT APPLE AND EPUI_BUILD_LINUX_SIM)
    if(X11_FOUND)
        add_executable(epui_sim
            ports/linux/simulator.cpp
            ports/linux/x11_simulator.cpp
            ${EPUI_SIM_UI_SOURCES}
        )
        target_link_libraries(epui_sim PRIVATE epui X11::X11)
        target_include_directories(epui_sim PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/simulator_ui
            ${CMAKE_CURRENT_SOURCE_DIR}/ports/linux
        )
    else()
        message(WARNING "X11 development headers were not found; epui_sim will not be built. On Ubuntu 22.04: sudo apt install libx11-dev")
    endif()
endif()

if(UNIX AND NOT APPLE AND (EPUI_BUILD_RPI OR EPUI_BUILD_RPI_SIM))
    add_subdirectory(apps/rpi5_oled_console)
endif()

if(EPUI_BUILD_TESTS)
    enable_testing()

    add_executable(epui_plugin_registry_test tests/plugin_registry_test.cpp)
    target_link_libraries(epui_plugin_registry_test PRIVATE epui)
    add_test(NAME epui_plugin_registry COMMAND epui_plugin_registry_test)

    add_executable(epui_advanced_runtime_test tests/advanced_runtime_test.cpp)
    target_link_libraries(epui_advanced_runtime_test PRIVATE epui)
    add_test(NAME epui_advanced_runtime COMMAND epui_advanced_runtime_test)

    add_executable(epui_menu_plugin_test tests/menu_plugin_test.cpp)
    target_link_libraries(epui_menu_plugin_test PRIVATE epui)
    add_test(NAME epui_menu_plugin COMMAND epui_menu_plugin_test)

    add_executable(epui_menu_viewport_test tests/menu_viewport_test.cpp)
    target_link_libraries(epui_menu_viewport_test PRIVATE epui)
    add_test(NAME epui_menu_viewport COMMAND epui_menu_viewport_test)

    add_executable(epui_page_transition_test tests/page_transition_test.cpp)
    target_link_libraries(epui_page_transition_test PRIVATE epui)
    add_test(NAME epui_page_transition COMMAND epui_page_transition_test)

    add_executable(epui_fps_debug_test tests/fps_debug_test.cpp)
    target_link_libraries(epui_fps_debug_test PRIVATE epui)
    add_test(NAME epui_fps_debug COMMAND epui_fps_debug_test)

    add_executable(epui_simulator_ui_test
        tests/simulator_ui_test.cpp
        ${EPUI_SIM_UI_SOURCES}
    )
    target_link_libraries(epui_simulator_ui_test PRIVATE epui)
    target_include_directories(epui_simulator_ui_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/examples/simulator_ui)
    add_test(NAME epui_simulator_ui COMMAND epui_simulator_ui_test)

    add_executable(epui_public_api_test tests/public_api_test.cpp)
    target_link_libraries(epui_public_api_test PRIVATE epui)
    add_test(NAME epui_public_api COMMAND epui_public_api_test)

    add_executable(epui_terminal_view_test tests/terminal_view_test.cpp)
    target_link_libraries(epui_terminal_view_test PRIVATE epui)
    add_test(NAME epui_terminal_view COMMAND epui_terminal_view_test)

    if(UNIX AND NOT APPLE)
        add_executable(epui_pty_session_test
            tests/pty_session_test.cpp
            ports/raspberry_pi/pty_session.cpp
        )
        target_link_libraries(epui_pty_session_test PRIVATE epui)
        target_include_directories(epui_pty_session_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/ports/raspberry_pi
        )
        add_test(NAME epui_pty_session COMMAND epui_pty_session_test)

        add_executable(epui_rpi_terminal_page_test
            tests/rpi_terminal_page_test.cpp
            apps/rpi5_oled_console/pages.cpp
            ports/raspberry_pi/pty_session.cpp
            ports/raspberry_pi/system_monitor.cpp
        )
        target_link_libraries(epui_rpi_terminal_page_test PRIVATE epui)
        target_include_directories(epui_rpi_terminal_page_test PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/apps/rpi5_oled_console
            ${CMAKE_CURRENT_SOURCE_DIR}/ports/raspberry_pi
        )
        add_test(NAME epui_rpi_terminal_page COMMAND epui_rpi_terminal_page_test)
    endif()
endif()
