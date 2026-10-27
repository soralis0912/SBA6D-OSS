#ifndef _UTIL_LOG_H
#define _UTIL_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


int get_log_level(void);
void set_log_level(int log_level);


#define ENABLE_SYS_LOG  
#define DEBUGLEVEL get_log_level()

#define KEYCTRL_VERSION   "V1.0.0.20220811_beta"

#ifdef ENABLE_SYS_LOG
    #define PRINT(level, fmt, ...) syslog(level, "[KEYCTL]" "[%s][%d]" fmt, __func__, __LINE__, ## __VA_ARGS__);
#else
    #define PRINT(level, fmt, ...) printf("[KEYCTL]" "[%s][%d]" fmt, __func__, __LINE__, ## __VA_ARGS__);
#endif

#define QLOGD(fmt, ...) \
do { \
    if (LOG_DEBUG <= DEBUGLEVEL) \
        PRINT(LOG_DEBUG, fmt, ##__VA_ARGS__); \
} while(0)

#define QLOGI(fmt, ...) \
do { \
    if (LOG_INFO <= DEBUGLEVEL) \
        PRINT(LOG_INFO,fmt, ##__VA_ARGS__); \
} while(0)

#define QLOGW(fmt, ...) \
do { \
    if (LOG_WARNING <= DEBUGLEVEL) \
        PRINT(LOG_WARNING,fmt, ##__VA_ARGS__); \
} while(0)

#define QLOGE(fmt, ...) \
do { \
    if (LOG_ERR <= DEBUGLEVEL) \
        PRINT(LOG_ERR, fmt, ##__VA_ARGS__); \
} while(0)

#endif
