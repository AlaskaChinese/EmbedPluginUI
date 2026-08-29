#include "terminal_feed.hpp"
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
namespace epui::rpi {TerminalFeed::TerminalFeed(std::string p):path_(std::move(p)){}TerminalFeed::~TerminalFeed(){if(fd_>=0)::close(fd_);}bool TerminalFeed::open_feed(){if(::mkfifo(path_.c_str(),0666)<0&&errno!=EEXIST)return false;fd_=::open(path_.c_str(),O_RDWR|O_NONBLOCK);return fd_>=0;}std::string TerminalFeed::strip_ansi(const std::string& s){std::string out;bool esc=false;for(char ch:s){if(!esc&&ch=='\x1b'){esc=true;continue;}if(esc){if(ch>='@'&&ch<='~')esc=false;continue;}if(ch>=' '&&ch<='~')out.push_back(ch);}return out;}void TerminalFeed::push_line(std::string line){line=strip_ansi(line);if(line.size()>Columns)line=line.substr(line.size()-Columns);for(std::size_t i=1;i<Lines;++i)lines_[i-1]=std::move(lines_[i]);lines_[Lines-1]=std::move(line);}void TerminalFeed::poll(){if(fd_<0)return;char b[256];ssize_t n;while((n=::read(fd_,b,sizeof(b)))>0){partial_.append(b,static_cast<std::size_t>(n));std::size_t p;while((p=partial_.find('\n'))!=std::string::npos){push_line(partial_.substr(0,p));partial_.erase(0,p+1);}}}}
