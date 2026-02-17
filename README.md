# Raxzer OS — Custom x86 Kernel

A monolithic x86 kernel built from scratch in C and Assembly. This is a personal learning project aimed at understanding how operating systems work at the lowest level — from bootloading to virtual memory.

## Features

- **Bootloader** — GRUB multiboot, boots via ISO in QEMU
- **VGA Text Mode** — Direct video memory output at `0xB8000` with color support
- **IDT & PIC** — Full interrupt descriptor table with remapped PIC (IRQ 0–15 mapped to IDT 32–47)
- **PIT Timer** — Programmable interval timer at 100Hz (10ms intervals) driving the scheduler
- **Dynamic Heap** — Custom `kmalloc`/`kfree` with block headers, 8-byte alignment, and coalescing free blocks
- **Preemptive Multitasking** — Round-robin scheduler with timer-driven context switching
- **Context Switching** — ESP-based process switching via assembly, saving/restoring full CPU state
- **Process Control Block (PCB)** — Tracks saved ESP, PID, and process state
- **Fork** — Spawns child processes with independent stacks and unique PIDs
- **Idle Process** — Always-running fallback process when all others sleep
- **Virtual Memory / Paging** — x86 two-level page tables (Page Directory + Page Tables), MMU-based address translation, kernel mapped at `0xC0100000` (higher-half kernel)
- **System Calls** — `int 0x80` based dispatch using EAX as syscall code

## Memory Layout

```
[ GRUB Bootloader ] [ Kernel ] [ Paging Tables ] [ Heap (1MB) ]
  0x00100000          ~1MB+       After kernel       After paging
```

After paging is enabled, the kernel is mapped to virtual address `0xC0100000` while physical memory starts at `0x00100000`.

## Building & Running

> Requires: `i686-elf-gcc`, `nasm`, `grub-mkrescue`, `qemu-system-i386`

```bash
# Build
./run.sh

# Or manually:
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o lib/io.o lib/vga.o -lgcc
grub-mkrescue -o myos.iso isodir

# Run in QEMU
qemu-system-i386 -cdrom myos.iso
```

## Tech Stack

- **Language:** C, x86 Assembly
- **Toolchain:** i686-elf-gcc, NASM, GRUB, QEMU
- **Architecture:** x86 (32-bit protected mode)

## Status

Active development. Current focus: higher-half kernel and virtual memory.
