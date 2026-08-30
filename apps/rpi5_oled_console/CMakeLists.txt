if(EPUI_BUILD_RPI)
    add_executable(epui_rpi
        main.cpp
        pages.cpp
        shell_completion.cpp
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/evdev_input.cpp
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/linux_i2c.cpp
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/pty_session.cpp
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/system_monitor.cpp
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/terminal_feed.cpp
    )

    target_link_libraries(epui_rpi PRIVATE epui)
    target_include_directories(epui_rpi PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/ports/raspberry_pi
    )
    set_target_properties(epui_rpi PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(EPUI_BUILD_RPI_SIM)
    if(X11_FOUND)
        add_executable(epui_rpi_sim
            simulator_main.cpp
            pages.cpp
            shell_completion.cpp
            ${PROJECT_SOURCE_DIR}/ports/linux/x11_simulator.cpp
            ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/pty_session.cpp
            ${PROJECT_SOURCE_DIR}/ports/raspberry_pi/system_monitor.cpp
        )
        target_link_libraries(epui_rpi_sim PRIVATE epui X11::X11)
        target_include_directories(epui_rpi_sim PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/ports/linux
            ${PROJECT_SOURCE_DIR}/ports/raspberry_pi
        )
        set_target_properties(epui_rpi_sim PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    else()
        message(WARNING "X11 development headers were not found; epui_rpi_sim will not be built")
    endif()
endif()
