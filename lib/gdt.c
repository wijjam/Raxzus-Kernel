#include "../include/gdt.h"
#include "../include/vga.h"

static struct gdt_entry gdt[6];
static struct tss_entry tss;
static struct gdt_ptr   gdtr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran)
{
    gdt[i].base_low    = BITS_0_TO_15(base);
    gdt[i].base_mid    = BITS_16_TO_23(base);
    gdt[i].base_high   = BITS_24_TO_31(base);
    gdt[i].limit_low   = BITS_0_TO_15(limit);
    gdt[i].granularity = BITS_16_TO_19(limit) | TOP_NIBBLE(gran);
    gdt[i].access      = access;
}

static void tss_set_entry(int i, uint32_t kernel_stack) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(struct tss_entry)-1;

    gdt_set_entry(i, base, limit, 0x89, 0x00);
    
    kmemset(&tss, 0, sizeof(tss));

    tss.ss0 = 0x10; // Setting the kernel descriptor which in our case is 0x10 because it is not cs it is ss which tells us where our esp0 is.

    tss.esp0 = kernel_stack;        // Setting the esp0 to the kernel stack for that user proces.

    tss.iomap_base = sizeof(struct tss_entry); // Essencialy says taht the I/O mapping is outside of the tss entry which means that the user process has no rights to it.

}


void tss_set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
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

static inline void tss_load(void) {
    asm volatile ("ltr %0" : : "r"((uint16_t)0x28));
}


void init_gdt(void)
{
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel Code: base=0, limit=4GB, ring 0, executable, readable
    // Access: 0x9A = Present | Ring0 | Code | Executable | Readable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(1, 0x00000000, 0xFFFFF, 0x9A, 0xCF);

    // Kernel Data: base=0, limit=4GB, ring 0, writable
    // Access: 0x92 = Present | Ring0 | Data | Writable
    // Gran:   0xCF = 4KB granularity | 32-bit
    gdt_set_entry(2, 0x00000000, 0xFFFFF, 0x92, 0xCF);

    gdt_set_entry(3, 0x00000000, 0xFFFFF, 0xFA, 0xCF);

    gdt_set_entry(4, 0x00000000, 0xFFFFF, 0xF2, 0xCF);


    void *stack = kmalloc(4096);
    if (!stack) {
    // kernel panic, you can't continue without this
    kprintf_red("Failed to allocate kernel stack for TSS");
        asm volatile("hlt");
}
    tss_set_entry(5, (uint32_t)stack + 4096);

    

    // Set up GDTR
    gdtr.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdtr.base  = (uint32_t)&gdt;

    gdt_load();
    tss_load();
}

