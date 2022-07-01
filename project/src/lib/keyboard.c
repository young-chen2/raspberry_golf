/* CS107E, Young Chen
 Processes key signals read from a PS/2 keyboard, repackaging
 key events at each step to account for modifiers (CTRL, ALT,
 CAPS, SHIFT) active and the ASCII values of
 any given keystroke to print out onto the console.
 
 Modifiers are tracked globally and toggled on/off via key press
 or release.
 */

#include "keyboard.h"
#include "ps2.h"
#include "ps2_keys.h"
#include "printf.h"

static ps2_device_t *dev;
static keyboard_modifiers_t active_modifiers;

/* Initializes the keyboard to read from the Pi's GPIO for the clock
 and GPIO data.
 */
void keyboard_init(unsigned int clock_gpio, unsigned int data_gpio)
{
    dev = ps2_new(clock_gpio, data_gpio);
}

/* Returns the character/hex value we've read from the keyboard.
 Calls ps2_read as a wrapper function.
 */
unsigned char keyboard_read_scancode(void)
{
    return ps2_read(dev);
}

/* Contextually processes keyboard read sequence events
 to ascertain whether a given series (1, 2, or 3 long)
 of scancodes encode a press, release, extended press,
 or extended release. Calls keyboard_read_sequence to
 access the current key action being transmitted by the
 keyboard.
 Uses special sequences 0xF0 and 0xE0 to determine release
 and extended keys respectively.
 Takes no parameters (we read using abstracted helpers), returns
 the package of the current action, which includes whether
 the key is pressed or released, and the keycode that we've read.
 */
key_action_t keyboard_read_sequence(void)
{
    int first_code = keyboard_read_scancode();
    //ordinary press
    if(first_code != 0xF0 && first_code != 0xE0) {
        key_action_t action = {KEY_PRESS, first_code};
        return action;
    }
    int second_code = keyboard_read_scancode();
    //ordinary release
    if(first_code == 0xF0) {
        key_action_t action = {KEY_RELEASE, second_code};
        return action;
    }
    //extended press
    if(first_code == 0xE0 && second_code != 0xF0) {
        key_action_t action = {KEY_PRESS, second_code};
        return action;
    }
    int third_code = keyboard_read_scancode();
    //extended release
    key_action_t action = {KEY_RELEASE, third_code};
    return action;
}

/* Uses helper function keyboard_read_sequence to
 process the ASCII/character representation of a given key and its
 active modifiers.
 Use a lookup table of ps2 codes to map keycode correspondence to
 ASCII representation. If the code translates to CTRL, ALT, SHIFT,
 or CAPSLOCK, we modify the global enum variable active_modifiers
 to track which modifiers should be applied. Uses control flow and
 if statements to process each key event and change modifiers as needed.
 Returns the pacakged struct of key events, including active
 modifiers, the current action, and current key.
 */
key_event_t keyboard_read_event(void)
{
    key_action_t cur_action = keyboard_read_sequence();
    ps2_key_t cur_keycode = ps2_keys[cur_action.keycode];
    unsigned int is_modifier = 1;
    
    while(is_modifier) {
        if(cur_keycode.ch == PS2_KEY_SHIFT) {
            if(cur_action.what == KEY_PRESS) {
                active_modifiers = active_modifiers | KEYBOARD_MOD_SHIFT;
            }
            else {
                active_modifiers = active_modifiers & ~KEYBOARD_MOD_SHIFT;
            }
            cur_action = keyboard_read_sequence();
            cur_keycode = ps2_keys[cur_action.keycode];
        }
        
        else if(cur_keycode.ch == PS2_KEY_ALT) {
            if(cur_action.what == KEY_PRESS) {
                active_modifiers = active_modifiers | KEYBOARD_MOD_ALT;
            }
            else {
                active_modifiers = active_modifiers & ~KEYBOARD_MOD_ALT;
            }
            cur_action = keyboard_read_sequence();
            cur_keycode = ps2_keys[cur_action.keycode];
        }
        
        else if(cur_keycode.ch == PS2_KEY_CTRL) {
            if(cur_action.what == KEY_PRESS) {
                active_modifiers = active_modifiers | KEYBOARD_MOD_CTRL;
            }
            else {
                active_modifiers = active_modifiers & ~KEYBOARD_MOD_CTRL;
            }
            cur_action = keyboard_read_sequence();
            cur_keycode = ps2_keys[cur_action.keycode];
        }
        
        else if(cur_keycode.ch == PS2_KEY_CAPS_LOCK) {
            if(cur_action.what == KEY_PRESS) {
                int on_or_off = (active_modifiers >> 3) & 0b1; //1 if caps lock is on, 0 if off
                if(on_or_off == 0) {
                    active_modifiers = active_modifiers | KEYBOARD_MOD_CAPS_LOCK; //turn on if it was off
                }
                if(on_or_off == 1) {
                    active_modifiers = active_modifiers & ~KEYBOARD_MOD_CAPS_LOCK; //turn off if it was on
                }
            }
            cur_action = keyboard_read_sequence();
            cur_keycode = ps2_keys[cur_action.keycode];
        }
        
        else { //if the current key is not a modifier, stop spinning
            is_modifier = 0;
        }
    }
    
    key_event_t event = {cur_action, cur_keycode, active_modifiers};
    
    return event;
}

/* Calls helper function read_event to gather the current
 key press and return it as its equivalence to an ASCII
 character (if the key returns an ASCII character).
 While we are reading key releases or any of the modifier keys,
 we read the next key event until we reach a valid keystroke to
 return an ASCII value for (using ps2's lookup table).
 Supercedes SHIFT over CAPS.
 */
unsigned char keyboard_read_next(void)
{
    key_event_t cur_event = keyboard_read_event();
    keyboard_modifiers_t modifiers = cur_event.modifiers;
    ps2_key_t cur_action = cur_event.key;
    
    //if we hit a special character, return nothing and move on
    //if we hit a key release, return nothing and move on
    while(cur_event.action.what == KEY_RELEASE || cur_action.ch == PS2_KEY_SHIFT || cur_action.ch == PS2_KEY_ALT || cur_action.ch == PS2_KEY_CTRL || cur_action.ch == PS2_KEY_CAPS_LOCK) {
        cur_event = keyboard_read_event();
        modifiers = cur_event.modifiers;
        cur_action = cur_event.key;
    }
    
    //checking if shift modifier is on or off; "overrides" caps
    if ( (modifiers & 0b1) == 1 ) {
        return cur_event.key.other_ch;
    }
    //checking if caps modifier is on or off
    if ( ((modifiers >> 3) & 0b1) == 1 ) {
        // caps lock and shift is on; return the ch because they cancel
        if(cur_event.key.other_ch >= 65 && cur_event.key.other_ch <= 90) { //checking if char is alphabetical; if not, return the normal ch
            return cur_event.key.other_ch;
        }
    }
    
    return cur_event.key.ch;
}
