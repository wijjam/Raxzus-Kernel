#include "../include/exceptions.h"
#include "../include/vga.h"
#include "../include/process_manager.h"

void divide_by_zero_handler() {

    kprintf("%e Divide by zero exception occurred!\n");
    kprintf("%e ehm..... no? That is not allowed in math or in the kernel. Like do you know how uppset the cpu is? You have him constantly trying to solve this equation.\n");
    kprintf("%e a/b = c => cb = a => 0 = cb - a (a can not be 0 since 0/0 is a whole other torture.) When in 0 = cb - a where a can't be 0 can it become true?");
    kprintf("%e Never think about it doing 0 = cb - a would be infinite and the cpu would be stuck in a loop.\n");
    kprintf("%e and you just like... divide by zero? That is not nice.\n");
    while (1) {} // Halt the system or perform other error handling
}

void page_fault_handler(uint32_t* stack) {


    struct registers *regs = (struct registers*)stack;

    uint32_t error_code = regs->err_code;

    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));
    
    kprintf("%e Page fault exception occurred!\n");
    kprintf("-> Faulting address: 0x%x\n", faulting_address);
    
    if (!(error_code & 0x1)) {
        kprintf("-> Page not present\n");
        kprintf("-> %eYou tried to access memory that doesn't exist\n");
    }
    if (error_code & 0x2) {
        kprintf("-> Write violation\n");
        kprintf("-> %eYou tried to write to read-only memory. Naughty.\n");
    }
    if (error_code & 0x4) {
        kprintf("-> User mode violation\n");
        kprintf("-> %eUser code tried to access kernel memory. Nice try buddy.\n");
    }
    
    while (1) {}
}
void general_protection_fault_handler(uint32_t* stack) {


    struct registers *regs = (struct registers*)stack;

    uint32_t error_code = regs->err_code;

    kprintf("%e General protection fault exception occurred!\n");

    if (error_code == 0) {
        kprintf("-> %eBad instruction or memory access\n");
        kprintf("-> %eCheck: Stack pointer, EIP, memory addresses\n");
        kprintf("-> %ePro tip: Pointers should point to ACTUAL mamoery (Chocking I know)\n");
        kprintf("-> %e So can you like not do that? Thanks.\n");
        } else {
        int external = error_code & 0x1;
        int table = (error_code >> 1) & 0x3;
        int selector_index = (error_code >> 3) & 0x1FFF;
        
        kprintf("-> Error code: 0x%x\n", error_code);
        kprintf("-> Selector index: %d\n", selector_index);
        kprintf("-> Table: %s\n", 
                table == 0 ? "GDT" : 
                table == 1 ? "IDT" : 
                table == 2 ? "LDT" : "IDT");
        kprintf("-> External: %s\n", external ? "Yes" : "No");
        
        if (table == 0) {
            kprintf("-> %eProblem with GDT entry %d\n", selector_index);
            kprintf("-> %eYour code/data segment is sus\n");
        } else if (table == 1) {
            kprintf("-> %eProblem with IDT entry %d\n", selector_index);
            kprintf("-> %eYour interrupt handler messed up\n");
        }
        
        kprintf("-> %e What have I told you about assuming segments? Stop it get some help.\n");
        kprintf("-> %eWeird error code, good luck!\n");
        kprintf("-> %e Congrats you found a legendary error, but like this is not pokemon... good luck debging you need it.\n");
    }
    kprintf("\nHalting the system...\n");
    while (1) {} // Halt the system or perform other error handling
}

