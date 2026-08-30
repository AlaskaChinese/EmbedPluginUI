#pragma once

#include <cstddef>

namespace epui::rpi::console {

bool complete_shell_token(const char* command, std::size_t length,
                          std::size_t cursor, char* output,
                          std::size_t output_capacity,
                          std::size_t& output_length,
                          std::size_t& output_cursor);

} // namespace epui::rpi::console
