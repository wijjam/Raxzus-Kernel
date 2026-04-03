#include "../include/process_manager.h"
#include "../include/vga.h"
#include "../include/pic.h"
#include "../include/memory.h"
#include "../include/paging_manager.h"
#include "../include/pmm.h"


struct PCB* current_process;
struct PCB* next_process;

struct PCB* process_lists;

uint16_t current_index = 0;

uint32_t blink_counter = 0;

//struct PCB process_lists[1000];

uint32_t* esp_address_variable;

uint32_t count = 0;

void timer_interrupt_handler() {

        // The round robbin functionality
        struct registers* stack = (struct registers *)esp_address_variable; 
        

        count++;

        //kprintf("ESP: %x\n", esp_address_variable);
        //kprintf("I someone there?!!!!!\n");


        //kprintf("Current_process: %x\n", current_process);
        //kprintf(" Next_process%x", next_process);

        blink_counter++;
        if (blink_counter >= 50) {
            cursor_blinker();
            blink_counter = 0;
        }
        
        schedule();
        


    

 //kprintf("This is 10ms=======================================================================\n");

    pic_send_eoi(0);
}


void create_process(void (*func)()) {

    kprintf("A process is being created\n");

    struct PCB* new_process = (void*) kmalloc(sizeof(struct PCB));

    if (new_process == (void*)0) {return (void*)0;}

    new_process->PID = current_index;
    
    new_process->sleep_time = 0;
    
    new_process->next = (void*)0;

    new_process->prev = (void*)0;
        __asm__ volatile("cli");  // disable interrupts
    
    
    new_process->page_dir = create_process_page_directory();
    //kprintf("Stored page_dir: %x\n", (uint32_t)new_process->page_dir);
    //kprintf("Returned from func: %x\n", (uint32_t)create_process_page_directory());

    

    
    new_process->heap_start = 0x1000;   // Puts heap at the bottom
    new_process->heap_end = new_process->heap_start + get_heap_size();
    new_process->stack_top = 0xEFFFF000;    // Puts stack at the top
    
    



    uint32_t stack_phys = get_next_free_process_frame();
    uint32_t* proc_dir_virt = (uint32_t*)((uint32_t)new_process->page_dir);

    //kprintf("We got %x as the address for the proc_dir", proc_dir_virt);

    map_page(proc_dir_virt, 0xEFFFF000, stack_phys, PAGE_USER);
     

    // Write stack frame through KERNEL virtual address
    uint32_t* sp = (uint32_t*)((stack_phys + 0xC0000000) + 4096);

    // Get ACTUAL CS from CPU
    uint16_t actual_cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(actual_cs));

    // IRET frame
   // *(--sp) = 0x00;                      // fake return address
    *(--sp) = 0x202;                    // EFLAGS
    *(--sp) = (uint32_t)actual_cs;      // CS <- ANVÄND RÄTT CS!
    *(--sp) = (uint32_t)func;            // argument to eip

    *(--sp) = 0x00; // ERROR CODE (ignored)

    // POPA frame
    *(--sp) = 0x00; // EAX
    *(--sp) = 0x00; // ECX
    *(--sp) = 0x00; // EDX
    *(--sp) = 0x00; // EBX
    *(--sp) = 0x00; // ESP (ignored)
    *(--sp) = 0x00; // EBP
    *(--sp) = 0x00; // ESI
    *(--sp) = 0x00; // EDI
    
    new_process->saved_esp = 0xEFFFF000 + ((uint32_t)sp - (stack_phys + 0xC0000000));


    if (current_index == 0) {
        process_lists = new_process;
        current_index = current_index + 1;
        return;
    }


    current_index = current_index + 1;

    struct PCB* current = process_lists;

    while (current->next != (void*)0) {
        current = current->next;
    }

    current->next = new_process;
    new_process->prev = current;

    for (uint32_t virt = new_process->heap_start; virt < new_process->heap_end; virt += 4096) {
    
        //kprintf("The virtual page dir is: %x\n", current_process->page_dir);
        //kprintf("The virtual address to map is: %x\n", virt);
        
        allocate_page(new_process->page_dir, virt, PAGE_USER);
        
        
    }

        uint32_t* debug_sp = (uint32_t*)((stack_phys + 0xC0000000) + 4096);
    kprintf("=== STACK DUMP (top to bottom) ===\n");
    kprintf("esp at: sp[0] = %x\n", debug_sp);
    for (int i = 0; i < 4; i++) {
        debug_sp--;
        kprintf("sp[%d] = %x\n", i, *debug_sp);

        if (*debug_sp != actual_cs && i == 1 ) {
            kprintf_red("Fucking CS got corrupted again my guy.");
            while(1){}
        }
        if (*debug_sp != 0x202 && i == 0) {
            kprintf_red("Fucking EFLAGS got corrupted again my guy!!!");
            while(1){}
        }
    }

    //kprintf("func ptr is: %x\n", (uint32_t)func);
    __asm__ volatile("sti");  // re-enable interrupts


}

void debug_print_esp(uint32_t esp_val) {
        kprintf("\n\nAbout to first switch\n");
        kprintf("Worker saved_esp: %x\n", next_process->saved_esp);
        
        // Dump worker's stack
        uint32_t* stack = (uint32_t*)next_process->saved_esp;
        for (int i = 0; i < 15; i++) {
            kprintf("  [%d] %x\n", i, stack[i]);
        }
}


void init_process_scheduler(void (*func)()) {
    current_index = 0;

    create_process(func);

    current_process = process_lists;
    next_process = process_lists;


    

    __asm__ volatile("int $0x82");

}

void schedule() {
    // The round robbin functionality
    struct PCB* sim_current = current_process;
    do  {

        if (sim_current->next != (void*)0) {
            next_process = sim_current->next;
            sim_current = next_process;
            kprintf("Hello");
           
        } else {
            next_process = process_lists;
            sim_current = next_process;
            //kprintf("No");
             
        }
        
        if (sim_current->sleep_time > 0) {
            sim_current->sleep_time = sim_current->sleep_time - 1;
        }

    } while (sim_current->sleep_time > 0);

    //__asm__ volatile("hlt");
}


void switch_interrupt_handler() {
    // The switch functionality happens in the assembly stub.
    pic_send_eoi(0);

}


/*int copy_process(uint32_t* esp_stack) {
    struct PCB* new_process = (void*) kmalloc(sizeof(struct PCB));

    if (new_process == (void*)0) {return (void*)0;}

    new_process->PID = current_index;
    new_process->sleep_time = 0;
    new_process->next = (void*)0;
    new_process->prev = (void*)0;
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

    struct registers* esp_ptr = (struct registers*)esp_stack;


    ecx = esp_ptr->ecx;
    edx = esp_ptr->edx;
    ebx = esp_ptr->ebx;
    ebp = esp_ptr->ebp;
    esi = esp_ptr->esi;
    edi = esp_ptr->edi;
    saved_ebp = esp_ptr->ebp;

    eip_address = esp_ptr->eip;


    

    __asm__ volatile("mov %%cs, %0" : "=r"(actual_cs)); // Gets the CS from the cpu, no Assumsion!


    current_PID = current_index;
    uint8_t *stack = kmalloc(4096); // We allocate our custom stack frame to the heap.
    if (stack == (void*)0) {return;} // we then check if it returns null, if the heap runs out we can not allocate a stack.
    uint32_t *child_ptr = (uint32_t*)(stack + 4096); // Since stack moves downward we make the stack pointer (esp) point at the top of the heap and then move down.
    uint32_t *sp = (uint32_t*)(current_process->saved_esp); // start process stuff fix later.
    uint32_t *new_esp;

    // IRET frame
    *(--sp) = 0x202;          // EFLAGS
    *(--sp) = actual_cs;      // CS <- ANVÄND RÄTT CS!

   

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




    for (int i = 0; i < 1024; i--) {
        *(--sp) = 
    }






    new_process->saved_esp = (uint32_t)sp;
    current_index = current_index + 1;


    struct PCB* sim_current = current_process;

    while (sim_current->next != (void*)0) {
        sim_current = sim_current->next;
    }
    sim_current->next = new_process;

    __asm__ volatile(
        "movl %0, %%eax \n"
    :
    :   "r"((uint32_t)current_PID)
    );
    

    return;
}
    */