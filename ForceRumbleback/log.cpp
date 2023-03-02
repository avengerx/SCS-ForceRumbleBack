#include "pch.h"
#include <stdio.h>
#include <share.h>
#include "log.h"

#define LOGDIR "c:\\cygwin\\var\\log\\"
#define LOGFILE "forcerumbleback.log"
#define LOGPATH LOGDIR LOGFILE

void log(const char* const message, ...) {
    va_list args;
    SYSTEMTIME st;
    GetSystemTime(&st);

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
}