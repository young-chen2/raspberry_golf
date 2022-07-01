/* Young Chen, CS 107e.
 Uses two helper functions, unsigned_to_base and signed_to_base,
 to convert a given positive or negative, hexadecimal or decimal number
 into its string representation. Uses the result of these functions
 in main function vnprintf.
 vnprintf and its auxiliary functions printf and snprintf
 use formatted codes in the form %_ ("_" is an insert
 character) to write chars, ints, strings, and pointers into a string
 result to be printed onto console using uart.
 */

#include "printf.h"
#include <stdarg.h>
#include <stdint.h>
#include "strings.h"
#include "uart.h"

int unsigned_to_base(char *buf,
                     size_t bufsize,
                     unsigned int val,
                     int base, size_t
                     min_width);
int signed_to_base(char *buf,
                   size_t bufsize,
                   int val,
                   int base,
                   size_t min_width);

/* From here to end of file is some sample code and suggested approach
 * for those of you doing the disassemble extension. Otherwise, ignore!
 *
 * The struct insn bitfield is declared using exact same layout as bits are organized in
 * the encoded instruction. Accessing struct.field will extract just the bits
 * apportioned to that field. If you look at the assembly the compiler generates
 * to access a bitfield, you will see it simply masks/shifts for you. Neat!
*/

static const char *cond[16] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
                               "hi", "ls", "ge", "lt", "gt", "le", "", ""};

static const char *opcodes[16] = {"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
                                  "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn"};

static const char *registers[16] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                  "r8", "r9", "r10", "fp", "ip", "sp", "lr", "pc"};

struct insn  {
    uint32_t reg_op2:4;
    uint32_t one:1;
    uint32_t shift_op: 2;
    uint32_t shift: 5;
    uint32_t reg_dst:4;
    uint32_t reg_op1:4;
    uint32_t s:1;
    uint32_t opcode:4;
    uint32_t imm:1;
    uint32_t kind:2;
    uint32_t cond:4;
};

//static void sample_use(unsigned int *addr) {
//    struct insn in = *(struct insn *)addr;
//    printf("opcode is %s, s is %d, reg_dst is r%d\n", opcodes[in.opcode], in.s, in.reg_dst);
//}

#define MAX_OUTPUT_LEN 1024

/* Converts an integer, either hex or decimal, into its string
 representation by writing into pointer parameter at "buf."
 Takes in bufsize, the maximum amount of space in buf. Unsigned_to_base
 writes each character to a temporary buffer with max output length,
 truncating upon completion by copying the temporary buffer into
 the intended buffer "buf" with maximum size bufsize, adding in a
 null terminator if temp buf ends up being larger than buf.
 First calculates the number of digits we need to write from val;
 divides by base each time until we reach a result 0 (3/10 = 0).
 Works via writing to the temp buf backwards, starting from the offset
 of the number of digits in the integer to convert. Uses modulus
 to access the current digit (least significant first), adding to char,
 and dividing by base to get the next digit.
 If the number of digits here is less than the minimum width, we offset
 by min width from the temp buffer, and after writing all digits of
 the desired integer, fills the front with digits.
 Returns the number of digits we've written.
 Parameters include the buffer to write the string representation to,
 the size of the buffer we've allocated, the unsigned int to convert,
 the base (10 or 16), and the minimum width of the digit.
 */
int unsigned_to_base(char *buf, size_t bufsize, unsigned int val, int base, size_t min_width)
{
    unsigned int count = 0;
    unsigned int num_digits = 0;
    char new_buf [MAX_OUTPUT_LEN];
    char *temp_buf = new_buf;
    unsigned int temp = val;
    
    //counting the number of digits in the given integer
    while(temp >= 1) {
        num_digits++;
        temp /= base;
    }
    
    //reset temp
    temp = val;
    
    if(num_digits < min_width) {
        temp_buf += min_width;
        *temp_buf = '\0'; //null terminating temp_buf
        temp_buf -= 1;
        count = min_width;
    }
    
    else {
        temp_buf += num_digits;
        count = num_digits;
        *temp_buf = '\0'; //null terminating temp_buf
        temp_buf -= 1;
    }

    while(temp >= 1) {
        if(base == 10) {
            *temp_buf = (char)(temp % base + 48); //writing to the char array in reverse
        }
        else if(base == 16) {
            if(temp % base < 10) {
                *temp_buf = (char)(temp % base + 48);
            }
            else if(temp % base >= 10) {
                *temp_buf = (char)((temp - 10) % base + 97);
            }
        }
        temp /= base;
        temp_buf -= 1;
    }
            
    if(num_digits < min_width) {
        while(temp_buf != new_buf) {
            *temp_buf = '0';
            temp_buf -= 1;
        }
        *temp_buf = '0'; //starts off temp_buf with 0 too
    }
    
    memcpy(buf, new_buf, bufsize);
    
    // if the given buffer does not already have a null terminator, add it
    if(num_digits > bufsize) {
        *(buf + (bufsize - 1)) = '\0';
    }
    
    return count;
}

/* Returns a signed hex or decimal integer in its string representation.
 Calls helper function unsigned_to_base; negates a negative
 integer and does nothing to a positive integer, while casting them
 to unsigned ints. If the integer is negative, write a "-" sign
 in the buffer we want to write to and decrease the buffer size by one
 along with min width since we've already written one char. Checks if
 mid_width > 1 to prevent subtraction of 0 - 1. Else if positive,
 same behavior as unsigned to base.
 Returns the number of digits we've written (plus - if written).
 Parameters include the buffer
 to write the string representation to, the size of the buffer
 we've allocated, the unsigned int to convert, the base (10 or 16),
 and the minimum width of the digit.
 */
int signed_to_base(char *buf, size_t bufsize, int val, int base, size_t min_width)
{
    unsigned int new_val;

    if(val < 0) {
        val = val * -1;
        new_val = (unsigned int)val; //casting val after negation
        *buf = '-';
        buf += 1;
        if(min_width > 1) {
            return unsigned_to_base(buf, bufsize - 1, new_val, base, min_width - 1) + 1;
        }
        else {
            return unsigned_to_base(buf, bufsize - 1, new_val, base, min_width) + 1;
        }
    }
    
    new_val = (unsigned int)val;
    return unsigned_to_base(buf, bufsize, new_val, base, min_width);
}

/* Uses string formatting codes starting with % (read L->R) to insert
 variadic arguments into a given string.
 %%, %c, %s, %0...d/x, %d, %x, %p supported.
 Writes to buf in forward order from the char array provided to be
 formatted. Copies each character from format string to buffer
 if there is a format code. If we run into %, then we read the
 next character(s) to figure out what type of character we are
 reading from the stream of inputs given (i.e. 1, 'u', "love", 0xa).
 Depending on each % flag, execute a different subroutine to write to
 buffer; uses helper functions from string library.
 */
int vsnprintf(char *buf, size_t bufsize, const char *format, va_list args)
{
    unsigned int count = 0;
    char *begin = buf;
            
    while(*format != '\0' && count <= bufsize) {
        if(*format == '%') {
            
            format += 1;
            
            if(*format == '%') {
                *buf = '%';
                count ++;
                buf += 1;
            }
            
            else if(*format == 'c') {
                *buf = (char)va_arg(args, int);
                count ++;
                buf += 1;
            }
            
            else if(*format == 's') {
                char* arg = va_arg(args, char*);
                if(strlen(arg) < bufsize - count) {
                    memcpy(buf, arg, strlen(arg)); //add str to the end of buf with according size
                }
                else {
                    memcpy(buf, arg, bufsize - count); //add str to the end of buf with truncation
                }
                buf += strlen(arg); //moves buf's current pointer
                count += strlen(arg);
            }
            
            else if(*format == '0') { //specifying the width
                const char **form = &format;
                int width = strtonum(format, form); //gets the min width, points format ahead to where d or x is.
                
                if(*format == 'd') {
                    int arg = va_arg(args, int);
                    int len = 0;
                    if(width < bufsize - count) { //only execute if the min width requested can be accomodated
                        len = signed_to_base(buf, bufsize - count, arg, 10, width); //offset by bufsize - the count of how many chars we've written thus far.
                    }
                    else {
                        len = signed_to_base(buf, bufsize - count, arg, 10, bufsize - count); //if we can't write min-width characters, we must truncate
                    }
                    buf += len;
                    count += len;
                }
                
                else if(*format == 'x') {
                    int arg = va_arg(args, unsigned int);
                    /*
                     RESOLVING BUG: CS107E_AUTO: Write format string '%06x' with dec input of 255 to buffer of size 6 and initial contents 'aaaaaaaaaaaaaaaaaaaaaaa'
                     CS107E_AUTO: Destination buffer has contents '0000ff' and was expected to have contents '0000f'
                     CS107E_AUTO: Checking memory addresses of buffer and 18 memory locations after end of buffer
                     CS107E_AUTO: Detected error in destination buffer at index 5. Expected character '' but found 'f'
                     CS107E_AUTO: Detected error in destination buffer at index 6. Expected character 'a' but found ''
                     
                     ISSUE: NOT TRUNCATING MIN-WIDTH REQUESTS LIKE WE'VE TRUNCATED STRINGS
                     */
                    int len = 0;
                    if(width < bufsize - count) { //only execute if the min width requested can be accomodated
                        len = unsigned_to_base(buf, bufsize - count, arg, 16, width);
                    }
                    else {
                        len = unsigned_to_base(buf, bufsize - count, arg, 16, bufsize - count);
                    }
                    buf += len;
                    count += len;
                }
            }
            
            else if(*format == 'd') {
                int arg = va_arg(args, int);
                int len = signed_to_base(buf, bufsize - count, arg, 10, 0);
                buf += len;
                count += len;
            }
            
            else if(*format == 'x') {
                int arg = va_arg(args, unsigned int);
                int len = unsigned_to_base(buf, bufsize - count, arg, 16, 0);
                buf += len;
                count += len;
            }
            
            else if(*format == 'p') {
                void* arg = va_arg(args, void*);
                
                char next_char = *(format + 1);
                
                if(next_char == 'I') {
                    
                    struct insn in = *(struct insn *)arg;
                    unsigned int in_as_hex = (unsigned int) *((unsigned int *)arg);

                    char result [MAX_OUTPUT_LEN];
                    memset(result, '\0', MAX_OUTPUT_LEN);
                    const char *opcode = opcodes[in.opcode];
                    const char *destination = registers[in.reg_dst];
                    const char *op1 = registers[in.reg_op1];
                    const char *op2 = registers[in.reg_op2];
                    const char *condition = cond[in.cond];
                    
                    if(in.kind == 0b00) { //data processing
                        //3-length instructions
                        if( (in.opcode == 0b1000 || in.opcode == 0b1001 || in.opcode == 0b1010 || in.opcode == 0b1011) && in.s == 1 ) {
                            memcpy(result, opcode, strlen(opcode)); //adds opcode to the buffer
                            strlcat(result, condition, MAX_OUTPUT_LEN - strlen(result)); //adds the condition code to the buffer
                            strlcat(result, " ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            strlcat(result, op1, MAX_OUTPUT_LEN - strlen(result)); //adds op1 to the buffer
                            strlcat(result, ", ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            strlcat(result, op2, MAX_OUTPUT_LEN - strlen(result)); //adds op2 to the buffer
                        }

                        // handling data shifting instructions with cond 1101
                        else if( in.opcode == 0b1101 ) {
                            // mov sp, #134217728
                            if(in.imm == 1 || in.shift_op == 0) {
                                memcpy(result, opcode, strlen(opcode)); //adds opcode (MOV) to the buffer
                                strlcat(result, condition, MAX_OUTPUT_LEN - strlen(result)); //adds the condition code to the buffer
                                strlcat(result, " ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                                strlcat(result, op1, MAX_OUTPUT_LEN - strlen(result)); //adds op1 to the buffer
                                strlcat(result, ", ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                                
                                strlcat(result, "#", MAX_OUTPUT_LEN - strlen(result)); //adds # for immediate value

                                unsigned int src2 = in_as_hex & 0b11111111;
                                
//                                unsigned int src2 = in_as_hex & 0b111111111111; //shifts over to the 20th bit to obtain the rot (4 bits) and imm8 (8 bits) chunks
//                                src2 = src2 << 4; //moves past rot to access the imm8 values
//                                unsigned int lsb = src2 % ( 2 * (in.shift >> 1) ); //saves the least significant bits
//                                src2 = src2 >> (2 * (in.shift >> 1));
//                                unsigned int shifted_num = 0;
//
//                                shifted_num = shifted_num | src2;
//                                shifted_num = shifted_num | ( lsb << (32 - 2 * (in.shift >> 1)) );
//
//                                char immediate_code [20];
//                                memset(immediate_code, '\0', 20);
//                                unsigned_to_base(immediate_code, 20, shifted_num, 10, 0);
//                                strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                                
                                char immediate_code [20];
                                memset(immediate_code, '\0', 20);
                                unsigned_to_base(immediate_code, 20, src2, 10, 0);
                                strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                            }
                        }
                        //all other cases of data processing: and, add, sub, sbc, bic, mvn, etc.
                        else {
                            memcpy(result, opcode, strlen(opcode)); //adds opcode to the buffer
                            strlcat(result, condition, MAX_OUTPUT_LEN - strlen(result)); //adds the condition code to the buffer
                            strlcat(result, " ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            strlcat(result, destination, MAX_OUTPUT_LEN - strlen(result)); //adds dst to the buffer
                            strlcat(result, ", ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            strlcat(result, op1, MAX_OUTPUT_LEN - strlen(result)); //adds op1 to the buffer
                            strlcat(result, ", ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            if(in.imm == 0) {
                                strlcat(result, op2, MAX_OUTPUT_LEN - strlen(result)); //adds op2 to the buffer
                            }
                            else {
                                strlcat(result, "#", MAX_OUTPUT_LEN - strlen(result)); //adds # for immediate value

                                unsigned int src2 = in_as_hex & 0b11111111;
                                
//                                unsigned int src2 = in_as_hex & 0b111111111111; //shifts over to the 20th bit to obtain the rot (4 bits) and imm8 (8 bits) chunks
//                                src2 = src2 << 4; //moves past rot to access the imm8 values
//                                unsigned int lsb = src2 % ( 2 * (in.shift >> 1) ); //saves the least significant bits
//                                src2 = src2 >> (2 * (in.shift >> 1));
//                                unsigned int shifted_num = 0;
//
//                                shifted_num = shifted_num | src2;
//                                shifted_num = shifted_num | ( lsb << (32 - 2 * (in.shift >> 1)) );
//
//                                char immediate_code [20];
//                                memset(immediate_code, '\0', 20);
//                                unsigned_to_base(immediate_code, 20, shifted_num, 10, 0);
//                                strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                                
                                char immediate_code [20];
                                memset(immediate_code, '\0', 20);
                                unsigned_to_base(immediate_code, 20, src2, 10, 0);
                                strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                            }
                        }
                    }
                    
                    else if(in.kind == 0b10) { //branch and link
                        int s = (in.opcode & 0b1000);
                        
                        if(s == 0) { //checkingthe L value
                            memcpy(result, "b", 1); //adds opcode (LSL) to the buffer
                            strlcat(result, condition, MAX_OUTPUT_LEN - strlen(result)); //adds the condition code to the buffer
                            strlcat(result, " ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            
                            strlcat(result, "#", MAX_OUTPUT_LEN - strlen(result)); //adds # for immediate value
                           
                            int imm24 = (int)in_as_hex << 2; //shifts over to the 24th bit & imm shift
                            int branch = (8 + (int)in_as_hex) + imm24;
                            char immediate_code [20];
                            memset(immediate_code, '\0', 20);
                            unsigned_to_base(immediate_code, 20, branch, 16, 0);
                            
                            strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                        }
                        
                        else if(s == 1) {
                            memcpy(result, "bl", 2); //adds opcode (LSL) to the buffer
                            strlcat(result, condition, MAX_OUTPUT_LEN - strlen(result)); //adds the condition code to the buffer
                            strlcat(result, " ", MAX_OUTPUT_LEN - strlen(result)); //commas/spaces
                            
                            strlcat(result, "#", MAX_OUTPUT_LEN - strlen(result)); //adds # for immediate value

                            int imm24 = (int)in_as_hex << 2; //shifts over to the 24th bit & imm shift
                            int branch = (8 + (int)in_as_hex) + imm24;
                            char immediate_code [20];
                            memset(immediate_code, '\0', 20);
                            unsigned_to_base(immediate_code, 20, branch, 16, 0);

                            strlcat(result, immediate_code, MAX_OUTPUT_LEN - strlen(result));
                        }
                    }
                    
                    //if the given address was an ARM instruction
                    if(strlen(result) > 0) {
                        //adds the resulting ARM assembly instruction to the back of the buffer
                        memcpy(buf, result, strlen(result)); //add str to the end of buf with according size
                        buf += strlen(result);
                        count += strlen(result);
                    }
                    
                    //if the given address was not an ARM instruction, same subroutine as a regular address
                    else {
                        *buf = '0'; //adding in '0x' at the start of code
                        buf += 1;
                        *buf = 'x';
                        buf += 1; //increment buf/cur pointer accordingly
                        int len = unsigned_to_base(buf, bufsize - count, (unsigned int)arg, 16, 0);
                        buf += len;
                        count += len + 2;
                    }
                    
                    format += 1; // moves past the "p" in "pI", outer-most loop moves past "I"
                }
                
                else {
                    *buf = '0'; //adding in '0x' at the start of code
                    buf += 1;
                    *buf = 'x';
                    buf += 1; //increment buf/cur pointer accordingly
                    int len = unsigned_to_base(buf, bufsize - count, (unsigned int)arg, 16, 0);
                    buf += len;
                    count += len + 2;
                }
            }
        }
        
        else { //if there is no formatting code, simply write singular char to buf
            *buf = *format;
            count ++;
            buf += 1;
        }
        
        format += 1; //moves down the string we were given to format
    }
        
    //null terminates buf
    if (*(begin + count) != '\0' && count <= bufsize) {
        *(begin + count) = '\0';
    }
    else {
        *(begin + bufsize - 1) = '\0'; //coud be bufsize not bufsize - 1
    }
            
    return count;
}

/* Uses vsnprintf to process variadic arguments. Turns a non-predetermined
 number of arguments and stores them to va_list arg. Calls
 vsn print with parameters buf (where we write the string to),
 bufsize of the string, and the formatting string we wish to
 format.
 Returns the number of characters we wrote to buf.
 */
int snprintf(char *buf, size_t bufsize, const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    
    int a = vsnprintf(buf, bufsize, format, arg);
    va_end(arg);
    
    return a;
}

/* Uses vsnprintf to process variadic arguments. Turns a non-predetermined
 number of arguments and stores them to va_list arg. Calls
 vsn print with parameters buf (where we write the string to),
 bufsize of the string, and the formatting string we wish to
 format.
 Places the string we wrote in buf to uart in order to display
 onto the console.
 Returns the number of characters we wrote to buf.
 */
int printf(const char *format, ...)
{
    char buf[MAX_OUTPUT_LEN]; //max size array for printf
    memset(buf, '\0', sizeof(buf)); // init contents with known value
    va_list arg;
    va_start(arg, format);
    
    int a = vsnprintf(buf, MAX_OUTPUT_LEN, format, arg);
    
    va_end(arg);
    uart_putstring(buf);
    
    return a;
}


