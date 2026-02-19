#ifndef PMM_H
#define PMM_H
#include <stdint.h>

#define ALLOCATE 1
#define FREE 0


void set_frame(uint32_t frame_addr);
uint32_t clear_frame(uint32_t frame_addr);
void init_pmm();
uint32_t allocate_page();

#endif