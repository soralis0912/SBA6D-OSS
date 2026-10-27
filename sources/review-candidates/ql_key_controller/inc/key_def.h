#ifndef KEY_DEF_H
#define KEY_DEF_H

#include "stdint.h"
#include <linux/input.h>
#include <time.h>

#define MAX_NAME_LEN  32
#define DEFAULT_CONFIG_FILE  "/etc/ql_config/ql_keyctrl_cfg"

typedef void (*keycallback)(void*);

typedef enum {
	PRESS_DOWN = 0,
	PRESS_UP = 1,
    LONG_PRESS = 2,
	NUM_OF_EVENT,
	NONE_PRESS
}key_state;

typedef struct keydef_t
{
    char name[MAX_NAME_LEN];
    unsigned int long_press_time;
    uint8_t   input_event;
    int   linux_code;
    key_state   status;
    keycallback cb[NUM_OF_EVENT];

    int fd;
    int  is_opened;
    char dev_name[MAX_NAME_LEN];


    struct input_event event;
    struct timeval press, release;

    struct keydef_t *next;
    int customized_key_hander;
}keydef_t;



#endif