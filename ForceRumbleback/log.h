#ifndef __LOG_H_INCLUDED__
#define __LOG_H_INCLUDED__
#include <mutex>

extern std::mutex logfile_access;
void log(const char* const message, ...);

#endif