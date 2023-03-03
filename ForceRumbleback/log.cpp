#include "pch.h"
#include <stdio.h>
#include <share.h>
#include "log.h"

#define LOGDIR "c:\\cygwin\\var\\log\\"
#define LOGFILE "forcerumbleback.log"
#define LOGPATH LOGDIR LOGFILE

std::mutex logfile_access;

void log(const char* const message, ...) {
    va_list args;
    SYSTEMTIME st;
    GetSystemTime(&st);

    logfile_access.lock();
    FILE* loghandle = _fsopen(LOGPATH, "a", _SH_DENYWR);
    fprintf_s(loghandle, "%04d-%02d-%02d %02d:%02d:%02d.%03d UTC: ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        st.wMilliseconds);

    va_start(args, message);
    vfprintf(loghandle, message, args);
    va_end(args);

    fwrite("\n", 1, 1, loghandle);
    fclose(loghandle);
    logfile_access.unlock();
}

void log(const wchar_t* const message, ...) {
    va_list args;
    SYSTEMTIME st;
    GetSystemTime(&st);

    logfile_access.lock();
    FILE* loghandle = _fsopen(LOGPATH, "a", _SH_DENYWR);
    fwprintf_s(loghandle, L"%04d-%02d-%02d %02d:%02d:%02d.%03d UTC: ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        st.wMilliseconds);

    va_start(args, message);
    vfwprintf(loghandle, message, args);
    va_end(args);

    fwprintf_s(loghandle, L"\n");
    fclose(loghandle);
    logfile_access.unlock();
}