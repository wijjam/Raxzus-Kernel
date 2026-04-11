#include "../include/kutils.h"

void kmemset(void *ptr, uint32_t value, uint32_t size)
{
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++)
        p[i] = value;
}