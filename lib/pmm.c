#include "../include/pmm.h"
#include "../include/vga.h"
#include "../include/memory.h"

// Helper functions:

uint32_t set_bit(int index, int bit, int status); // Sets the page bit ALLOCATED/1 or FREE/0
uint32_t calculate_page(int frame_addr); // calculates which page it is we are going to FREE or ALLOCATE.
uint32_t find_index(int frame); // Finds which index in the bitmask our page lives in
uint32_t find_bit(int frame); // Finds what bit out of the 32 bits we need to flip.

// This below finds the end of kernel memory address
extern uint32_t _end;
uint32_t end_kernel_memory = (uint32_t) &_end;
uint32_t reserved_end;

uint32_t bitmap[32768]; // logic 4GB / 4KB = 1 048 576 pages. 1 048 576/32 bits-per-int = 32768 integers 
uint32_t max_frames = 1048576; // total pages
uint32_t used_frames = 0;


void set_frame(uint32_t frame_addr) {

    uint32_t frame =  calculate_page(frame_addr);
    uint32_t index = find_index(frame);
    uint32_t bit = find_bit(frame);

    set_bit(index, bit, ALLOCATE);
}


uint32_t clear_frame(uint32_t frame_addr) {

    uint32_t frame =  calculate_page(frame_addr);
    uint32_t index = find_index(frame);
    uint32_t bit = find_bit(frame);

   return set_bit(index, bit, FREE);
}


void init_pmm() {
    reserved_end = end_kernel_memory + HEAP_SIZE;
    for (int i = 0; i < 32768; i++) {
        bitmap[i] = 0; // Free the entier bitmap making it ready for use.
    }

    
    for (uint32_t addr = 0; addr < reserved_end; addr += 4096) {
        set_frame(addr);
        used_frames = used_frames + 1;
    }
}


uint32_t allocate_page() {
    uint32_t start_frame = reserved_end / 32; // In this reserved_end is the end of the kernel memory so we can just start to look at pages after it.
    uint32_t index_start = start_frame / 32;
    for (int index = index_start; index < 32768; index++) {
        if (bitmap[index] == 0xFFFFFFFF) {
            // pass
        } else {
            uint32_t bit_check = 1;
            for (int bit_position = 0; bit_position < 32; bit_position++) {
                
                if ((bitmap[index] & bit_check) == 0) {
                    uint32_t frame = (index * 32) + bit_position; // calculates the frame we are at.
                    uint32_t address = (frame * 4096);
                    set_frame(address);
                    return address;
                }
                bit_check = bit_check << 1;
            }
        }
    }

    return 0;
}  







// ========================================================== HELP FUNCTIONS ===============================================================


uint32_t set_bit(int index, int bit, int status) {
    if (status == ALLOCATE) {
        return bitmap[index] |= (1 << bit);
    } else if (status == FREE) {
        return bitmap[index] &= ~(1 << bit);
    } else {
        kprintf_red("Error: you can't set a paging bit to anything but 0 and 1.");
        return 0;
    }
}

uint32_t calculate_page(int frame_addr) {
    return frame_addr / 4096;
}

uint32_t find_index(int frame) {
    return frame / 32;
}

uint32_t find_bit(int frame) {
    return frame % 32;
}




