#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define SLEEP_SYSCALL_MAGIC 555555
#define MAX_PROCESSES 100


#include <stdint.h>

typedef struct registers { // This is a map for the stack. Nothing more

    uint32_t edi;       // [0]
    uint32_t esi;       // [1]
    uint32_t ebp;       // [2]
    uint32_t esp;       // [3]
    uint32_t ebx;       // [4]
    uint32_t edx;       // [5]
    uint32_t ecx;       // [6]
    uint32_t eax;       // [7]
    
    uint32_t err_code;  // [8]

    uint32_t eip;       // [9]
    uint32_t cs;        // [10]
    uint32_t eflags;    // [11]

    uint32_t user_esp;  // [12]
    uint32_t ss;        // [13]

} __attribute__((packed)); // __attribute__((packed)) just tells the  compiler to not add padding

typedef struct PCB {
    uint32_t saved_esp;
    uint32_t* page_dir;
    uint32_t heap_start;
    uint32_t heap_end;
    uint32_t stack_top;
    uint32_t next_virt;
    struct registers reg;
    uint32_t PID;
    int sleep_time;
    struct PCB* next;
    struct PCB* prev;
};

extern struct PCB* current_process;
extern struct PCB* next_process;

int copy_process(uint32_t* esp_stack);

void create_process(void (*function_address)());

void init_process_scheduler(void (*func)());

void schedule();

void debug_print_esp(uint32_t esp_val);


#endif