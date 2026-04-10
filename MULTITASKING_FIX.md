# Multitasking Fix — What Was Wrong, Why It Crashed, and How It Was Fixed

---

## The Short Answer

You had **five separate bugs** that all worked together to cause the triple-fault:

1. `create_process` never assigned a `page_dir` — CR3 was loaded with garbage.
2. Every process shared the *kernel heap* for its stack — no true stack isolation.
3. No process ever had a heap of its own.
4. `init_process_scheduler` returned to `kernel_main` and sat in `hlt` forever — `idle_process` was never actually called.

Any one of these alone would break multitasking.  Together, they guaranteed a crash on the very first timer tick that tried to switch to a second process.

---

## Bug 1 — `create_process` Never Set `page_dir` (the crash cause)

### What the code did

```c
void create_process(void (*func)()) {
    struct PCB* new_process = kmalloc(sizeof(struct PCB));
    // ... built a fake stack ...
    new_process->saved_esp = (uint32_t)sp;
    // NO line that sets new_process->page_dir ← MISSING
}
```

`page_dir` is the second field in the PCB (offset 4).  Because `kmalloc` does not zero memory, it contained whatever bytes happened to be there — garbage.

### Why it crashed

When the timer fired and the scheduler decided to switch to the new process, `isr_wrapper_32` did:

```asm
movl 4(%ebx), %ecx      # load page_dir → loaded GARBAGE
subl $0xC0000000, %ecx  # subtracted kernel offset from garbage
movl %ecx, %cr3         # loaded GARBAGE into CR3
```

The moment CR3 was loaded with a garbage value, the CPU flushed the TLB and tried to use whatever physical address happened to be in `%ecx` as a page directory.  That memory contained random bytes, not valid page-directory entries.  The very next memory access (the `popa` instruction reading from the stack) caused a **page fault**.  The page fault handler itself tried to access memory (to push an error code and call the C handler), which caused a **double fault**.  The double fault handler crashed the same way, which caused a **triple fault** — and the CPU reset.

### The fix

`create_process` now calls `create_process_page_directory()` and stores the result:

```c
uint32_t* proc_dir = create_process_page_directory();
new_process->page_dir = proc_dir;
```

`create_process_page_directory()` allocates a fresh 4 KB physical frame, zeroes it, and then *copies the kernel's upper-half entries* (page-directory indices 768–1023, covering 0xC0000000 and above) into it.  This means the process can still call kernel functions and use kernel data, but the lower 3 GB of virtual address space are completely private.

---

## Bug 2 — Stack Was in the Shared Kernel Heap (no true process isolation)

### What the code did

```c
uint8_t *stack = kmalloc(4096);  // allocated from the kernel heap
uint32_t *sp = (uint32_t*)(stack + 4096);
// ... built fake frame ...
new_process->saved_esp = (uint32_t)sp;  // kernel virtual address
```

The stack was just a block inside the kernel heap.  The kernel heap is mapped identically in *every* page directory (because all of it is above 0xC0000000 and those mappings are shared).  This means:

* Process A's stack was visible to Process B and to the kernel at all times.
* Corrupting one process's stack could silently corrupt another.
* This is **not** process isolation — it is just multiple threads sharing one address space.

### The fix

Each process now gets an exclusive physical page mapped at `PROC_STACK_VIRT` (virtual `0xBFFFF000`) *only inside that process's own page directory*:

```c
uint32_t stack_phys = get_next_free_process_frame();
map_page(proc_dir, PROC_STACK_VIRT, stack_phys, PAGE_KERNEL);
```

Because `PROC_STACK_VIRT` (0xBFFFF000) is below 0xC0000000 it falls in page-directory index 767.  That index is *not* copied from the kernel directory into the process directory — it starts blank and only gets the one entry we add here.  No other process has this physical page anywhere in its address space.

While we are still in the kernel (CR3 = kernel page directory), we temporarily access the physical page through the kernel's `phys + 0xC0000000` window to write the fake frame.  Then we calculate the process-virtual equivalent of `sp`:

```c
uint32_t sp_offset = (uint32_t)sp - (stack_phys + 0xC0000000);
new_process->saved_esp = PROC_STACK_VIRT + sp_offset;
```

When `isr_wrapper_32` loads this `saved_esp` into ESP and the CPU is running under the process's own CR3, `PROC_STACK_VIRT + sp_offset` resolves correctly to the process's private physical stack page.

---

## Bug 3 — Processes Had No Heap

### What the code did

`heap_start`, `heap_end`, and `next_virt` in the PCB were never initialised.  `pmalloc()` uses `current_process->heap_start` as the start of the heap — with zero there, it would write to virtual address 0 (NULL), causing an instant crash the first time any process called `pmalloc`.

`heap_init()` existed in `memory.c` but was never called from `create_process`.  It also could not be called directly because it writes to `new_process->heap_start` as a virtual address, and that address (0x40000000) is not mapped in the *kernel's* current page directory — only in the process's own.

### The fix

We allocate a separate physical page for the heap and map it at `PROC_HEAP_VIRT` (0x40000000) in the process's page directory:

```c
uint32_t heap_phys = get_next_free_process_frame();
map_page(proc_dir, PROC_HEAP_VIRT, heap_phys, PAGE_KERNEL);
```

We then initialise the heap header through the kernel's physical window (not through the virtual PROC_HEAP_VIRT address, which is not mapped here):

```c
struct heap* proc_heap_hdr = (struct heap*)(heap_phys + 0xC0000000);
proc_heap_hdr->size      = PROC_HEAP_SIZE;
proc_heap_hdr->prev_size = 0;
set_flag (&proc_heap_hdr->size, FREE);
set_magic(&proc_heap_hdr->size, MAGIC_FIRST);

new_process->heap_start = PROC_HEAP_VIRT;
new_process->heap_end   = PROC_HEAP_VIRT + PROC_HEAP_SIZE;
```

When the process runs under its own CR3, `pmalloc` can reach `PROC_HEAP_VIRT` correctly.

---

## Bug 4 — `idle_process` Was Never Called

### What the code did

`init_process_scheduler` did this:

```c
void init_process_scheduler(void (*func)()) {
    current_index = 0;
    create_process(func);
    current_process = process_lists;
    next_process    = process_lists;
    // function returned here ↓
}
```

It returned to `kernel_main`, which enabled STI and entered a `while(1) hlt` loop.  The `idle_process` PCB existed and had a fake stack pointing to its entry address — but nobody ever switched to it.

The timer IRQ was also masked (`pic_enable_irq(0)` is commented out in `kernel_main`).  The only place it was enabled was *inside* `idle_process`:

```c
void idle_process() {
    create_process(&timer_process_worker);  // never reached
    pic_enable_irq(0);                      // never reached
    ...
}
```

Classic chicken-and-egg: the timer needed to be enabled to switch to `idle_process`, but `idle_process` was responsible for enabling the timer.

### The fix

`init_process_scheduler` now does a manual context switch into the first process at the end, and never returns:

```c
uint32_t phys_dir  = (uint32_t)current_process->page_dir - 0xC0000000;
uint32_t first_esp = current_process->saved_esp;

__asm__ volatile(
    "movl %0, %%cr3\n"   /* switch to first process's page directory */
    "movl %1, %%esp\n"   /* load its saved stack pointer             */
    "popa\n"             /* restore the zeroed fake register frame   */
    "iret\n"             /* jump to idle_process() with IF=1         */
    : : "r"(phys_dir), "r"(first_esp) : "memory"
);
```

This is exactly what `isr_wrapper_32` does on every subsequent context switch — we just do it once manually to bootstrap the first process.

`idle_process` then runs normally:
1. Calls `create_process(&timer_process_worker)` — adds PID 1.
2. Enables STI.
3. Calls `pic_enable_irq(0)` — enables the timer.
4. Enters its `while(1) hlt` loop.

The timer now fires, `schedule()` picks `timer_process_worker`, and round-robin multitasking works.

---

## Bug 5 — Wrong CR3/ESP Switch Order in `isr_wrapper_32` (and `isr_wrapper_129`)

### What the code did

```asm
movl (%ebx), %esp       # 1. load new ESP  ← WRONG ORDER
movl %ebx, current_process
movl 4(%ebx), %ecx      # 2. then load page_dir
subl $0xC0000000, %ecx
movl %ecx, %cr3         # 3. then switch CR3
```

### Why this is a problem

After fixing Bug 2, each process's stack is at `PROC_STACK_VIRT` (0xBFFFF000).  This virtual address is mapped **only in that process's page directory** — not in the old process's directory that is still active in CR3 when ESP is first changed.

With the old order: ESP is loaded to 0xBFFFF... while the *old* CR3 is still in effect.  No crash happens *at that exact instruction* because the CPU does not dereference ESP for a `movl` to ESP — it just changes the register.  But it is fragile: any instruction that *uses* ESP (like `push`, `call`, or exception handling) before CR3 is switched would page-fault, because 0xBFFFF... is not mapped yet.

Between the wrong `movl (%ebx), %esp` and the `movl %ecx, %cr3` there are only register operations, so in practice the kernel survives — but it is one accidental memory access away from a crash.

### The fix

Switch CR3 *first*, then load ESP.  After CR3 is live, the new stack address is mapped and everything is safe:

```asm
movl 4(%ebx), %ecx      # 1. load page_dir (from PCB in kernel space — always safe)
subl $0xC0000000, %ecx
movl %ecx, %cr3         # 2. switch CR3 — new stack is now mapped

movl %ebx, current_process

movl (%ebx), %esp       # 3. THEN load new ESP — correct page directory is active
```

---

## Summary Table

| # | File | What was wrong | What was fixed |
|---|------|---------------|----------------|
| 1 | `lib/process_manager.c` | `page_dir` never assigned → garbage CR3 → triple fault | Call `create_process_page_directory()` and store result |
| 2 | `lib/process_manager.c` | Stack allocated in shared kernel heap → no isolation | Allocate physical page, map at `PROC_STACK_VIRT` in private page dir |
| 3 | `lib/process_manager.c` | No heap per process → `pmalloc` writes to NULL | Allocate physical page, map at `PROC_HEAP_VIRT`, init header |
| 4 | `lib/process_manager.c` | `init_process_scheduler` returned → `idle_process` never ran | Manually do CR3+ESP+popa+iret at the end to boot first process |
| 5 | `interrupt_stubs.s` | ESP loaded before CR3 switch → wrong page dir active | Load CR3 first, then ESP, in both `isr_wrapper_32` and `_129` |
| 6 | `linker.ld` + `paging_manager.c` | Kernel page tables and kernel heap occupied the same physical pages — every `kmalloc` silently corrupted the CPU's live page tables, crashing on the 7th process | Pre-allocate 1 MB for page tables in BSS (before `_heap_start`); use that fixed array in `map_kernel_memory()` instead of `get_next_free_kernel_frame()` |

---

## Virtual Address Layout After the Fix

```
Each process's virtual address space (under its own CR3):

0x00000000 – 0x3FFFFFFF   (not mapped — null pointer protection)
0x40000000 – 0x40000FFF   PROC_HEAP_VIRT  — 4 KB private heap
0x40001000 – 0xBFFEFFFF   (not mapped)
0xBFFFF000 – 0xBFFFFFFF   PROC_STACK_VIRT — 4 KB private stack, grows down
0xC0000000 – 0xFFFFFFFF   Kernel space — shared (identical in all processes)
                            • kernel code, data, BSS, heap
                            • VGA buffer, hardware MMIO
                            • PCBs, page directories, page tables
```

The kernel space region is shared by *copying* page-directory entries 768–1023 from the master kernel page directory into every new process page directory at creation time.  The lower 768 entries start blank and are never shared — that is what makes every process's stack and heap truly private.
