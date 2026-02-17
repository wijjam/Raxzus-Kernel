#include "../include/process_manager.h"
#include "../include/vga.h"
#include "../include/pic.h"
#include "../include/memory.h"

struct PCB* current_process;
struct PCB* next_process;

struct PCB process_lists[1000];
uint32_t current_index = 0;

void timer_interrupt_handler(uint32_t esp_address_variable) {
            
        // The round robbin functionality

        struct registers* stack = (struct registers *)esp_address_variable; 

    if (stack->eax != SLEEP_SYSCALL_MAGIC) {
        for (int i = 0; i < current_index; i++) {
            if (process_lists[i].sleep_time > 0) {
                process_lists[i].sleep_time = process_lists[i].sleep_time - 1;
            }
        }
    }

    schedule();

 //kprintf("This is 10ms=======================================================================\n");

    pic_send_eoi(0);
}

void create_process(void (*func)()) {
    process_lists[current_index].PID = (uint16_t)current_index; // Gives the process a PID that is explicitly its own.
    process_lists[current_index].sleep_time = 0; // assings the sleep time to 0 for the process being created.
    // Get ACTUAL CS from CPU
    uint16_t actual_cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(actual_cs)); // Gets the CS from the cpu, no Assumsion!

    uint8_t *stack = kmalloc(4096); // We allocate our custom stack frame to the heap.
    if (stack == (void*)0) {return;} // we then check if it returns null, if the heap runs out we can not allocate a stack.
    uint32_t *sp = (uint32_t*)(stack + 4096); // Since stack moves downward we make the stack pointer (esp) point at the top of the heap and then move down.

    // IRET frame
    *(--sp) = 0x202;          // EFLAGS
    *(--sp) = actual_cs;      // CS <- ANVÄND RÄTT CS!
    *(--sp) = (uint32_t)func; // EIP

    // POPA frame
    *(--sp) = 0x00; // EAX
    *(--sp) = 0x00; // ECX
    *(--sp) = 0x00; // EDX
    *(--sp) = 0x00; // EBX
    *(--sp) = 0x00; // ESP (ignored)
    *(--sp) = 0x00; // EBP
    *(--sp) = 0x00; // ESI
    *(--sp) = 0x00; // EDI

    process_lists[current_index].saved_esp = (uint32_t)sp;
    current_index++;
}

void init_process_scheduler(void) {
    current_process = &process_lists[0];
    next_process = &process_lists[0];
}

void schedule() {
    int current_idx = (current_process - process_lists);
    int next_idx = (current_idx + 1) % current_index;
    
        // Keep looking until we find one that's awake (or hit idle process 0)
    while (process_lists[next_idx].sleep_time > 0 && next_idx != 0) {
        next_idx = (next_idx + 1) % current_index;
    }
        
    next_process = &process_lists[next_idx];
}


int copy_process() {
    process_lists[current_index].PID = (uint16_t)current_index; // Gives the process a PID that is explicitly its own.
    process_lists[current_index].sleep_time = 0; // assings the sleep time to 0 for the process being created.
    // Get ACTUAL CS from CPU

    uint16_t current_PID;
    uint16_t actual_cs;
    uint32_t eip_address;
    //uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t esp_value;
    uint32_t saved_ebp;



    __asm__ volatile("movl %%esp, %0" : "=r"(esp_value) : : "memory");

    struct registers* esp_ptr = (struct registers*)esp_value;


    ecx = esp_ptr->ecx;
    edx = esp_ptr->edx;
    ebx = esp_ptr->ebx;
    ebp = esp_ptr->ebp;
    esi = esp_ptr->esi;
    edi = esp_ptr->edi;
    saved_ebp = esp_ptr->ebp;

    eip_address = *((int*)(saved_ebp + 4));

    __asm__ volatile("mov %%cs, %0" : "=r"(actual_cs)); // Gets the CS from the cpu, no Assumsion!

    current_PID = current_index;
    uint8_t *stack = kmalloc(4096); // We allocate our custom stack frame to the heap.
    if (stack == (void*)0) {return;} // we then check if it returns null, if the heap runs out we can not allocate a stack.
    uint32_t *sp = (uint32_t*)(stack + 4096); // Since stack moves downward we make the stack pointer (esp) point at the top of the heap and then move down.

    // IRET frame
    *(--sp) = 0x202;          // EFLAGS
    *(--sp) = actual_cs;      // CS <- ANVÄND RÄTT CS!

    //__asm__ volatile("hlt");

    *(--sp) = (uint32_t)eip_address; // EIP

    // POPA frame
    *(--sp) = 0x00; // EAX
    *(--sp) = ecx; // ECX
    *(--sp) = edx; // EDX
    *(--sp) = ebx; // EBX
    *(--sp) = 0x00; // ESP (ignored)
    *(--sp) = ebp; // EBP
    *(--sp) = esi; // ESI
    *(--sp) = edi; // EDI

    process_lists[current_index].saved_esp = (uint32_t)sp;
    current_index++;

    __asm__ volatile(
        "movl %0, %%eax \n"
    :
    :   "r"((uint32_t)current_PID)
    );

    return;
}