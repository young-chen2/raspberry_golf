/*
 CS 107E, Young Chen.
 Displays a graphical command-line interface through the Pi onto a
 HDMI-connected display. Initializes a command line with user-requested number
 of characters per row and per column.
 When entering values into the CL console, if we write strings that go beyond
 the alloted number of columns or rows, wrap or scroll lines to handle.
 */

#include "console.h"
#include "gl.h"
#include "fb.h"
#include "printf.h"
#include "strings.h"
#include "malloc.h"

const static int LINE_SPACING = 5;
static int line_height;
static int line_width;
static int MAX_PRINT_LENGTH;

typedef struct {
    unsigned int cursor_x; //the column the cursor is at
    unsigned int cursor_y; //the row the cursor is at
} cursor_position;

typedef struct {
    unsigned int rows;
    unsigned int cols;
    unsigned int width;
    unsigned int height;
    color_t foreground;
    color_t background;
} console_t;

cursor_position cursor;
console_t console;
void* buf_mem; // fixed buffer memory post-initialization

static void process_char(char ch);

/*
 Initializes a given console with the parameters for the number of characters per
 row and per column, the color of the console background, and the color of the
 text/foreground written on the console. Sets up global struct fields with the
 init fields.
 An abstraction with cursors enables tracking of which space to write to
 in the buffer.
 */
void console_init(unsigned int nrows, unsigned int ncols, color_t foreground, color_t background)
{
    line_height = gl_get_char_height() + LINE_SPACING;
    line_width = gl_get_char_width();
    
    // initializes gl with columns * char height number of width pixels and
    // nrows * line height of row pixels
    gl_init(ncols * line_width, nrows * line_height, GL_DOUBLEBUFFER);
    gl_clear(background);
    
    //initializing key elements
    cursor.cursor_x = 0;
    cursor.cursor_y = 0;
    
    MAX_PRINT_LENGTH = ncols * nrows;
    
    console.rows = nrows;
    console.cols = ncols;
    console.width = ncols * line_width;
    console.height = nrows * line_height;
    console.foreground = foreground;
    console.background = background;
    
    buf_mem = malloc(nrows * ncols); //allocating appropriate amount of memory for a 2D array of chars
}

/*
 Resets the cursor to (0,0) and clears the console to only display its background
 color. In addition, clears out every character in the 2D array to a space.
 */
void console_clear(void)
{
    cursor.cursor_x = 0;
    cursor.cursor_y = 0;
    gl_clear(console.background);
    char (*string_buffer) [console.cols] = (char (*) [console.cols]) buf_mem;
    
    // clears out every character in the string buffer to a space
    for (int y = 0; y < console.rows; y++) {
        for (int x = 0; x < console.cols; x++) {
            string_buffer[y][x] = ' ';
        }
    }
}

/*
 Formats client's given formatting string and variadic arguments with vsnprintf,
 subsequently processing each character post-printf to our malloced rows x cols 2D array
 to account for special characters and scrolling / wrapping behavior.
 After writing to the string buffer, draws its contents to the graphical console via gl_draw_string
 and refreshes the double buffer to generate a smooth transition to the display buffer.
 */
int console_printf(const char *format, ...)
{
    char vsnprint_buf[MAX_PRINT_LENGTH];
    
    char (*string_buffer) [console.cols] = (char (*) [console.cols]) buf_mem;

    // writes the formatted string after vsnprintf into buffer vsnprint_buf
    va_list arg;
    va_start(arg, format);
    int chars_written = vsnprintf(vsnprint_buf, MAX_PRINT_LENGTH, format, arg);
    va_end(arg);
    
    for(int i = 0; i < strlen(vsnprint_buf); i++) {
        process_char(vsnprint_buf[i]); //processes singular characters into the string buffer
    }
    
    for(int j = 0; j < console.rows; j++) {
        gl_draw_string(0, j * line_height, string_buffer[j], console.foreground);
    }
        
    gl_swap_buffer();
    gl_clear(console.background);
    
	return chars_written;
}

/*
 Copies every lower row 1-below the top row to the top row, displacing the very first (or 0th)
 row of our string buffer. Clears out the newly-freed bottom row of the console with spaces.
 */
static void scroll(void) {
    char (*string_buffer) [console.cols] = (char (*) [console.cols]) buf_mem;
    //copies the 2nd row to the 1st, etc while displacing the
    //very first row out of the buffer
    for(int y = 0; y < console.rows; y++) {
        memcpy(string_buffer[y], string_buffer[y + 1], console.cols);
    }
    // clears out the last row which we've just shifted
    for(int x = 0; x < console.cols; x++) {
        string_buffer[console.rows - 1][x] = ' ';
    }
}

/*
 Uses tracker struct "cursor" to keep track of the insertion point into the string buffer
 to display onto console. If a character is normal, simply add to the buffer and account for
 scrolling/wrapping behavior.
 Else if \b, delete the current character by setting it to a null terminator and
 move the cursor's x position backwards if possible.
 If \n, increment the cursor's y and set x to 0, scrolling when necessary.
 If \f, call console_clear to clean up.
 Uses the same memory as the malloced buf_mem.
 */
static void process_char(char ch)
{
    char (*string_buffer) [console.cols] = (char (*) [console.cols]) buf_mem;

    if(ch != '\b' && ch != '\n' && ch != '\f') {
        if( cursor.cursor_y >= console.rows ){ //scrolling handling for a new, normal line
            scroll();
            cursor.cursor_x = 0;
            cursor.cursor_y--;
            string_buffer[cursor.cursor_y][cursor.cursor_x] = ch;
            cursor.cursor_x++;
        }
        
        else if( (cursor.cursor_y == console.rows - 1) && (cursor.cursor_x == console.cols) ) { //when we are about to overflow, edge case for scrolling
            scroll();
            cursor.cursor_x = 0; //keep y the same (nrows - 1)
            string_buffer[cursor.cursor_y][cursor.cursor_x] = ch;
            cursor.cursor_x++;
        }
        
        //case to handle the horizontal
        else if (cursor.cursor_x < console.cols) {
            string_buffer[cursor.cursor_y][cursor.cursor_x] = ch;
            cursor.cursor_x++;
        }

        //handles line wrapping without scrolling
        else {
            cursor.cursor_x = 0;
            cursor.cursor_y++;
            string_buffer[cursor.cursor_y][cursor.cursor_x] = ch;
            cursor.cursor_x++;
        }
    }
    
    if(ch == '\b') {
        if(cursor.cursor_x > 0) {
            cursor.cursor_x--; //moves the cursor back
            string_buffer[cursor.cursor_y][cursor.cursor_x] = ' '; //clears the previous char
        }
    }
    
    if(ch == '\n') {
        if( cursor.cursor_y < console.rows ) { //normal case for new line
            cursor.cursor_x = 0;
            cursor.cursor_y++;
        }
        else { //scrolling if we hit newspace past the framebuffer
            scroll();
            cursor.cursor_x = 0;
            cursor.cursor_y--;
        }
    }
    
    if(ch == '\f') {
        console_clear();
    }
}
