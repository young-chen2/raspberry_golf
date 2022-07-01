/*
 CS 107E, Young Chen.
 The gl.c module uses the framebuffer abstraction to initialize and draw pixels
 on the graphical display. Includes characters and fonts as well
 as rectangles and lines. Clients provide the desired pixel locations to draw to.
 Drawings will overlap each other if a later drawing overlaps an earlier one.
 */

#include "gl.h"
#include "font.h"
#include "strings.h"

unsigned int gl_get_char_height(void);
unsigned int gl_get_char_width(void);
unsigned int per_row;
static const int DEPTH = 4;

/*
Initializes the graphical display with the parameters passed
 through the function call (width, height, and mode).
 */
void gl_init(unsigned int width, unsigned int height, gl_mode_t mode)
{
    fb_init(width, height, 4, mode);    // use 32-bit depth always for graphics library
    per_row = fb_get_pitch() / DEPTH; // length of each row in pixels (include pitch's padding);
}

/*
Swaps the display buffer with the background buffer we just
 drew into, displaying the background and placing the previous
 buffer to be the one to be drawn to.
 */
void gl_swap_buffer(void)
{
    fb_swap_buffer();
}

/*
Returns the physical width of the framebuffer.
 */
unsigned int gl_get_width(void)
{
    return fb_get_width();
}

/*
Returns the physical height of the framebuffer.
 */
unsigned int gl_get_height(void)
{
    return fb_get_height();
}

/*
Returns a color_t data type that encodes the R, G, B, and A
 values of a particular pixel.
 */
color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    return (0xff << 24) | (r << 16) | (g << 8) | b;
}

/*
 Clears the screen with one solid, opaque color the client passes in.
 */
void gl_clear(color_t c)
{
    color_t (*im)[per_row] = fb_get_draw_buffer();
    for(int y = 0; y < fb_get_height(); y++) {
        for(int x = 0; x < per_row; x++) {
            im[y][x] = c;
        }
    }
}

/*
 Code from Lab 6, CS 107e.
 Draws a singular pixel of the specified color at the
 location with the coordinate (x, y) as provided by the client.
 */
void gl_draw_pixel(int x, int y, color_t c)
{
    if(x < 0 || x >= fb_get_width() || y < 0 || y >= fb_get_height()) {
        return;
    }
    color_t (*im)[per_row] = fb_get_draw_buffer();
    im[y][x] = c;
}

/*
 Returns the color of a particular pixel at location (x, y)
 provded by the client.
 */
color_t gl_read_pixel(int x, int y)
{
    if(x < 0 || x >= per_row || y < 0 || y >= fb_get_height()) {
        return 0;
    }
    color_t (*im)[per_row] = fb_get_draw_buffer();
    return im[y][x];
}

/*
 Code from Lab 6, CS 107e.
 Parameters are the x and y coordinates of the upper left corner,
 with w and h being the width and height of the rectangle, respectively.
 */
void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for(int y_len = y; y_len < y + h; y_len++) {
        for(int x_len = x; x_len < x + w; x_len++) {
            gl_draw_pixel(x_len, y_len, c); //fills up the 2D space with color
        }
    }
}

/*
Code partially from Lab 6, CS 107e.
Draws a character with the glyph font style given the
upper-left corner's (x, y) coordinate and the color c.
 */
void gl_draw_char(int x, int y, char ch, color_t c)
{
    unsigned char buf[font_get_glyph_size()];
    bool got_glyph = font_get_glyph(ch, buf, sizeof(buf));

    unsigned char (*img)[gl_get_char_width()] = (unsigned char (*)[gl_get_char_width()]) buf;

    if(got_glyph) { //if the glyph exists and was obtained correctly
        for (int y_len = 0; y_len < gl_get_char_height(); y_len++) {
            for (int x_len = 0; x_len < gl_get_char_width(); x_len++) {
                if(img[y_len][x_len] == 0xff) {
                    gl_draw_pixel(x + x_len, y + y_len, c); //draws pixel in offsets from given x, y
                }
            }
        }
    }
}

/*
Draws a string horizontally starting from the upper-left coordinate of the
 first character (x, y) with the color c.
 Truncates individual characters on graphical display if the string is too long to fit.
 */
void gl_draw_string(int x, int y, const char* str, color_t c)
{
    for (int i = 0; i < strlen(str); i++) {
        gl_draw_char(x, y, str[i], c);
        x += gl_get_char_width();
    }
}

/*
Returns the height of a character in the glyph font size in pixels.
 */
unsigned int gl_get_char_height(void)
{
    return font_get_glyph_height();
}

/*
 Returns the width of a character in the glyph font size in pixels.
 */
unsigned int gl_get_char_width(void)
{
    return font_get_glyph_width();
}

/*
 Helper function which performs anti-aliasing; returns the color
 value after "dimming" the current color base on the pixel's error/distance
 from the "actual" y value determined by the slope.
 Parameters include the actual y value and hex color.
 */
color_t get_opacity(color_t c, float num) {
    float decimal_distance = num - (int)num;
    
    char *result = (char*)c; //accessing each BGRA field as byte values

    if(decimal_distance <= 0.1) {
        result[0] = (int)(result[0] * 0.9);
        result[1] = (int)(result[1] * 0.9);
        result[2] = (int)(result[2] * 0.9);
    }
    else if(decimal_distance <= 0.2) {
        result[0] = (int)(result[0] * 0.8);
        result[1] = (int)(result[1] * 0.8);
        result[2] = (int)(result[2] * 0.8);
    }
    else if(decimal_distance <= 0.3) {
        result[0] = (int)(result[0] * 0.7);
        result[1] = (int)(result[1] * 0.7);
        result[2] = (int)(result[2] * 0.7);
    }
    else if(decimal_distance <= 0.4) {
        result[0] = (int)(result[0] * 0.6);
        result[1] = (int)(result[1] * 0.6);
        result[2] = (int)(result[2] * 0.6);
    }
    else if(decimal_distance <= 0.5) {
        result[0] = (int)(result[0] * 0.5);
        result[1] = (int)(result[1] * 0.5);
        result[2] = (int)(result[2] * 0.5);
    }
    else if(decimal_distance <= 0.6) {
        result[0] = (int)(result[0] * 0.4);
        result[1] = (int)(result[1] * 0.4);
        result[2] = (int)(result[2] * 0.4);
    }
    else if(decimal_distance <= 0.7) {
        result[0] = (int)(result[0] * 0.3);
        result[1] = (int)(result[1] * 0.3);
        result[2] = (int)(result[2] * 0.3);
    }
    else if(decimal_distance <= 0.8) {
        result[0] = (int)(result[0] * 0.2);
        result[1] = (int)(result[1] * 0.2);
        result[2] = (int)(result[2] * 0.2);
    }
    else if(decimal_distance <= 0.9) {
        result[0] = (int)(result[0] * 0.1);
        result[1] = (int)(result[1] * 0.1);
        result[2] = (int)(result[2] * 0.1);
    }
    else {
        return GL_WHITE;
    }
    
    return (color_t)result;
}

/*
 Draws an anti-aliased line from (x1, y1) to (x2, y2) or vice versa.
 Draws the line using the Wu Xiaolin algorithm, first calculating
 the exact slope between the two points, then drawing pixels in pairs, with
 the pixel's opacity/anti-alias color determined by its distance from the
 actual y-value (calculated from the slope) given the x value.
 */
void gl_draw_line(int x1, int y1, int x2, int y2, color_t c) {
    float slope; //uses a float to keep track of a pixel's distance from slope
    if(x1 - x2  == 0) {
        slope = 0;
    }
    else {
        slope = (float)(y1 - y2) / (float)(x1 - x2);
    }
    float cur_y;
    if(x1 < x2) {
        cur_y = y1;
        for(int cur_x = x1; cur_x < x2; cur_x++) {
            gl_draw_pixel(cur_x, (int)cur_y, get_opacity(c, cur_y));
            gl_draw_pixel(cur_x, (int)cur_y - 1, get_opacity(c, cur_y));
            cur_y += slope;
        }
    }
    else {
        cur_y = y2;
        for(int cur_x = x2; cur_x < x1; cur_x++) {
            gl_draw_pixel(cur_x, (int)cur_y, get_opacity(c, cur_y));
            gl_draw_pixel(cur_x, (int)cur_y - 1, get_opacity(c, cur_y));
            cur_y += slope;
        }
    }
}

/*
 Draws the 3 lines which create a given triangle. Unfilled.
 */
void gl_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t c) {
    gl_draw_line(x1, y1, x2, y2, c);
    gl_draw_line(x1, y1, x3, y3, c);
    gl_draw_line(x2, y2, x3, y3, c);
}
