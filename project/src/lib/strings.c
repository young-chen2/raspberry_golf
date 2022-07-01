/* Young Chen, CS 107e.
Declares a variety of essential string-related functions bare-metal, including:
memcpy to write all bytes at one section of memory to another section.
memset to write an integer value to a sectino of memory.
strlen to obtain the length of a null-terminated char array.
strcmp to compare lexicographically two char arrays.
strlcat to concatenate one chunk of memory to the end of another.
strtonum converts a given string to its numerical representation.

Functions operate via manipulation of memory directly with minimal safeguads.
Users should be cognizant not to overwrite any pre-established, un-declared,
or invalid pieces of memory.
 */

#include "strings.h"

/* Copies a segment of memory, of size n (assumed correct when passed in)
 to destination point in memory. Parameters for pointers to the addresses to
 each segment, returns a pointer to the end of the destination segment we've
 copied to after writing n chars/ints.
 */
void *memcpy(void *dst, const void *src, size_t n)
{
    /* Copy contents from src to dst one byte at a time */
    char *d = dst;
    const char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

/* Sets a segment of memory of size "n" to a particular integer value.
 Takes the pointer ot the start of the target memory segment and the desired
 value to write to. After completion, returns the pointer to the
 destination we've written to after writing n ints.
 */
void *memset(void *dst, int val, size_t n)
{
    char *d = dst; //pointer to pointer dst
    while(n--) {
        *d++ = val; //writes val to offset at dst
    }
    return dst;
}

/* Returns the length of a null terminated string, from the character
 in its first space in memory until it hits a null terminator. Returns the
 number of characters we've counted until the null terminator. Behavior undefined
 if the string is invalid. Takes in parameter to the char array/string we
 want the size of.
 */
size_t strlen(const char *str)
{
    size_t n = 0;
    while (str[n] != '\0') {
        n++;
    }
    return n;
}

/* Lexicographically compares two char arrays; returns a number of the
 ASCII difference between the first parameter's first unequal char with the second
 parameter's first unequal char. If the difference is negative, the first number
 is "less than" the second, and if positive, the first number is "greater than" the
 second. If either string runs out of characters and every character has been equal
 so far, returns 0, which means that the two strings are effective equal.
 */
int strcmp(const char *s1, const char *s2)
{
    int diff = *s1 - *s2;

    //iterates until one or both strings "run out"
    while (*s1 != '\0' && *s2 != '\0') {
        if(diff != 0) {
            return diff;
        }
        diff = *s1++ - *s2++;
    }
    
    //checks last character of the string; skipped over in processing
    if(diff != 0) {
        return diff;
    }
    
    return 0;
}

/* Takes in two memory addresses of valid char arrays or strings (null terminated).
 If the destination memory address is invalid, i.e. the size of dst is less
 than the length of dst, exits the function and returns the size of dst if we
 added in the chars/ints from src.
 Otherwise, uses helper function memcpy to copy the the second memory address of the
 destination, at end of the dst string, or null terminator of destination.
 Returns the new length of the destination.
 */
size_t strlcat(char *dst, const char *src, size_t dstsize)
{
    if (strlen(dst) > dstsize) { //malformed string = len > size
        return dstsize + strlen(src);
    }
    else if (strlen(dst) == dstsize - 1) { //NEW TO FIX BUG
        return strlen(dst) + strlen(src);
    }
    memcpy(dst + strlen(dst), src, strlen(src));
    *(dst + dstsize - 1) = '\0'; //BUG FIX! Concatenated string is 'cs107�' and expected 'cs107'
    return strlen(dst) + 1;
}

/* Writes an int representation of either a decimal or hex "sequence" in string.
 Takes in the address we wish to read from, writing the result int "front to
 back" via adding the current digit stripped off from the string we're reading's
 front from to result, multiplying the digit by the base (10 or 16),
 and repeating the process until we're reached the end of the
 string we're reading from.
 Returns the numerical version of the number from the string.
 */
unsigned int strtonum(const char *str, const char **endptr)
{
    unsigned int conversion = 0;
    unsigned int base = 0;
    
    //handles hexadecimal
    if(*str == '0' && *(str + 1) == 'x') {
        base = 16;
        str += 2; //ignore 0x in the hex string
        while( *str != '\0' && ( (*str >= 48 && *str <= 57) || (*str >= 97 && *str <= 102) ) ) {
            if(*str >= 48 && *str <= 57) { //if the char is 0-9
                conversion += *str % 48;
                conversion *= 16;
            }
            else { //if the char is A-F
                conversion += (*str % 97) + 10;
                conversion *= 16;
            }
            str += 1; //moves the string along
        }
    }
    
    //handles decimal
    else {
        base = 10;
        while(*str != '\0' && (*str >= 48 && *str <= 57)) {
            conversion += *str % 48;
            conversion *= 10;
            str += 1;
        }
    }

    if(endptr != NULL) {
        *endptr = str;
    }
    
    return conversion / base; //multiplied base to the "one"'s digit, so take it off before return
}
