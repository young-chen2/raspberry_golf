/*
 CS 107E, Young Chen.
 The framebuffer module initializes the framebuffer display
 setup that controls how pixels are displayed onto the HDMI
 screen. Clients can either choose a single-buffer mode, in
 which drawing is done on one layer, the same as the display
 layer, or double-buffer mode, in which two layers are used
 (one for display and the other for drawing) that alternatively refreshes
 to create a smoother image transition.
 */

#include "fb.h"
#include "assert.h"
#include "mailbox.h"

typedef struct {
    unsigned int width;       // width of the physical screen
    unsigned int height;      // height of the physical screen
    unsigned int virtual_width;  // width of the virtual framebuffer
    unsigned int virtual_height; // height of the virtual framebuffer
    unsigned int pitch;       // number of bytes per row
    unsigned int bit_depth;   // number of bits per pixel
    unsigned int x_offset;    // x of the upper left corner of the virtual fb
    unsigned int y_offset;    // y of the upper left corner of the virtual fb
    void *framebuffer;        // pointer to the start of the framebuffer
    unsigned int total_bytes; // total number of bytes in the framebuffer
} fb_config_t;

static volatile fb_config_t fb __attribute__ ((aligned(16)));

void fb_init(unsigned int width, unsigned int height, unsigned int depth_in_bytes, fb_mode_t mode)
{
    fb.width = width;
    fb.virtual_width = width;
    fb.height = height;
    if(mode == FB_SINGLEBUFFER) {
        fb.virtual_height = height;
    }
    // if we are in double buffer mode, set virtual height to twice
    // the height to simultaneous store the display and draw buffers
    if(mode == FB_DOUBLEBUFFER) {
        fb.virtual_height = 2 * height;
    }
    fb.bit_depth = depth_in_bytes * 8; // convert number of bytes to number of bits
    fb.x_offset = 0;
    fb.y_offset = 0;

    // the manual states that we must set these value to 0
    // the GPU will return new values in its response
    fb.pitch = 0;
    fb.framebuffer = 0;
    fb.total_bytes = 0;

    // Send address of fb struct to the GPU as message
    bool mailbox_success = mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
    assert(mailbox_success); // confirm successful config
}

/*
 Swaps the front framebuffer with the back framebuffer and
 vice versa, depending on which one is currently being displayed.
 Uses y_offset to repoint the framebuffer to either the start of
 the contiguous memory where the first half is, or the second half
 of contiguous memory where the second half is.
 Sends a request to the mailbox to update the information change.
 */
void fb_swap_buffer(void)
{
    if(fb.virtual_height == 2 * fb.height) { //only executes if in doouble buffer mode
        // rotates to the back buffer
        if(fb.y_offset == 0) {
            fb.y_offset = fb.height;
        }
        // rotates to the front buffer
        else {
            fb.y_offset = 0;
        }
        bool mailbox_success = mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
        assert(mailbox_success); // confirm success
    }
}

/*
 Uses the initialized field y_offset to access
 a pointer to memory at the start of the current framebuffer
 the client can draw to.
 */
void* fb_get_draw_buffer(void)
{
    if(fb.virtual_height == 2 * fb.height) { // if in doouble buffer mode
        if(fb.y_offset == 0) { //if the buffer "on-screen" is in the top half
            return (char*)fb.framebuffer + (fb.height * fb.pitch);
        }
        else {
            return fb.framebuffer;
        }
    }
    return fb.framebuffer; //default, single buffer return case
}

/*
 Returns the physical width of the framebuffer.
 */
unsigned int fb_get_width(void)
{
    return fb.width;
}

/*
 Returns the physical height of the framebuffer.
 */
unsigned int fb_get_height(void)
{
    return fb.height;
}

/*
 Returns the depth of the framebuffer (which is always 4 bytes for the pixel
 arithmetic).
 */
unsigned int fb_get_depth(void)
{
    return fb.bit_depth / 8;
}

/*
 Returns the pitch of the framebuffer.
 */
unsigned int fb_get_pitch(void)
{
    return fb.pitch;
}

