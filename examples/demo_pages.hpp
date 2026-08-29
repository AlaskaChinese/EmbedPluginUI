#pragma once
#include "epui/page.hpp"
#include "epui/widgets.hpp"
namespace epui::demo {
class HomePage final:public Page{public:void draw(Canvas& c,std::uint32_t now)override{draw_header(c,"EmbedPluginUI",1,3);c.text(5,18,"PLUGIN-FIRST UI");c.progress_bar(5,31,118,8,0.25f+0.75f*((now%3000)/3000.0f));c.text(5,45,"NEXT: RIGHT / D");draw_spinner(c,115,49,now);}};
class SensorPage final:public Page{public:void draw(Canvas& c,std::uint32_t now)override{draw_header(c,"Sensors",2,3);const float wave=0.5f+0.5f*static_cast<float>((now/25)%100)/100.0f;draw_card(c,3,17,60,38,"TEMP");draw_thermometer(c,9,32,wave);c.text(25,35,"42.6 C");draw_card(c,66,17,59,38,"LOAD");c.progress_bar(72,36,47,7,wave);}};
class AboutPage final:public Page{public:void draw(Canvas& c,std::uint32_t)override{draw_header(c,"About",3,3);c.text(8,20,"128 x 64 MONO");c.text(8,31,"SSD1306 SH1106");c.text(8,42,"C++17 / PLUGINS");}};
}
