#pragma once
#include "openoledui/page.hpp"
#include "system_monitor.hpp"
#include "terminal_feed.hpp"
namespace openoledui::rpi {class OverviewPage final:public Page{public:explicit OverviewPage(const StatusSnapshot&s):s_(s){}void draw(Canvas&,std::uint32_t)override;private:const StatusSnapshot&s_;};class NetworkPage final:public Page{public:explicit NetworkPage(const StatusSnapshot&s):s_(s){}void draw(Canvas&,std::uint32_t)override;private:const StatusSnapshot&s_;};class PowerPage final:public Page{public:explicit PowerPage(const StatusSnapshot&s):s_(s){}void draw(Canvas&,std::uint32_t)override;private:const StatusSnapshot&s_;};class SystemPage final:public Page{public:explicit SystemPage(const StatusSnapshot&s):s_(s){}void draw(Canvas&,std::uint32_t)override;private:const StatusSnapshot&s_;};class TerminalPage final:public Page{public:explicit TerminalPage(const TerminalFeed&f):f_(f){}void draw(Canvas&,std::uint32_t)override;private:const TerminalFeed&f_;};}
