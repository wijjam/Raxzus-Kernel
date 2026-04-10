#include "../include/gdt.h"

static struct gdt_entry gdt[3];
static struct gdt_ptr   gdtr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran)
{
    gdt[i].base_low   = (base & 0xFFFF);
    gdt[i].base_mid   = (base >> 16) & 0xFF;
    gdt[i].base_high  = (base >> 24) & 0xFF;

    gdt[i].limit_low  = (limit & 0xFFFF);
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);

    gdt[i].access = access;
}


static inline void gdt_load(void)
{
    asm volatile (
        "lgdt (%0)\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"   // Far jump to reload CS
        "1:\n"
        : : "r"(&gdtr) : "ax", "memory"
    );
}

void init_gdt(void)
{
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel Code: base=0, limit=4GB, ring 0, executable, readable
    // Access: 0x9A = Present | Ring0 | Code | Executable | Readable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(1, 0x00000000, 0xFFFFFFFF, 0x9A, 0xCF);

    // Kernel Data: base=0, limit=4GB, ring 0, writable
    // Access: 0x92 = Present | Ring0 | Data | Writable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(2, 0x00000000, 0xFFFFFFFF, 0x92, 0xCF);

    // Set up GDTR
    gdtr.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdtr.base  = (uint32_t)&gdt;

    gdt_load();
}

