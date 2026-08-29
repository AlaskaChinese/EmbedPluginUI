#include "linux_i2c.hpp"
#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <utility>
namespace epui::rpi {
LinuxI2cTransport::LinuxI2cTransport(std::string device,std::uint8_t address):device_(std::move(device)),address_(address){}
LinuxI2cTransport::~LinuxI2cTransport(){if(fd_>=0)::close(fd_);}bool LinuxI2cTransport::open_bus(){fd_=::open(device_.c_str(),O_RDWR);if(fd_<0)return false;if(::ioctl(fd_,I2C_SLAVE,address_)<0){::close(fd_);fd_=-1;return false;}return true;}
bool LinuxI2cTransport::write_prefixed(std::uint8_t control,const std::uint8_t* data,std::size_t size){if(fd_<0||(!data&&size))return false;std::uint8_t packet[129];while(size>0){const std::size_t chunk=std::min<std::size_t>(size,128);packet[0]=control;for(std::size_t i=0;i<chunk;++i)packet[i+1]=data[i];if(::write(fd_,packet,chunk+1)!=static_cast<ssize_t>(chunk+1))return false;data+=chunk;size-=chunk;}return true;}
bool LinuxI2cTransport::write_command(const std::uint8_t* d,std::size_t n){return write_prefixed(0x00,d,n);}bool LinuxI2cTransport::write_data(const std::uint8_t* d,std::size_t n){return write_prefixed(0x40,d,n);}void LinuxI2cTransport::delay_ms(std::uint32_t ms){std::this_thread::sleep_for(std::chrono::milliseconds(ms));}
}
