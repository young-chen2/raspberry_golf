#include "uart.h"
#include "keyboard.h"
#include "printf.h"
#include "shell.h"
#include "timer.h"
#include "interrupts.h"

void main(void)
{
    gpio_init();
    timer_init();
    uart_init();
    
    interrupts_init();
    keyboard_init(KEYBOARD_CLOCK, KEYBOARD_DATA);
    interrupts_global_enable();

    shell_init(keyboard_read_next, printf);

    shell_run();
}
