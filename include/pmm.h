#ifndef PMM_H
#define PMM_H
#include <stdint.h>


void set_frame(uint32_t frame_addr);
uint32_t clear_frame(uint32_t frame_addr);
void pmm_init();

#endif