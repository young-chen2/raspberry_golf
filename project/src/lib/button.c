#include "gpio.h"
#include "gpio_extra.h"
#include "timer.h"

static const int BUTTON = GPIO_PIN20;

int wait_for_release(void) {
    int time_elapsed = 0;
    while(gpio_read(BUTTON) == 0) {
        timer_delay(1);
        time_elapsed++;
    }
    return time_elapsed;
}
