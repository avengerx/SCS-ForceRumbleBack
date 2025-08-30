#ifndef __LOG_H_INCLUDED__
#define __LOG_H_INCLUDED__
#include <mutex>
#include "scsutil.h"

extern std::mutex logfile_access;
void log(const char* const message, ...);
void log(const wchar_t* const message, ...);
void log_message(const char* const message, ...);
void log_message(const wchar_t* const message, ...);
void log_warning(const char* const message, ...);
void log_error(const char* const message, ...);
extern scs_log_t game_log;

#endif