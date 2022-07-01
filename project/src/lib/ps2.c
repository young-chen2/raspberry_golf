/* CS107E, Young Chen
 Reads GPIO pins from PS/2 keyboard device to receive and
 process data packets for which keys are pressed/released/held.
 Follows strict transmission protocol for 11 bit data packets with
 start bit, payload, parity, and stop bit. Data is transmitted
 whenever the clock edge "falls," or transitions from high to low.
 If a data package is malformed, we discard it and reread until we
 receive a valid package to return as a character/hex value.
 
 After enabling interrupts, uses a ringbuffer queue to store keyboard
 events everytime they are read from the clock and data lines. Enqueues
 and dequeues from the RBQ when we attempt to read from the keys the ps2
 has gathered.
 */

#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupts.h"
#include "malloc.h"
#include "ps2.h"
#include "printf.h"
#include "uart.h"
#include "ringbuffer.h"

void wait_for_falling_clock_edge(ps2_device_t *dev);
int read_bit(ps2_device_t *dev);

// This struct definition fully defines the type declared in ps2.h
// Since the ps2.h API only uses this struct as a pointer, its
// definition can be implementation-specific and deferred until
// here.
struct ps2_device {
    unsigned int clock; // GPIO pin number of the clock line
    unsigned int data;  // GPIO pin number of the data line
    unsigned char code; // The current scancode accumulated by the interrupt
    unsigned int cur_code_index; // Ranges 0-10 based on which part of the code we're on
    unsigned int num_ones;
    rb_t * rb;
};

/*
 Interrupts-driven key handler; takes in parameters for the pc register and
 auxiliary data of any systems-level event.
 Casts aux_data to a ps2_device we've passed in. Reads data from the ps2_device (dev),
 particularly 1 bit every time the handler is called. After gathering a complete scan code
 from a key press, enqueues the scancode into the RBQ.
 Uses tracker variables kept globally within the ps2_device structure itself to track the current
 scancode gathered, the parity of the code, the index we're at in the 11-bit packet,
 and a RBQ initilialized within the struct.
 Clears the GPIO event with each handle_key call to prepare for the next.
 */
void handle_key(unsigned int pc, void *aux_data) {
    ps2_device_t *dev = (ps2_device_t *)aux_data;
    int cur_code = read_bit(dev);
            
    // if we don't get the correct start code upon falling edge, discard
    // if successful, moves the index along and continue into the rest of the packet (11 bits)
    if(cur_code == 0 && dev->cur_code_index == 0) {
        dev->cur_code_index++;
    }
    
    // while we're reading the data-payload, use bit-OR to set each bit in the scancode
    else if(dev->cur_code_index >= 1 && dev->cur_code_index <= 8) {
        if(cur_code == 1) {
            dev->num_ones++;
        }
        dev->code = dev->code | ( cur_code << (dev->cur_code_index - 1) );
        dev->cur_code_index++;
    }
    
    // checking for the parity bit's consistency
    else if(dev->cur_code_index == 9) {
        // if inconsistent, "discard" by resetting all tracking variables
        if(dev->num_ones % 2 == 0 && cur_code == 0) {
            dev->code = 0;
            dev->cur_code_index = 0;
            dev->num_ones = 0;
        }
        // inconsisitency case 2: turns into even parity overall and resets trackers
        else if(dev->num_ones % 2 == 1 && cur_code == 1) {
            dev->code = 0;
            dev->cur_code_index = 0;
            dev->num_ones = 0;
        }
        // if successful, moves the index along
        else {
            dev->cur_code_index++;
        }
    }
    
    // checking the end bit's consistency
    else if(dev->cur_code_index == 10) {
        if(cur_code == 1) {
            rb_enqueue(dev->rb, dev->code); //if the end bit is correct, enqueue the scancode
        }
        //resets the variables to receive the next packet
        dev->code = 0;
        dev->cur_code_index = 0;
        dev->num_ones = 0;
    }
    
    gpio_clear_event(dev->clock);
}

ps2_device_t *ps2_new(unsigned int clock_gpio, unsigned int data_gpio)
{
    ps2_device_t *dev = malloc(sizeof(*dev));

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);
    
    rb_t *rb = rb_new();
    dev->rb = rb;
    
    dev->code = 0;
    dev->cur_code_index = 0;
    dev->num_ones = 0;
    
    //enabling interrupts on the dataline gpio of the keyboard driver
    gpio_interrupts_init();
    gpio_enable_event_detection(clock_gpio, GPIO_DETECT_FALLING_EDGE);
    gpio_interrupts_register_handler(clock_gpio, handle_key, dev); //handler_key is our interrupt event
    gpio_interrupts_enable();
    
    return dev;
}

/* Waits until the clock edge has fallen; uses an infinite
 while loop to track the clock GPIO on the PI.
 */
void wait_for_falling_clock_edge(ps2_device_t *dev)
{
  while(gpio_read(dev->clock) == 0) {}
  while(gpio_read(dev->clock) == 1) {}
}

/* Returns a bit read from ps2 in a "compressed"
 format as an integer: 4 bytes or 32 bits.
*/
int read_bit(ps2_device_t *dev) {
    return gpio_read(dev->data);
}

/* Reads the current data package from the PS2 device's data
 line in 11-bit chunks. Reads the package until
 a "start" bit is received using continue,
 reads the following 8 bits, and checks if the parity
 and stop bits are correctly formed.
 The function reruns until we receive a well-formed package.
 Returns the 8-bit code received in the package.
 */
unsigned char ps2_read(ps2_device_t *dev)
{
    while (rb_empty(dev->rb)) { /* spin */ };
    int scancode = 0;
    rb_dequeue(dev->rb, &scancode);
    return (unsigned char)scancode;
}
