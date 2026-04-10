#include <stdint.h>

void init_gdt(void);

// A GDT entry is 8 bytes
struct gdt_entry {
    uint16_t limit_low;    // Lower 16 bits of limit
    uint16_t base_low;     // Lower 16 bits of base
    uint8_t  base_mid;     // Middle 8 bits of base
    uint8_t  access;       // Access byte
    uint8_t  granularity;  // Flags + upper 4 bits of limit
    uint8_t  base_high;    // Upper 8 bits of base
} __attribute__((packed));

// The GDTR structure (pointer passed to lgdt)
struct gdt_ptr {
    uint16_t limit;        // Size of GDT - 1
    uint32_t base;         // Linear address of GDT
} __attribute__((packed));