#include <stdio.h>
#include <stdlib.h>
#include "key_callback.h"

static void custom_wps_key_press_down_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_wps_key_press_down_handler in.\" > /dev/console");
}

static void custom_wps_key_release_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_wps_key_release_handler in.\" > /dev/console");
}

static void custom_wps_key_long_press_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_wps_key_long_press_handler in.\" > /dev/console");
}

static void custom_restore_key_press_down_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_restore_key_press_down_handler in.\" > /dev/console");
}

static void custom_restore_key_release_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_restore_key_release_handler in.\" > /dev/console");
}

static void custom_restore_key_long_press_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_restore_key_long_press_handler in.\" > /dev/console");
}

static void custom_power_key_press_down_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_power_key_press_down_handler in.\" > /dev/console");
}

static void custom_power_key_release_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_power_key_release_handler in.\" > /dev/console");
}

static void custom_power_key_long_press_handler(void* priv)
{
    printf("%s() in.\n", __func__);
    system("echo \"custom_power_key_long_press_handler in.\" > /dev/console");
}


key_handler_data_t g_custom_key_handler_table[] =
{
    {
        "wps_key",
        {
            custom_wps_key_press_down_handler,
            custom_wps_key_release_handler,
            custom_wps_key_long_press_handler
        }
    },
    {
        "restore_key",
        {
            custom_restore_key_press_down_handler,
            custom_restore_key_release_handler,
            custom_restore_key_long_press_handler
        }
    },
    {
        "power_key",
        {
            custom_power_key_press_down_handler,
            custom_power_key_release_handler,
            custom_power_key_long_press_handler
        }
    },
/* set customized_key_hander = 1 in ql_keyctrl_cfg
    {
        "key_name",
        {
            key_press_down_handler,
            key_release_handler,
            key_long_press_handler
        }
    }
*/
};

int custom_key_handler_num(void)
{
    return ARRAY_SIZE(g_custom_key_handler_table);
}
