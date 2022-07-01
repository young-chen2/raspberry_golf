/* Young Chen, CS 107e
 Uses main driver function, backtrace, to trace from the innermost
 to outermost frames in a function call given the current frame pointer.
 Uses helper function name_of to retrieve, by virtue of offset
 memory access from compile-time memory layout, the name of the
 executing function in a particular frame.
 Prints out, as a result of backtrace, the particular address of
 the caller, followed by the name of the caller, and finally
 the offset from the first instruction of the caller to the current
 fp of the caller. 
 */

#include "backtrace.h"
#include "printf.h"

/* Returns the name of a particular frame given the address
 where the frame starts. Uses the layout of registers, in particular,
 that the first 4-byte address below the start address contains in
 its first byte whether or not a name for the function has been
 set at compile-time and in its last three bytes the length of the
 function name.
 name_of uses bit masking to access both pieces of information, using
 the length of the word encoded to back up to the memory address
 the word starts at and returns the word as a const char array.
 If the word is not encoded, return ???.
 */
const char *name_of(uintptr_t fn_start_addr)
{
    uintptr_t *preceding_instrc = (uintptr_t *)(fn_start_addr - 4);
    if( (*preceding_instrc & ~0x00ffffff) == 0xff000000 ) { //if the most significant byte is 0xff
        int word_length = *preceding_instrc & ~0xff000000; //accesses the lower 3 bytes to get word's length
        void *word_start = (char*)preceding_instrc - word_length; //backs up to pointer at the start of the word
        return (char*)word_start;
    }
    return "???";
}

/* Given an array of frame structs and the max amount of frames
 the client desires to write, fill the array with information
 regarding each frame in the call stack's caller resume address,
 where the caller called the callee, the caller's function name,
 and the offset between the resume address and the function prolog
 of the caller, a.k.a the first instruction of the caller.
 Resume address accessed via callee's saved lr, offset calculated
 via subtracting prolog (caller's PC - 12 bytes), name_of retrieved
 via helper.
 Returns the number of frames written to frame_t f[], the count of
 which is kept throughout the function's loop.
 */
int backtrace (frame_t f[], int max_frames)
{
    //accesses current frame pointer
    uintptr_t *cur_fp;
    __asm__("mov %0, fp" : "=r" (cur_fp));
    int numFrames = 0;
    
    while( *(cur_fp - 3) != 0x0 && (numFrames < max_frames) ) {
        uintptr_t caller_pc = *(uintptr_t*)*(cur_fp - 3);
        uintptr_t resume_addr = *(cur_fp - 1); //stores value at saved lr (4 bytes from fp or saved pc)
        int resume_offset = resume_addr - (caller_pc - 12); //offset = saved lr - pc - prolog. pc of caller accessed via casting then dereferencing the memory address in saved fp
        const char *name = name_of(caller_pc - 12); //calls nameof on the first instruction of the frame, which is 12 bytes away from the pc
                
        frame_t newFrame = {name, resume_addr, resume_offset};
        
        cur_fp = (uintptr_t*)*(cur_fp - 3); //sets cur_fp to the fp in the caller frame
        f[numFrames] = newFrame;
        numFrames++;
    }

    return numFrames;
}

/* Prints all component information of structs in an array of
 frame structs; each frame's resume address, caller name,
 and resume offset.
 */
void print_frames (frame_t f[], int n)
{
    for (int i = 0; i < n; i++)
        printf("#%d 0x%x at %s+%d\n", i, f[i].resume_addr, f[i].name, f[i].resume_offset);
}

/* Uses print_frames to print all frames backtraced up until
 a max number of frames —- 50.
 */
void print_backtrace (void)
{
    int max = 50;
    frame_t arr[max];

    int n = backtrace(arr, max);
    print_frames(arr+1, n-1);   // print frames starting at this function's caller
}
