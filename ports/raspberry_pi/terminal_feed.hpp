#pragma once
#include <array>
#include <cstddef>
#include <string>
namespace epui::rpi {class TerminalFeed{public:static constexpr std::size_t Lines=6,Columns=20;explicit TerminalFeed(std::string path="/tmp/epui-terminal");~TerminalFeed();bool open_feed();void poll();const std::array<std::string,Lines>& lines()const{return lines_;}const std::string& path()const{return path_;}private:void push_line(std::string);static std::string strip_ansi(const std::string&);std::string path_;int fd_{-1};std::string partial_;std::array<std::string,Lines> lines_{};};}
