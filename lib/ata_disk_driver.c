#include "../include/ata_disk_driver.h"
#include "../include/vga.h"
uint8_t set_master_slave_super_lba(uint32_t lba, uint8_t master_slave); // Sets the master or slave and also finds out the top lba
uint8_t get_low_lba(uint32_t lba);
uint8_t get_mid_lba(uint32_t lba);
uint8_t get_high_lba(uint32_t lba);



void ata_wait_ready() {
    // Keep reading status port until BSY clears
    // You figure out the loop :)

    uint8_t result = inb(ATA_STATUS);

    while (result != ATA_STATUS_DRQ) {
        result = inb(ATA_STATUS);
    }


}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_ready();
    
    outb(ATA_DRIVE, set_master_slave_super_lba(lba, ATA_MASTER)); // Select master drive, LBA mode
    outb(ATA_SECTOR_COUNT, 1);                      // Read 1 sector
    outb(ATA_LBA_LOW,  get_low_lba(lba));
    outb(ATA_LBA_MID,  get_mid_lba(lba));
    outb(ATA_LBA_HIGH, get_high_lba(lba));
    outb(ATA_COMMAND, ATA_CMD_READ);
    
    ata_wait_ready();
    
    // Now read 256 uint16_t words from data port into buffer
    // You figure out this loop :)
}








// ======================================= HELPER FUNCTIONS ====================================================================


uint8_t set_master_slave_super_lba(uint32_t lba, uint8_t drive_select) {
   return drive_select | ((lba >> 24) & 0x0F);
}

uint8_t get_low_lba(uint32_t lba) {
    return (lba)      & 0xFF;
}

uint8_t get_mid_lba(uint32_t lba) {
    return (lba >> 8) & 0xFF;
}

uint8_t get_high_lba(uint32_t lba) {
    return (lba >> 16) & 0xFF;
}