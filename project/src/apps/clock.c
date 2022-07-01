#include "gpio.h"
#include "timer.h"

// array with all bit patterns for clock digits
unsigned char clock[] = {0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111, 0b1110111, 0b1111111, 0b0111001, 0b0111111, 0b1111001, 0b1110001, 0b1000000}; //GFEDCBA ordering, within bits, contains 0-F, ind 16 contains hyphen

// array with all relevant GPIO addresses
unsigned int all[12] = {GPIO_PIN26, GPIO_PIN19, GPIO_PIN13, GPIO_PIN6,
                        GPIO_PIN5, GPIO_PIN11, GPIO_PIN9, GPIO_PIN10,
                        GPIO_PIN21, GPIO_PIN20, GPIO_PIN16, GPIO_PIN12}; //indices 8, 9, 10, 11 are the 4 digits

void turn_on_segment(int number);

void main(void) {
    
    // While button at GPIO 2 not pressed, display initialized state
    while (gpio_read(2) != 0) {
        for(int i = 8; i <= 11; i++) { //intialize clock to all "-"
            gpio_set_function(all[i], GPIO_FUNC_OUTPUT);
            gpio_write(all[i], 1); //turns on segment ("-")
            turn_on_segment(16);
            timer_delay_us(2500); //waits
            gpio_write(all[i], 0); //turns off segment
        }
    }
    
    timer_delay_ms(300); //allots 300 ms delay so that pressing button to start will not increment clock
    
    // Clock marches on until the red button (pin 2) or blue button (3) is pressed to reset time
    while(1) {
        for(int j = 0; j <= 5999; j++) { //wraps around after max, 99:59 minutes, is reached
            
            for (int k = 0; k <= 100; k++) { //ensures that the given time displays for 1 second
                
                for (int i = 8; i <= 11; i++){ //displays the time elapsed in seconds
                    
                    gpio_write(all[i], 1); //turns on segment
                    
                    // UX system for the user to manually change minutes and seconds!
                    // Start off by pressing only one button; 4 different configurations
                    // depending if  the other button's pressed within 1 second of
                    // pressing the first. Will halt the for-loop displaying time at
                    // the particular instant.
                    while (gpio_read(2) == 0 || gpio_read(3) == 0) { //if either of the two buttons are pressed at the moment, enter UX mode
                        
                        while (gpio_read(2) == 0) {
                            timer_delay(1);
                            if (gpio_read(3) != 0 && j <= 5989) { //if the blue button isn't pressed, add to the seconds
                                j += 10;
                            }
                            else if (gpio_read(3) == 0 && j >= 10) { //if the blue button is pressed, subtract from the seconds
                                j -= 10;
                            }
                        }
                        
                        while (gpio_read(3) == 0) {
                            timer_delay(1);
                            if (gpio_read(2) != 0 && j <= 5939) { //if the red button isn't pressed, add to the minutes
                                j += 60;
                            }
                            else if (gpio_read(2) == 0 && j >= 60) { //if the red button isn't pressed, subtract to the seconds
                                j -= 60;
                            }
                        }
                
                    }
                    
                    if (i == 8) { //controlling the ten's digit of the minute segment
                        turn_on_segment( (j / 600) % 10); //uses modulus by 10 to access 1 digit only
                    }
                    if (i == 9) { //controlling the one's digit of the minute segment
                        turn_on_segment( (j / 60) % 10); //division by 60, 60 sec in 1 min
                    }
                    if (i == 10) { //controlling the ten's digit of the second segment
                        turn_on_segment( (j / 10) % 6);
                    }
                    if (i == 11) { //controlling the one's digit of the second segment
                        turn_on_segment(j % 10);
                    }
                    timer_delay_us(2500); //waits 2500 micro-seconds
                    gpio_write(all[i], 0); //turns off segment
                }
                
            }
            
        }
        
    }
    
}

/* Sets each of the 8 segments on the digital clock to output,
// reads the bit patterns for each character to determine which
// segment to set, shifting left by 1 until all segments are accessed
// in 8 bits
*/
void turn_on_segment(int number) {
    unsigned char bit_seq = clock[number];
    for(int i = 0; i <= 7; i++) {
        unsigned int num = bit_seq & (0b1); //accesses the very first bit with masking
        gpio_set_function(all[i], GPIO_FUNC_OUTPUT);
        gpio_write(all[i], num); //writes accessed bit to gpio target
        bit_seq = bit_seq >> 1; //goes to the next bit
    }
}
