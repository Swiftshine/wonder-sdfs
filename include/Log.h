#pragma once

#include <hk/diag/diag.h>

#define SHOW_VERBOSE_LOG false
#define SHOW_INFO_LOG true
#define SHOW_ERROR_LOG true

#if SHOW_VERBOSE_LOG
    #define VERBOSE_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - VERBOSE: " #fmt, ##__VA_ARGS__);
#else
    #define VERBOSE_LOG(...)
#endif

#if SHOW_INFO_LOG
    #define INFO_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - INFO: " #fmt, ##__VA_ARGS__);
#else
    #define INFO_LG(...)
#endif

#if SHOW_ERROR_LOG
    #define ERROR_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - ERROR: " #fmt, ##__VA_ARGS__)
#else
    #define ERROR_LOG(...)
#endif
