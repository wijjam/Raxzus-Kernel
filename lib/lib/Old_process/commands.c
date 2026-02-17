#include "../include/commands.h"
#include "../include/vga.h"
#include "../include/rtc.h"
#include "../include/memory.h"
#include "../include/process_manager.h"
#include <stdarg.h>

static int char_to_value[256] = {0};

void process_input(char* input) {

    char* command = get_first_word(input);  // "neofetch"
    char* args = get_remaining_words(input); // "open time"
    
    double cmd_hash = calculate_command_hash(command);
    
    kprintf("%f\n", cmd_hash);

    if (cmd_hash == HELP_HASH) {
        cmd_help();
    } else if (cmd_hash == NEOFETCH_HASH) {
        kprintf("%eYou called neofetch my guy!!!\n");
    } else if (cmd_hash == MEM_HASH) {
        test_kmalloc_kfree();
        print_heap();
    } else if (cmd_hash == TIME_HASH) {
        print_rtc_time();
    } else if (cmd_hash == CLEAR_HASH) {
        clearScreen();
    } else if (cmd_hash == FRACTURE_HASH) {
        kprintf("BANAN");
    }
}



char* get_first_word(char* input) {
    static char output[20];
    int index = 0;
    
    // Skip leading spaces (in case user typed "  help")
    while (*input == ' ') {
        input++;
    }
    
    // Extract first word
    while (*input != ' ' && *input != '\0' && index < 19) {  // Leave room for '\0'
        output[index] = *input;
        index++;
        input++;
    }
    output[index] = '\0';
    return output;
}


char* get_remaining_words(char* input) {
    // Skip first word
    while (*input != ' ' && *input != '\0') {
        input++;
    }
    // Skip spaces after first word
    while (*input == ' ') {
        input++;
    }
    return input;  // Return pointer to rest of string
}


// Precomputed powers of 10
static double power_table[] = {
    1.0,          // 10^0
    0.1,          // 10^-1  
    0.01,         // 10^-2
    0.001,        // 10^-3
    0.0001,       // 10^-4
    0.00001,      // 10^-5
    0.000001,     // 10^-6
    0.0000001,    // 10^-7
    0.00000001,   // 10^-8
    0.000000001,   // 10^-9
    0.0000000001,   // 10^-10
    0.00000000001,   // 10^-11
    0.000000000001,   // 10^-12
    0.0000000000001,   // 10^-13
    0.00000000000001,   // 10^-14
    0.000000000000001,   // 10^-15
    0.0000000000000001,   // 10^-16
    0.00000000000000001,   // 10^-17
    0.000000000000000001,   // 10^-18
    0.0000000000000000001,   // 10^-19
    0.00000000000000000001,   // 10^-20
    // ... add more as needed
};

double calculate_command_hash(char* command) {
    double result = 0.0;
    int decimal_places = 0;
    
    for (int i = 0; command[i] != '\0'; i++) {
        int value = char_to_value[command[i]];
        result += value * power_table[decimal_places];
        decimal_places += 2;  // Each char takes 2 decimal places
    }
    
    return result;
}

void init_char_table(void) {
    // Map a-z to 20-45
    for (int i = 0; i < 26; i++) {
        char_to_value['a' + i] = 20 + i;  // a=20, b=21, ..., z=45
        char_to_value['A' + i] = 20 + i;  // A=20, B=21, ... (case insensitive)
    }
}

void cmd_help(void) {
    kprintf("To calaculate number write in format 'calc num operation num'\n");
    kprintf("To get the awsome image write neofetch'\n");
    
}