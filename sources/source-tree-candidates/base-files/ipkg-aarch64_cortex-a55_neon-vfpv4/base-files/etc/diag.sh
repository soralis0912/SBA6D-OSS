#!/bin/sh
# Customize your LED control APIs
. /lib/functions/leds.sh

# Customize your sysfs name of LED under path /sys/class/leds/
status_led="led9517:red:system"

# Customize your LED behavior for each state
set_led_state() {
    case "$1" in
        # Triggered by /init.d/led_shutdown.init script from led-controller package
        shutdown)
            status_led_blink_fast
            ;;
        preinit)
            #status_led_blink_fast
            ;;
        failsafe)
            status_led_blink_slow
            ;;
        preinit_regular)
            #status_led_blink_fast
            ;;
        upgrade)
            status_led_blink_slow
            ;;
        done)
            status_led_on
        ;;
        esac
}

set_state() {
    set_led_state "$1"
}
