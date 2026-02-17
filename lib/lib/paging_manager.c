
#include <stdint.h>
#include <stdarg.h>
#include "../include/vga.h"
#include "../include/memory.h"

extern uint32_t _paging_dir[];
extern uint32_t _paging_table1[];
extern uint32_t _end[];
volatile uint32_t* page_dir = (uint32_t*) _paging_dir;
volatile uint32_t* paging_table1 = (uint32_t*) _paging_table1;
volatile uint32_t* start_of_heap = (uint32_t*) _end;


void setup_entries() {
    for (int i = 0; i < 1024; i++) {
        page_dir[i] = 0x0; // Not present
        paging_table1[i] = 0x0; // Not present
    }
}

uint32_t get_physical_address(uint32_t virtual_address) {
    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111);

    // Kolla om directory entry finns
    if ((page_dir[dir_index] & 0x1) == 0) {
        return 0; // Inte mappat
    }
    
    // Kolla om table entry finns
    if ((paging_table1[table_index] & 0x1) == 0) {
        return 0; // Inte mappat
    }


    uint32_t physical_page = paging_table1[table_index] & ~0xFFF;
    uint32_t offset = virtual_address & 0xFFF;
    return physical_page + offset;
}

void set_CR3_register() {
    asm volatile("movl %0, %%cr3\n" : : "r"(page_dir));
}

void set_CR0_32_bit_register(int on) {
    uint32_t value;

    asm volatile("movl %%cr0, %0" : "=r"(value));  // Läs FRÅN CR0

    if (on == 1) {
        value = (value) | 0x80000000;
    } else {
        value = (value) & 0x7FFFFFFF;
    }


    asm volatile("movl %0, %%cr0" : : "r"(value)); // Skriv TILL CR0




}

void map_virtual_to_physical_address(uint32_t virtual_address, uint32_t choosen_physical_address) {

    uint32_t dir_index = (virtual_address >> 22);
    uint32_t table_index = ((virtual_address >> 12) & 0b00000000001111111111);

    // check if page is present
    if ((page_dir[dir_index] & 0x1) == 1) {
        // do nothing
    }

    if ((page_dir[dir_index] & 0x1) == 0) {
        page_dir[dir_index] = (uint32_t)paging_table1 | 0x3;
    }

    paging_table1[table_index] = (choosen_physical_address | 0x3);
}

void map_kernel_memory() {
    uint32_t heap_end = (uint32_t)start_of_heap + get_heap_size() + 2*4096;

    for (uint32_t addr = 0; addr < heap_end; addr += 4096) {
        map_virtual_to_physical_address(addr, addr);
    }
}


void init_paging() {
    kprintf_white("[MEM] Setting up entries........");
    kprintf_green("[OK]\n");
    setup_entries();
    kprintf_white("[MEM] Creating blank entries........");
    kprintf_green("[OK]\n");
    map_kernel_memory();
    kprintf_white("[MEM] Mapping memory........");
    kprintf_green("[OK]\n");
    set_CR3_register();
    kprintf_white("[MEM] Assigning CR3 to paging dir........");
    kprintf_green("[OK]\n");
    set_CR0_32_bit_register(1);
    kprintf_white("[MEM] Fliping the CR0 bit........");
    kprintf_green("[OK]\n");
}
