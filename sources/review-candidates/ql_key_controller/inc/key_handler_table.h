#include "key_def.h"
#include "power_key_handler.h"
#include "restore_key_handler.h"
#include "wps_key_handler.h"

typedef struct 
{
    const char *name;
    keycallback cb[NUM_OF_EVENT];
} key_handler_data_t;


static key_handler_data_t g_key_handler_table[] = 
{
    {
        "power_key", 
        {
            power_key_press_down_handler,
            power_key_release_handler,
            power_key_long_press_handler
        }
    },

    {
        "restore_key", 
        {
            restore_key_press_down_handler,
            restore_key_release_handler,
            restore_key_long_press_handler
        } 
    },

    {
        "wps_key", 
        {
            wps_key_press_down_handler,
            wps_key_release_handler,
            wps_key_long_press_handler
        }
    },
};



#define g_key_handler_table_start 0
#define g_key_handler_table_size (sizeof(g_key_handler_table) / sizeof(key_handler_data_t))