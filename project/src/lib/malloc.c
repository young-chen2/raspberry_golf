/* Young Chen, CS107e
 Malloc dynamically manages memory allocation on the heap using "header" structs,
 which have fields for the size of the payload and for whether the chunk is free
 or occupied. With each request to malloc that is within the serviceable limit,
 malloc uses the implicit linked list of  headers (advancing or decrementing via
 the header's payload size) to either find a free block in the middle of the heap
 to accomodate the new request, or expand the end of the heap to meet the request.
 When freeing any particular block of memory, the malloc module checks the
 subsequent forward block to see if it is free. If so, free updates the header
 of the current block to coalesce the size of the subsequent block. This process
 repeats until the program runs into an occupied subsequent block.
 Heap dump is a utility function to print out the state of the entire heap,
 including the chunks, their sizes, and their addresses.
 */

#include "malloc.h"
#include "printf.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "backtrace.h"

extern int __bss_end__;

typedef struct{
    size_t payload_size;
    int status;       // 0 if free, 1 if in use
}header;

enum{IN_USE = 0, FREE = 1};

/*
 * The pool of memory available for the heap starts at the upper end of the
 * data section and extend up from there to the lower end of the stack.
 * It uses symbol __bss_end__ from memmap to locate data end
 * and calculates end of stack reservation assuming a 16MB stack.
 *
 * Global variables for the bump allocator:
 *
 * `heap_start`  location where heap segment starts
 * `heap_end`    location at end of in-use portion of heap segment
 */

// Initial heap segment starts at bss_end and is empty
static void *heap_start = &__bss_end__;
static void *heap_end = &__bss_end__;

// Call sbrk as needed to extend size of heap segment
// Use sbrk implementation as given
void *sbrk(int nbytes)
{
    void *sp;
    __asm__("mov %0, sp" : "=r"(sp));   // get sp register (current stack top)
    char *stack_reserve = (char *)sp - 0x1000000; // allow for 16MB growth in stack

    void *prev_end = heap_end;
    if ((char *)prev_end + nbytes > stack_reserve) {
        return NULL;
    } else {
        heap_end = (char *)prev_end + nbytes;
        return prev_end;
    }
}


// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two because ~(n - 1)
// only clears out the least significant bit to round
// up via powers of 2 in binary.
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

/* Dynamically allocates memory in the heap (starting at bss_end) in
 chunks of 8 bytes. Rounds up the requested number of bytes to allocate
 to the nearest power of 8, adding 8 to that number to compute the total
 size of the chunk including the header.
 Takes the number of bytes we want to reserve.
 Tracking the current block we are on in the heap and starting from the bottom
 of the heap, increment block-by-block using pointer arithmetic (the next block
 is always located payload_size + 8 bytes away). If we find a free block before
 the end of the heap that is both free and has a payload larger than or equal
 to the requested payload, we overwrite the header to that free block to be IN_USE
 and updates the payload to the new size, nbytes. If there remains space after
 this (if the requested size is smaller than what the block has), split the
 block into two by creating a new header that's free and encapsulates the number
 of bytes left in the old payload.
 If no free blocks are available in the middle of the heap, utilize
 the end of the heap to get the number of bytes desired.
 Returns a pointer to the start of the payload or the address of the requested size.
 */
void *malloc (size_t nbytes)
{
    nbytes = roundup(nbytes, 8);
    size_t total_bytes = nbytes + sizeof(header);

    header* cur_block = (header*) heap_start;

    // handles malloc for 0 bytes; returns NULL if no zero size block is found
    // or returns the address to the first allocated block with a size of zero
    if(nbytes == 0) {
        while (cur_block != heap_end) {
            if(cur_block->payload_size == 0 && cur_block->status == IN_USE) {
                return cur_block;
            }
            cur_block = cur_block + ( (cur_block->payload_size / 8) + 1) ; //goes to the free space after adding in the current block, addds 1 to get past header
        }
        return NULL;
    }

    //attempts to find a free block with the correct memory
    while(cur_block != heap_end) {
        if(cur_block->status == FREE && cur_block->payload_size >= nbytes) {
            size_t prev_block_size = cur_block->payload_size;

            cur_block->payload_size = nbytes; //modifies header to reflect new chunk
            cur_block->status = IN_USE;
            size_t bytes_remaining = prev_block_size - nbytes;

            //"split" step; creates 2 blocks within larger block if there is space
            if(bytes_remaining >= 8) { //if there is space left for a header and payload_size is not 0, add the block header and indicate payload size
                header *new_block = cur_block + ( (nbytes / 8) + 1) ; //goes to the free space after adding in the new block, addds 1 to get past header
                new_block->payload_size = (bytes_remaining - 8);
                new_block->status = FREE;
            }

            return cur_block + 1; //returns the start of payload once we've found a suitable free block to recycle
        }
        //go to the next block's header
        cur_block += ( (cur_block->payload_size / 8) + 1);
    }

    //if there are no free blocks in the middle of the contiguous memory to reallocate
    header *hdr = sbrk(total_bytes); //extends the end of the heap

    if(hdr == NULL) { //if sbrk returned a NULL value due to too large of a heap growth
        return NULL; //error-checking NULL
    }

    hdr->payload_size = nbytes;
    hdr->status = IN_USE;
    return hdr + 1; //return address at the start of the payload
}

/* Sets the status of a given header's address to FREE.
 Checks if the following block after the given block is also
 FREE — if so, update the first block's header to coalesce
 the subsequence block (by enlarging the size). Tracks the
 previous and subsequent blocks with separate pointers, udpating them
 throughout the "while" routine.
 This function has no return.
 */
void free (void *ptr)
{
    if(ptr == NULL) {
        return;
    }
    header* hdr = (header*) ptr;
    hdr = hdr - 1; //decrements to the start of the header in memory by 8 bytes
    hdr->status = FREE;
    header* start_header = hdr;
    //advances to the next block
    header* next_hdr = hdr + (hdr->payload_size) / 8 + 1;
    while(next_hdr->status == FREE) {
        start_header->payload_size += (next_hdr->payload_size + 8); //adds the payload size of the subsequent block plus the header's size
        hdr = next_hdr; //using hdr as a tracker for the current header
        next_hdr = hdr + (hdr->payload_size) / 8 + 1; //subsequent block
    }
}

/* Prints out all blocks on the heap: their addresses, their size in bytes, and
 whether they're free or not. Accesses all blocks via pointer arithmetic.
 */
void heap_dump (const char *label)
{
    printf("\n---------- HEAP DUMP (%s) ----------\n", label);
    printf("Heap segment at %p - %p\n", heap_start, heap_end);

    header *cur_heap = (header*)heap_start; //makes the current heap equivalent to a pointer to a pointer
    while(cur_heap != heap_end) {
        size_t cur_size = cur_heap->payload_size;
        int cur_status = cur_heap->status;
        cur_heap = cur_heap + 1; //advance past the header
        printf("Heap with size %d that's %d (0 if taken, 1 if free) at %p\n", cur_size, cur_status, cur_heap);
        cur_heap = cur_heap + (cur_size / 8); //uses size of cur_heap to go the next block in bytes
    }

    printf("----------  END DUMP (%s) ----------\n", label);
}


// FAILED EXTENSION IMPLEMENTATION :(

//extern int __bss_end__;
//
//typedef struct{
//    size_t payload_size;
//    int status;       // 0 if free, 1 if in use
//    frame_t inner_frames[3];
//}header;
//
//enum{IN_USE = 0, FREE = 1, REDZONE = 4};
//
///*
// * The pool of memory available for the heap starts at the upper end of the
// * data section and extend up from there to the lower end of the stack.
// * It uses symbol __bss_end__ from memmap to locate data end
// * and calculates end of stack reservation assuming a 16MB stack.
// *
// * Global variables for the bump allocator:
// *
// * `heap_start`  location where heap segment starts
// * `heap_end`    location at end of in-use portion of heap segment
// */
//
//// Initial heap segment starts at bss_end and is empty
//static void *heap_start = &__bss_end__;
//static void *heap_end = &__bss_end__;
//
//void report_damaged_redzone (void *ptr);
//void memory_report (void);
//
//// Call sbrk as needed to extend size of heap segment
//// Use sbrk implementation as given
//void *sbrk(int nbytes)
//{
//    void *sp;
//    __asm__("mov %0, sp" : "=r"(sp));   // get sp register (current stack top)
//    char *stack_reserve = (char *)sp - 0x1000000; // allow for 16MB growth in stack
//
//    void *prev_end = heap_end;
//    if ((char *)prev_end + nbytes > stack_reserve) {
//        return NULL;
//    } else {
//        heap_end = (char *)prev_end + nbytes;
//        return prev_end;
//    }
//}
//
//// Simple macro to round up x to multiple of n.
//// The efficient but tricky bitwise approach it uses
//// works only if n is a power of two because ~(n - 1)
//// only clears out the least significant bit to round
//// up via powers of 2 in binary.
//#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
//
///* Helper function to set the redzones of a particular block given
// a pointer ot the start of the block's header.
// Sets repetitive value 0x7e to the redzones using memset.
//*/
//void set_redzones (void* ptr) {
//    //memsets the redzones for the newest block
//    header* hdr = (header*)ptr;
//    char* redzone_front = (char*)ptr;
//    redzone_front = redzone_front + sizeof(header); //first zone past the header
//    memset(redzone_front, 0x7e, REDZONE); //fills redzone with 4 byte int 0x7e7e7e7e
//    redzone_front = redzone_front + REDZONE + hdr->payload_size; //advance payload bytes past the front redzone through the payload's exact size
//    memset(redzone_front, 0x7e, REDZONE); //fills redzone with 4 byte int 0x7e7e7e7e
//}
//
///* Dynamically allocates memory in the heap (starting at bss_end) in
// chunks of 8 bytes. Rounds up the requested number of bytes to allocate
// to the nearest power of 8, adding 8 to that number to compute the total
// size of the chunk including the header.
// Takes the number of bytes we want to reserve.
// Tracking the current block we are on in the heap and starting from the bottom
// of the heap, increment block-by-block using pointer arithmetic (the next block
// is always located payload_size + 8 bytes away). If we find a free block before
// the end of the heap that is both free and has a payload larger than or equal
// to the requested payload, we overwrite the header to that free block to be IN_USE
// and updates the payload to the new size, nbytes. If there remains space after
// this (if the requested size is smaller than what the block has), split the
// block into two by creating a new header that's free and encapsulates the number
// of bytes left in the old payload.
// If no free blocks are available in the middle of the heap, utilize
// the end of the heap to get the number of bytes desired.
// Returns a pointer to the start of the payload or the address of the requested size.
// */
//void *malloc (size_t nbytes)
//{
//    //need to track exact bytes for the purpose of redzone detection
//    //the roundup could ignore much free space between and redzone!
//    size_t exact_bytes = nbytes;
//
//    nbytes = roundup(nbytes, 8);
//    size_t total_bytes = nbytes + sizeof(header) + REDZONE * 2; //adds extra 8 to account for red zone
//
//    header* cur_block = (header*) heap_start;
//
//    // handles malloc for 0 bytes; returns NULL if no zero size block is found
//    // or returns the address to the first allocated block with a size of zero
//    if(nbytes == 0) {
//        while (cur_block != heap_end) {
//            if(cur_block->payload_size == 0 && cur_block->status == IN_USE) {
//                return cur_block;
//            }
//            char* new_block = (char*)cur_block;
//            cur_block = (header*)(new_block + roundup(cur_block->payload_size, 8) + sizeof(header) + REDZONE * 2); //goes to next header's address
//        }
//        return NULL;
//    }
//
//    //attempts to find a free block with the correct memory, splits
//    //the memory into two, and returns the start of the payload for
//    //the first chunk
//    while(cur_block != heap_end) {
//        if(cur_block->status == FREE && roundup(cur_block->payload_size, 8) >= nbytes) {
//            size_t prev_block_size = roundup(cur_block->payload_size, 8);
//
//            cur_block->payload_size = exact_bytes; //modifies header to reflect new chunk, exact_bytes for redzone
//            cur_block->status = IN_USE;
//            size_t bytes_remaining = prev_block_size - nbytes; //must use nbytes; roundup
//
//            set_redzones(cur_block);
//
//            //"split" step; creates 2 blocks within larger block if there is space
//            if( bytes_remaining >= (sizeof(header) + 2 * REDZONE) ) { //if there is space left for a header and 8 byte redzone, add the block header and indicate payload size
//                header* new_hdr = cur_block;
//                char* new_block = (char*)cur_block;
//                new_hdr = (header*)(new_block + total_bytes); //goes to the free space after adding in the new block
//
//                new_hdr->payload_size = (bytes_remaining - sizeof(header) - 2 * REDZONE);
//                new_hdr->status = FREE;
//
//                //uses a temporary array of frames to backtrace, then stores the values
//                //into the newly split header
//                frame_t backtrace_frames_cur[3];
//                backtrace(backtrace_frames_cur, 3);
//                new_hdr->inner_frames[0] = backtrace_frames_cur[0];
//                new_hdr->inner_frames[1] = backtrace_frames_cur[1];
//                new_hdr->inner_frames[2] = backtrace_frames_cur[2];
//
//                set_redzones(new_hdr);
//            }
//
//            char* block_start = (char*) cur_block;
//            return (block_start + sizeof(header) + REDZONE); //returns the start of payload once we've found a suitable free block to recycle
//        }
//        //go to the next block's header if it's unable to be recycled
//        char* new_block = (char*)cur_block;
//        cur_block = (header*)(new_block + total_bytes);
//    }
//
//    //if there are no free blocks in the middle of the contiguous memory to reallocate
//    header *hdr = sbrk(total_bytes); //extends the end of the heap
//
//    if(hdr == NULL) { //if sbrk returned a NULL value due to too large of a heap growth
//        return NULL; //error-checking NULL
//    }
//
//    hdr->payload_size = exact_bytes;
//    hdr->status = IN_USE;
//
//    frame_t backtrace_frames_cur[3];
//    backtrace(backtrace_frames_cur, 3);
//    hdr->inner_frames[0] = backtrace_frames_cur[0];
//    hdr->inner_frames[1] = backtrace_frames_cur[1];
//    hdr->inner_frames[2] = backtrace_frames_cur[2];
//
//    set_redzones(hdr);
//
//    char* payload_start = (char*)hdr;
//    payload_start = (payload_start + sizeof(header) + REDZONE);
//
//    return payload_start; //return address at the start of the payload, after first redzone
//}
//
///* Sets the status of a given header's address to FREE.
// Checks if the following block after the given block is also
// FREE — if so, update the first block's header to coalesce
// the subsequence block (by enlarging the size). Tracks the
// previous and subsequent blocks with separate pointers, udpating them
// throughout the "while" routine.
// Redzone functionality calls error reporting function to print out
// a "mini Valgrind" report if either redzone (offset by 4 bytes) at the
// start or end of the payload were overwritten.
// This function has no return.
// */
//void free (void *ptr)
//{
//    if(ptr == NULL) {
//        return;
//    }
//
//    char *hdr_addr = (char*) ptr;
//    hdr_addr = (hdr_addr - REDZONE - sizeof(header)); // decrements to the start of the header after the redzone
//    //hdr points to the start of the current header
//    header *hdr = (header*)hdr_addr;
//    hdr->status = FREE;
//
//    //moves to the chunk_start redzone
//    char *redzone_front = (char*) hdr;
//    redzone_front = redzone_front + sizeof(header);
//
//    if(*(unsigned int*)redzone_front != 0x7e7e7e7e) {
//        report_damaged_redzone(hdr);
//    }
//
//    //moves to the chunk-end redzone
//    redzone_front = redzone_front + REDZONE + hdr->payload_size;
//
//    if(*(unsigned int*)redzone_front != 0x7e7e7e7e) {
//        report_damaged_redzone(hdr);
//    }
//
//    //advances to the next block
//    char* next_block_addr = (char*)hdr;
//    next_block_addr = next_block_addr + sizeof(header) + REDZONE * 2 + roundup(hdr->payload_size, 8);
//    header* next_hdr = (header*)next_block_addr;
//
//    //coalesce step
//    while(next_hdr->status == FREE) {
//        hdr->payload_size += (roundup(next_hdr->payload_size, 8) + sizeof(header) + REDZONE * 2); //adds the payload size of the subsequent block, plus the header's size and its redzones
//        hdr = next_hdr;
//        next_block_addr = (next_block_addr + sizeof(header) + REDZONE * 2 + roundup(hdr->payload_size, 8)); //accesses subsequent block's address
//        next_hdr = (header*)next_block_addr; //updates the next header block to the correct address
//    }
//}
//
///* Prints out all blocks on the heap: their addresses, their size in bytes, and
// whether they're free or not. Accesses all blocks via pointer arithmetic.
// */
//void heap_dump (const char *label)
//{
//    printf("\n---------- HEAP DUMP (%s) ----------\n", label);
//    printf("Heap segment at %p - %p\n", heap_start, heap_end);
//
//    header *cur_heap = (header*)heap_start; //start at the heap's start
//    while(cur_heap != heap_end) {
//        int cur_status = cur_heap->status;
//        printf("Heap with payload size %d that's %d (0 if taken, 1 if free) at %p\n", cur_heap->payload_size, cur_status, cur_heap);
//        char* cur_heap_addr = (char*)cur_heap;
//        cur_heap = (header*)(cur_heap_addr + roundup(cur_heap->payload_size, 8) + sizeof(header) + REDZONE * 2); //uses size of cur_heap to go the next block in bytes
//    }
//
//    printf("----------  END DUMP (%s) ----------\n", label);
//}
//
///* Reports, at the end of the main() program, the state of the
// heap, with the amount of bytes lost, amount of unfreed/leaked blocks,
// and number of allocations.
// */
//void memory_report (void)
//{
//    printf("\n=============================================\n");
//    printf(  "         Mini-Valgrind Memory Report         \n");
//    printf(  "=============================================\n");
//
//    header* cur_block = (header*) heap_start;
//
//    unsigned int allocs = 0;
//    unsigned int frees = 0;
//    unsigned int bytes_allocated = 0;
//
//    // goes through implicit headers list to count number of allocs and frees
//    while (cur_block != heap_end) {
//        if(cur_block->status == FREE) {
//            frees++;
//        }
//        bytes_allocated += cur_block->payload_size;
//        allocs++;
//        char* new_block = (char*)cur_block;
//        cur_block = (header*)(new_block + roundup(cur_block->payload_size, 8) + sizeof(header) + REDZONE * 2); //goes to the free space after adding in the new block, adds 1 to get past header
//    }
//
//    unsigned int total_loss = 0;
//    unsigned int num_blocks = 0;
//
//    // starts at the start of the heap yet again to traverse through and output which blocks
//    // of memory in particular were leaked
//    cur_block = (header*) heap_start;
//
//    printf("mallocs/frees: %d mallocs, %d frees, %d bytes allocated\n\n", allocs, frees, bytes_allocated);
//
//    while (cur_block != heap_end) {
//        if(cur_block->status == IN_USE) {
//            printf("%d bytes are lost, allocated by\n", cur_block->payload_size);
//            total_loss += roundup(cur_block->payload_size, 8) + sizeof(header);
//            num_blocks++;
//        }
//        char* new_block = (char*)cur_block;
//        cur_block = (header*)(new_block + roundup(cur_block->payload_size, 8) + sizeof(header) + REDZONE * 2); //goes to the free space after adding in the new block, adds 1 to get past header
//    }
//
//    printf("Lost %d total bytes in %d blocks.", total_loss, num_blocks);
//
//}
//
///* Parameter is the address to the heeader of the block.
// Backtraces the current block of memory for the malloc call which
// overwrote the redzone, and the current state of the redzones in front
// and in the back.
// */
//void report_damaged_redzone (void *ptr)
//{
//    printf("\n=============================================\n");
//    printf(  " **********  Mini-Valgrind Alert  ********** \n");
//    printf(  "=============================================\n");
//    header* hdr = (header*) ptr;
//    char* redzone_ptr = (char*)ptr;
//    redzone_ptr = redzone_ptr + sizeof(header); //access past the header
//    unsigned int front_zone = *(unsigned int*)redzone_ptr;
//    redzone_ptr = redzone_ptr + REDZONE + hdr->payload_size; //access past the header
//    unsigned int back_zone = *(unsigned int*)redzone_ptr;
//    printf("Attempt to free address %p that has damaged red zone(s): [%x] [%x]\n", ptr, front_zone, back_zone);
//    printf("Block of size %d bytes, allocated by\n", roundup(hdr->payload_size, 8));
//    print_frames(hdr->inner_frames, 3);
//}
