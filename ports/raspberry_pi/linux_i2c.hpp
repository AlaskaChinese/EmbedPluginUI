#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "epui/oled.hpp"
namespace epui::rpi {
class LinuxI2cTransport final:public OledTransport{public:LinuxI2cTransport(std::string device,std::uint8_t address);~LinuxI2cTransport()override;bool open_bus();bool good()const{return fd_>=0;}bool write_command(const std::uint8_t*,std::size_t)override;bool write_data(const std::uint8_t*,std::size_t)override;void delay_ms(std::uint32_t)override;private:bool write_prefixed(std::uint8_t,const std::uint8_t*,std::size_t);std::string device_;std::uint8_t address_;int fd_{-1};};
}
