#include "../include/raxzus_shell.h"
#include "../include/raxzus_shell_ui.h"
#include "../include/vga.h"
#include "../include/rtc.h"
#include "../include/memory.h"
#include "../include/process_manager.h"
#include <stdarg.h>


void process_input(char* input) {


    

    if (checkString(input, "help")) {
        cmd_help();
    } else if (checkString(input, "neofetch")) {
        cmd_neofetch();
    } else if (checkString(input, "mem")) {
        test_kmalloc_kfree();
    } else if (checkString(input, "time")) {
        print_rtc_time();
    } else if (checkString(input, "clear")) {
        clearScreen();
    } else {
        kprintf("\n%eCommand not found\n");
    }

}

