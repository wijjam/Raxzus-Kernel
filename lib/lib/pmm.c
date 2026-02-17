uint32_t bitmap[32768] // logic 4GB / 4KB = 1 048 576 pages. 1 048 576/32 bits-per-int = 32768 integers 
uint32_t max_frames = 1048576 // total pages
uint32_t used_frames = 0


void set_frame(uint32_t frame_addr) {

    uint32_t frame =  frame_addr / 4096;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;

    bitmap[index] |= (1 << offset);
}


uint32_t clear_frame(uint32_t frame_addr) {

    uint32_t frame = frame_addr / 4096;
    uint32_t index = frame / 32;
    uint32_t offset = frame % 32;

   return (bitmap[index] &= ~(1 << offset));
}

void pmm_init() {

    for (int i = 0; i <= 32768) {

        if (bitmap[i] == 0xFFFFFFFF) {

        } else {

            for (int j = 0; j <= 31) {
                
            }

        }

    }


}