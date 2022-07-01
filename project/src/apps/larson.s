/*
 * SMOOTH LARSON SCANNER
 *
 * Intializes GPIOs 20-26 (I only have 7 LEDs) via bit-shift operations. We left-shift r1 each iteration to access the next available register (i.e. GPIO 21 in FSEL 2). After this, we "orr" with #1 to write a "1" to the end of the 32-bit binary to signal configuration for output.
 * Operates via a 2 inner loops (one for moving the pins in a forward direction and one for the reverse direction) embedded within an infinitely looping outer loop.
 * Each inner loop follows the same operational logic; a tracker variable, stored within r3, is initialized before the inner loop header to the start or end GPIO pin respectively (GPIO 20, GPIO 27). All GPIOs are turned on for DELAY times as the first part of the dimming instruction.
 * Next, all GPIO pins which aren't the central LED are turned off for DELAY2 times in order to create the first dimming effect. Note that central pin is on the entire duration. These pins are on approximately 50% of the time.
 * Then, all GPIO pins more than 2 indices/offsets away from the central LED are turned off for DELAY2 times in order to create the second dimming effect. These pins are on approximately 25% of the time.
 * The "forward" inner loop iterates and increments r3 (LSL 1) until the BNE flag is set to 0 via comparing the tracker register to the last GPIO (1<<27).
 * r3 is reset to the GPIO 26 pin before the "reverse" inner loop iterates in the same modular instructinos as the "foward" loop.
 */

.equ DELAY, 0x1A0 //time set to let LED be "on"
.equ DELAY2, 0x1F000 //time set to let LED be "off"


// configure GPIO 20 for output
ldr r0, FSEL2 //loads FSEL2 for pins 20-29
mov r1, #1 //immediately moves 1 to r1
str r1, [r0]

// configure GPIO 21 for output
lsl r1, r1, #3 //shifts r1 3 places left to access pin 21 at 1000
orr r1, #1 //appends 1 to the end of 1000 to create 1001
str r1, [r0] //store r1 into FSEL2

// configure GPIO 22 for output
lsl r1, r1, #3 //shifts r1 3 places left to access pin 21 at 1001000
orr r1, #1 //1001001
str r1, [r0]

// configure GPIO 23 for output
lsl r1, r1, #3 //1001001000
orr r1, #1 //1001001001
str r1, [r0]

// configure GPIO 24 for output
lsl r1, r1, #3 //10010010001000
orr r1, #1 //1001001001001001
str r1, [r0]

// configure GPIO 25 for output
lsl r1, r1, #3 //1001001001001001000
orr r1, #1 //1001001001001001001
str r1, [r0]

// configure GPIO 26 for output
lsl r1, r1, #3 //1001001001001001001000
orr r1, #1 //1001001001001001001001
str r1, [r0]

loop:

// tracker variable for the central LED
mov r3, #(1<<20)

forwardCentral:

// set GPIO 20-26 to high: all lights are turned on
ldr r0, SET0
mov r1, #(1<<20) //accesses bit 20
str r1, [r0] //sets bit 20
mov r1, #(1<<21) //accesses bit 21
str r1, [r0] //sets bit 21
mov r1, #(1<<22) //accesses bit 22
str r1, [r0] //sets bit 22
mov r1, #(1<<23) //accesses bit 23
str r1, [r0] //sets bit 23
mov r1, #(1<<24) //accesses bit 24
str r1, [r0] //sets bit 24
mov r1, #(1<<25) //accesses bit 25
str r1, [r0] //sets bit 25
mov r1, #(1<<26) //accesses bit 26
str r1, [r0] //sets bit 26

mov r2, #DELAY
wait1:
    subs r2, #1
    bne wait1

// clear the all bits 6+ GPIOs from the center
ldr r0, CLR0 //loads clr into r0
lsl r4, r3, #1 //shifts r3 to the left by 1 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #1 //shifts r3 to the right by 1 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #2 //shifts r3 to the left by 2 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #2 //shifts r3 to the right by 2 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #3 //shifts r3 to the left by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #3 //shifts r3 to the right by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #4 //shifts r4 to the left by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #4 //shifts r4 to the right by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #5 //shifts r3 to the left by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #5 //shifts r3 to the right by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #6 //shifts r3 to the left by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #6 //shifts r3 to the right by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit

mov r2, #DELAY2
wait2:
    subs r2, #1
    bne wait2

// set GPIO 20-26 to high: all lights are turned on
ldr r0, SET0
mov r1, #(1<<20) //accesses bit 20
str r1, [r0] //sets bit 20
mov r1, #(1<<21) //accesses bit 21
str r1, [r0] //sets bit 21
mov r1, #(1<<22) //accesses bit 22
str r1, [r0] //sets bit 22
mov r1, #(1<<23) //accesses bit 23
str r1, [r0] //sets bit 23
mov r1, #(1<<24) //accesses bit 24
str r1, [r0] //sets bit 24
mov r1, #(1<<25) //accesses bit 25
str r1, [r0] //sets bit 25
mov r1, #(1<<26) //accesses bit 26
str r1, [r0] //sets bit 26

mov r2, #DELAY
wait3:
    subs r2, #1
    bne wait3
 
// clear the bits furthest to current center
ldr r0, CLR0 //loads clr into r0
lsl r4, r3, #3 //shifts r3 to the left by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #3 //shifts r3 to the right by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #4 //shifts r4 to the left by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #4 //shifts r4 to the right by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #5 //shifts r3 to the left by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #5 //shifts r3 to the right by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #6 //shifts r3 to the left by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #6 //shifts r3 to the right by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
 
mov r2, #DELAY2
wait4:
    subs r2, #1
    bne wait4

lsl r3, r3, #1 // incrementing counter by 1
cmp r3, #(1<<27) //sets flag or halt condition when we reach the last central LED

bne forwardCentral



// tracker variable for the central LED
mov r3, #(1<<26)

reverseCenter:

// set GPIO 20-26 to high: all lights are turned on
ldr r0, SET0
mov r1, #(1<<20) //accesses bit 20
str r1, [r0] //sets bit 20
mov r1, #(1<<21) //accesses bit 21
str r1, [r0] //sets bit 21
mov r1, #(1<<22) //accesses bit 22
str r1, [r0] //sets bit 22
mov r1, #(1<<23) //accesses bit 23
str r1, [r0] //sets bit 23
mov r1, #(1<<24) //accesses bit 24
str r1, [r0] //sets bit 24
mov r1, #(1<<25) //accesses bit 25
str r1, [r0] //sets bit 25
mov r1, #(1<<26) //accesses bit 26
str r1, [r0] //sets bit 26

mov r2, #DELAY
wait5:
    subs r2, #1
    bne wait5


// clear the all bits 6+ GPIOs from the center
ldr r0, CLR0 //loads clr into r0
lsl r4, r3, #1 //shifts r3 to the left by 1 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #1 //shifts r3 to the right by 1 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #2 //shifts r3 to the left by 2 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #2 //shifts r3 to the right by 2 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #3 //shifts r3 to the left by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #3 //shifts r3 to the right by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #4 //shifts r4 to the left by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #4 //shifts r4 to the right by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #5 //shifts r3 to the left by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #5 //shifts r3 to the right by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #6 //shifts r3 to the left by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #6 //shifts r3 to the right by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit

mov r2, #DELAY2
wait6:
    subs r2, #1
    bne wait6

// set GPIO 20-26 to high: all lights are turned on
ldr r0, SET0
mov r1, #(1<<20) //accesses bit 20
str r1, [r0] //sets bit 20
mov r1, #(1<<21) //accesses bit 21
str r1, [r0] //sets bit 21
mov r1, #(1<<22) //accesses bit 22
str r1, [r0] //sets bit 22
mov r1, #(1<<23) //accesses bit 23
str r1, [r0] //sets bit 23
mov r1, #(1<<24) //accesses bit 24
str r1, [r0] //sets bit 24
mov r1, #(1<<25) //accesses bit 25
str r1, [r0] //sets bit 25
mov r1, #(1<<26) //accesses bit 26
str r1, [r0] //sets bit 26

mov r2, #DELAY
wait7:
    subs r2, #1
    bne wait7

// clear the bits furthest to current center
ldr r0, CLR0 //loads clr into r0
lsl r4, r3, #3 //shifts r3 to the left by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #3 //shifts r3 to the right by 3 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #4 //shifts r4 to the left by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #4 //shifts r4 to the right by 4 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #5 //shifts r3 to the left by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #5 //shifts r3 to the right by 5 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsl r4, r3, #6 //shifts r3 to the left by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit
lsr r4, r3, #6 //shifts r3 to the right by 6 and stores result in r4
mov r1, r4 //stores left shifted r3 value in r1
str r1, [r0] //clears bit

mov r2, #DELAY2
wait8:
    subs r2, #1
    bne wait8

lsr r3, r3, #1 // decrementing counter by 1
cmp r3, #(1<<19) //sets flag or halt condition when we reach the first central LED

bne reverseCenter

b loop

FSEL0: .word 0x20200000
FSEL1: .word 0x20200004
FSEL2: .word 0x20200008
SET0:  .word 0x2020001C
SET1:  .word 0x20200020
CLR0:  .word 0x20200028
CLR1:  .word 0x2020002C
