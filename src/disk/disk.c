#include "disk.h"
#include "io/io.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"

struct disk disk;

int disk_read_sector(int lba, int total, void* buf)
{
    // Send commands to the disk
    outb(0x1F6, (lba >> 24) | 0xE0);               // Send high bits of LBA with drive/head info
    outb(0x1F2, total);                            // Send the number of sectors to read
    outb(0x1F3, (unsigned char)(lba & 0xFF));      // Send low byte of LBA
    outb(0x1F4, (unsigned char)((lba >> 8) & 0xFF)); // Send mid-low byte of LBA
    outb(0x1F5, (unsigned char)((lba >> 16) & 0xFF)); // Send mid-high byte of LBA
    outb(0x1F7, 0x20);                             // Send the READ command

    unsigned char* ptr = (unsigned char*) buf;     // Changed to unsigned char* for byte alignment
    for (int b = 0; b < total; b++)
    {
        // Wait until the drive is ready to transfer data
        while (!(insb(0x1F7) & 0x08))
        {
            // You could add a small delay here if needed
        }

        // Copy from hard disk to memory
        for (int i = 0; i < 512; i++)              // Reading 512 bytes (1 sector) in 8-bit chunks
        {
            *ptr = insb(0x1F0);                    // Use 8-bit read to ensure correct data alignment
            ptr++;
        }
    }
    return 0;
}


void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = PEACHOS_DISK_TYPE_REAL;
    disk.sector_size = PEACHOS_SECTOR_SIZE;
    disk.id = 0;
    disk.filesystem = fs_resolve(&disk);
}

struct disk* disk_get(int index)
{
    if (index != 0)
        return 0;
    
    return &disk;
}

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    if (idisk != &disk)
    {
        return -EIO;
    }

    return disk_read_sector(lba, total, buf);
}