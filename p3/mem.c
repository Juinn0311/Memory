////////////////////////////////////////////////////////////////////////////////
// Main File:        mem.c
// This File:        mem.c
// Other Files:      mem.h
// Semester:         CS 354 Fall 2018
//
// Author:           Kendra Raczek
// Email:            raczek@wisc.edu
// CS Login:         raczek
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//
// Persons:
//
// Online sources:   UC Santa Barbara Computer Science: general description of
//                   allocating and freeing memory
//                   https://www.cs.ucsb.edu/~rich/class/cs170/labs/lab1.malloc/

//////////////////////////// 80 columns wide ///////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

/*
 * This structure serves as the header for each allocated and free block
 * It also serves as the footer for each free block
 * The blocks are ordered in the increasing order of addresses 
 */
typedef struct blk_hdr {                         
        int size_status;
  
    /*
    * Size of the block is always a multiple of 8
    * => last two bits are always zero - can be used to store other information
    *
    * LSB -> Least Significant Bit (Last Bit)
    * SLB -> Second Last Bit 
    * LSB = 0 => free block
    * LSB = 1 => allocated/busy block
    * SLB = 0 => previous block is free
    * SLB = 1 => previous block is allocated/busy
    * 
    * When used as the footer the last two bits should be zero
    */

    /*
    * Examples:
    * 
    * For a busy block with a payload of 20 bytes (i.e. 20 bytes data + an additional 4 bytes for header)
    * Header:
    * If the previous block is allocated, size_status should be set to 27
    * If the previous block is free, size_status should be set to 25
    * 
    * For a free block of size 24 bytes (including 4 bytes for header + 4 bytes for footer)
    * Header:
    * If the previous block is allocated, size_status should be set to 26
    * If the previous block is free, size_status should be set to 24
    * Footer:
    * size_status should be 24
    * 
    */
} blk_hdr;

/* Global variable - This will always point to the first block
 * i.e. the block with the lowest address */
blk_hdr *first_block = NULL;

int total_mem_size = 0;
/*
 * Note: 
 *  The end of the available memory can be determined using end_mark
 *  The size_status of end_mark has a value of 1
 *
 */

/* 
 * Function for allocating 'size' bytes
 * Returns address of allocated block on success 
 * Returns NULL on failure 
 * Here is what this function should accomplish 
 * - Check for sanity of size - Return NULL when appropriate 
 * - Round up size to a multiple of 8 
 * - Traverse the list of blocks and allocate the best free block which can accommodate the requested size 
 * - Also, when allocating a block - split it into two blocks
 * Tips: Be careful with pointer arithmetic 
 */                    
void* Alloc_Mem(int size) {

    // check for sanity of size
    if (total_mem_size < 8 || size <= 0) {
        return NULL;
    }
    // add header space
    int blk_size = size + 4;

    // round up to multiple of 8
    int padding = 0;
    if (blk_size % 8 != 0) {
        padding = 8 - (blk_size % 8);
    }
    blk_size += padding;

    blk_hdr* curr_blk = first_block;
    blk_hdr* best_blk = NULL;
    // the header of the current block we are looking at
    char *curr_hdr = 0;
    // the actual size of the block we are looking at
    int curr_size = 0;
    // the size of the smallest block we can fit needed block
    int smallest_blk = 100000;
    while (curr_blk < (blk_hdr*) ((char*)first_block + total_mem_size)) {
        curr_hdr = (char*)curr_blk;
        curr_size = curr_blk -> size_status - (curr_blk -> size_status & 3);

        // check a-bit status of current block
        int status = curr_blk -> size_status & 3;
        int curr_free = 1;
        if (status % 2 == 0) {
            curr_free = 0;
        }

        // looks for blocks that are free, smaller than the current smallest free block
        // while being larger than the needed block size
        if (curr_free == 0) {
            if (curr_size > blk_size) {
                if (curr_size < smallest_blk) {
                    smallest_blk = curr_size;
                    best_blk = (blk_hdr*)curr_hdr;
                }
            }
            //found perfect fit, break
            else if (curr_size == blk_size) {
                best_blk = (blk_hdr*)curr_hdr;
                break;
            }

        }
        // increment ptr to next block
        curr_blk = (blk_hdr*)((char*)curr_blk + curr_size);
    }
    // no room anywhere for needed block
    if (best_blk == NULL) {
        return NULL;
    }
    // if the remaining free block from a split is > 8 bytes, we will split
    if (smallest_blk - blk_size < 8) {
        blk_size = smallest_blk;
        if (blk_size % 8 != 0) {
            return NULL;
        }
    }

    best_blk -> size_status = blk_size;
    // to set next block's previous p-bit to allocated
    blk_hdr* next_blk = best_blk;
    int currentBlockSize = best_blk -> size_status - (best_blk -> size_status & 3);
    //next block now points to the header of its block
    next_blk = ((void*)((char*)best_blk) + currentBlockSize);

    //diff is the size of the new free block that we split
    int diff = 0;
    if (smallest_blk - best_blk -> size_status >= 8 ) {
        diff = smallest_blk - best_blk -> size_status;
        next_blk -> size_status = diff;
    }
    blk_hdr* next_blk_footer;
    //if the next block is free, set footer
    if ((next_blk -> size_status & 1) == 0) {
        next_blk_footer = next_blk;
        next_blk_footer = (void*)(((char*)next_blk_footer) + 4);
        next_blk_footer -> size_status = diff;
        next_blk_footer -> size_status += 2;
    }
    //next block's previous block is busy
    next_blk -> size_status += 2;
    //set the current block is busy
    best_blk -> size_status += 1;
    //best_blk now points to the payload of the allocated block
    best_blk = (void*)((char*)best_blk + 4);

    return best_blk;
}

/* 
 * Function for freeing up a previously allocated block 
 * Argument - ptr: Address of the block to be freed up 
 * Returns 0 on success 
 * Returns -1 on failure 
 * Here is what this function should accomplish 
 * - Return -1 if ptr is NULL
 * - Return -1 if ptr is not 8 byte aligned or if the block is already freed
 * - Mark the block as free 
 * - Coalesce if one or both of the immediate neighbours are free 
 */                    
int Free_Mem(void *ptr) {

    if (ptr == NULL) {
        return -1;
    }
    blk_hdr *start = ptr - 4;
    // will point to current blk if prev blk is busy, if prev blk is free will point to prev blk
    blk_hdr *first_free = start;
    int ptr_size = start->size_status - (start->size_status & 3);
    char *last_address = (char *) first_block + total_mem_size;
    // checks if ptr is out of bounds
    if (start < first_block || (char *) start > last_address) {
        if (start < first_block || (char *) start > last_address) {
            return -1;
        }
        if (ptr_size % 8 != 0) {
            return -1;
        }

        blk_hdr *next_blk = (blk_hdr *) ((void *) ((char *) start) + ptr_size);
        // coalesce if the next block is free
        int next_blk_size = 0;
        int prev_ptr_size = 0;
        // footer of the rightmost free block that we coalesce
        blk_hdr *footerPtr = (blk_hdr *) (((void *) ((char *) start) + ptr_size) - 4);
        // point to the previous block if it is free
        blk_hdr *prev_blk;

        // checks status of the next block
        int next_blk_status = next_blk->size_status;
        int next_blk_free = (next_blk_status & 1);
        // if the next block is within bounds, free, status is less than total_mem_size
        if ((char *) next_blk < last_address && (next_blk_free == 0) && next_blk_status < total_mem_size) {
            // store the actual size of the next block
            next_blk_size = next_blk->size_status - (next_blk->size_status & 3);
            // footer of the next block
            footerPtr = (((void *) ((char *) next_blk) + next_blk_size) - 4);
        }
        // check status of prev block
        if ((start->size_status & 2) == 0) {
            prev_blk = start;
            // prev_blk now points at prev_footer
            prev_blk = (blk_hdr *) ((void *) ((char *) prev_blk) - 4);
            prev_ptr_size = prev_blk->size_status - (prev_blk->size_status & 3);
            // prev pointer subtracts prev blocks size from start pointer
            prev_blk = (blk_hdr *) ((void *) ((char *) start) - prev_ptr_size);
            first_free = prev_blk;
        }
        // size of coalesced free block
        int free_size = ptr_size + next_blk_size + prev_ptr_size;
        // set statuses of free block's header and footer
        footerPtr->size_status = free_size;
        footerPtr->size_status += 2;

        first_free->size_status = free_size;
        first_free->size_status += 2;

        return 0;
    }
    return -1;
}
/*
 * For testing purpose
 * To verify whether a block is double word aligned
 */
void *start_pointer;

/*
 * Function used to initialize the memory allocator
 * Not intended to be called more than once by a program
 * Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated
 * Returns 0 on success and -1 on failure 
 */
    int Init_Mem(int sizeOfRegion) {
    int pagesize;
    int padsize;
    int fd;
    int alloc_size;
    void* space_ptr;
    static int allocated_once = 0;

    if(0 != allocated_once) {
        fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
        return -1;
    }
    if(sizeOfRegion <= 0) {
        fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if(-1 == fd){
        fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == space_ptr) {
        fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }

    allocated_once = 1;

    // for double word alignement
    alloc_size -= 4;

    // Intialising total available memory size
    total_mem_size = alloc_size;

    // To begin with there is only one big free block
    // initialize heap so that first block meets double word alignement requirement
    first_block = (blk_hdr*) space_ptr + 1;
    start_pointer = space_ptr;

    // Setting up the header
    first_block->size_status = alloc_size;

    // Marking the previous block as busy
    first_block->size_status += 2;

    // Setting up the footer
    blk_hdr *footer = (blk_hdr*) ((char*)first_block + alloc_size - 4);
    footer->size_status = alloc_size;

    return 0;
    }

/* 
 * Function to be used for debugging 
 * Prints out a list of all the blocks along with the following information i
 * for each block 
 * No.      : serial number of the block 
 * Status   : free/busy 
 * Prev     : status of previous block free/busy
 * t_Begin  : address of the first byte in the block (this is where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block (as stored in the block header) (including the header/footer)
 */
    void Mem_Dump() {
        int counter;
        char status[5];
        char p_status[5];
        char *t_begin = NULL;
        char *t_end = NULL;
        int t_size;

        blk_hdr *current = first_block;
        counter = 1;

        int busy_size = 0;
        int free_size = 0;
        int is_busy;

        fprintf(stdout, "************************************Block list***\
                    ********************************\n");
        fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
        fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");

        while (current->size_status != 1) {
            t_begin = (char *) current;
            t_size = current->size_status;

            if (t_size & 1) {
                // LSB = 1 => busy block
                strcpy(status, "Busy");
                is_busy = 1;
                t_size = t_size - 1;
            } else {
                strcpy(status, "Free");
                is_busy = 0;
            }

            if (t_size & 2) {
                strcpy(p_status, "Busy");
                t_size = t_size - 2;
            } else {
                strcpy(p_status, "Free");
            }

            if (is_busy)
                busy_size += t_size;
            else
                free_size += t_size;

            t_end = t_begin + t_size - 1;

            fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status,
                    p_status, (unsigned long int) t_begin, (unsigned long int) t_end, t_size);

            current = (blk_hdr *) ((char *) current + t_size);
            counter = counter + 1;
        }

        fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
        fprintf(stdout, "***************************************************\
                    ******************************\n");
        fprintf(stdout, "Total busy size = %d\n", busy_size);
        fprintf(stdout, "Total free size = %d\n", free_size);
        fprintf(stdout, "Total size = %d\n", busy_size + free_size);
        fprintf(stdout, "***************************************************\
                    ******************************\n");
        fflush(stdout);



        // return information in the blocks


//        return;
    }