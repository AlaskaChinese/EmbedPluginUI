#include "openoledui/widgets.hpp"
#include <algorithm>
#include <cstdio>

namespace openoledui {
void draw_header(Canvas& c, const char* title, int page, int pages) { c.text(3,3,title); char p[12]{}; std::snprintf(p,sizeof(p),"%d/%d",page,pages); c.text(Canvas::Width-c.text_width(p)-3,3,p); c.line(2,12,125,12); }
void draw_metric(Canvas& c, int x, int y, const char* label, const char* value) { c.text(x,y,label); c.text(x,y+9,value); }
void draw_card(Canvas& c, int x, int y, int w, int h, const char* title) { c.round_rect(x,y,w,h,4,true); c.text(x+5,y+4,title); }
void draw_wifi_icon(Canvas& c, int x, int y, int strength) { strength=std::max(0,std::min(3,strength)); c.pixel(x+7,y+9,true); if(strength>=1){c.line(x+4,y+7,x+7,y+4);c.line(x+7,y+4,x+10,y+7);} if(strength>=2){c.line(x+2,y+5,x+7,y);c.line(x+7,y,x+12,y+5);} if(strength>=3){c.pixel(x+1,y+2);c.pixel(x+13,y+2);} }
void draw_thermometer(Canvas& c, int x, int y, float normalized) { normalized=std::max(0.0f,std::min(1.0f,normalized)); c.round_rect(x+2,y,5,12,2,true); c.circle(x+4,y+14,4,true); const int fill=static_cast<int>(normalized*8.0f); c.fill_rect(x+4,y+10-fill,1,fill+3,true); c.fill_rect(x+2,y+12,5,4,true); }
void draw_spinner(Canvas& c, int cx, int cy, std::uint32_t now_ms) { static const int px[8]={0,3,4,3,0,-3,-4,-3}; static const int py[8]={-4,-3,0,3,4,3,0,-3}; const int phase=static_cast<int>((now_ms/90)%8); for(int i=0;i<8;++i){const int d=(i-phase+8)%8; if(d<3)c.fill_rect(cx+px[i]-1,cy+py[i]-1,2,2,true); else c.pixel(cx+px[i],cy+py[i],true);} }
} // namespace openoledui
