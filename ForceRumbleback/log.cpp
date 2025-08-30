#include "pch.h"
#include <stdio.h>
#include <share.h>
#include "log.h"

#undef LOG_TO_FILE

#if LOG_TO_FILE
#define LOGDIR "c:\\cygwin\\var\\log\\"
#define LOGFILE "forcerumbleback.log"
#define LOGPATH LOGDIR LOGFILE

std::mutex logfile_access;
#endif

const char* scs_log_prefix = "ForceRumbleBack: ";

void log(const char* const message, ...) {
#ifdef LOG_TO_FILE
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
#endif
}

void log(const wchar_t* const message, ...) {
#ifdef LOG_TO_FILE
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
#endif
}

char* to_char(const char* const str, va_list args) {
    size_t size;
    char* catenated_str;
    char* prefixed_str;

    if (!str) {
        size = strlen(scs_log_prefix) + 1;
        catenated_str = (char*)malloc(size * sizeof(char));
        if (!catenated_str) throw std::runtime_error("No memory available to allocate log message prefix alone.");
        strcpy_s(catenated_str, size, scs_log_prefix);
        return catenated_str;
    }

    size = _vscprintf(str, args) + 1;
    catenated_str = (char*)malloc(size * sizeof(char));

    if (!catenated_str) throw std::runtime_error("No memory available to allocate base log message.");

    vsprintf_s(catenated_str, size, str, args);

    size = strlen(scs_log_prefix) + strlen(catenated_str) + 1;
    prefixed_str = (char*)malloc(size * sizeof(char));
    if (!prefixed_str) throw std::runtime_error("No memory available to allocate prefixed log message.");
    strcpy_s(prefixed_str, size, scs_log_prefix);
    strcat_s(prefixed_str, size, catenated_str);
    free(catenated_str);
    return prefixed_str;
}

char* to_char(const wchar_t* str, va_list args) {
    errno_t err;
    size_t size;
    wchar_t* wcatenated_str;
    char* catenated_str;
    char* prefixed_str;

    if (!str) {
        size = strlen(scs_log_prefix) + 1;
        catenated_str = (char*)malloc(size * sizeof(char));
        if (!catenated_str) throw std::runtime_error("No memory available to allocate log message prefix alone.");
        strcpy_s(catenated_str, size, scs_log_prefix);
        return catenated_str;
    }

    size = _vscwprintf(str, args) + 1; // +1 -> include the terminating NULL character
    wcatenated_str = (wchar_t*)malloc(size * sizeof(wchar_t));
    if (!wcatenated_str) throw std::runtime_error("No memory available to allocate the wide character version of the log message.");
    vswprintf_s(wcatenated_str, size, str, args);

    wcstombs_s(&size, nullptr, 0, wcatenated_str, 0);
    if (size < 1) throw std::runtime_error("Error inferring the size of the destination multibyte character variable.");

    catenated_str = (char*)malloc(size);
    if (!catenated_str) throw std::runtime_error("No memory available to allocate the multibyte character version of the log message.");
    err = wcstombs_s(nullptr, catenated_str, size, wcatenated_str, size);
    free(wcatenated_str);

    if (err) throw std::runtime_error("Error converting wide character message to multibyte character message.");

    size = strlen(scs_log_prefix) + strlen(catenated_str) + 1;
    prefixed_str = (char*)malloc(size * sizeof(char));
    if (!prefixed_str) throw std::runtime_error("No memory available to allocate prefixed log message.");
    strcpy_s(prefixed_str, size, scs_log_prefix);
    strcat_s(prefixed_str, size, catenated_str);
    free(catenated_str);
    return prefixed_str;
}

void log_message(const char* const message, ...) {
    va_list args;
    char* gamelog_msg;
    va_start(args, message);
    log(message, args);
    if (game_log) {
        gamelog_msg = to_char(message, args);
        game_log(SCS_LOG_TYPE_message, gamelog_msg);
        free(gamelog_msg);
    }
    va_end(args);
}

void log_message(const wchar_t* const message, ...) {
    va_list args;
    char* gamelog_msg;
    va_start(args, message);
    log(message, args);
    if (game_log) {
        gamelog_msg = to_char(message, args);
        game_log(SCS_LOG_TYPE_message, gamelog_msg);
        free(gamelog_msg);
    }
    va_end(args);
}

void log_warning(const char* const message, ...) {
    va_list args;
    char* gamelog_msg;
    va_start(args, message);
    log(message, args);
    if (game_log) {
        gamelog_msg = to_char(message, args);
        game_log(SCS_LOG_TYPE_warning, gamelog_msg);
        free(gamelog_msg);
    }
    va_end(args);

}

void log_error(const char* const message, ...) {
    va_list args;
    char* gamelog_msg;
    va_start(args, message);
    log(message, args);
    if (game_log) {
        gamelog_msg = to_char(message, args);
        game_log(SCS_LOG_TYPE_error, gamelog_msg);
        free(gamelog_msg);
    }
    va_end(args);
}