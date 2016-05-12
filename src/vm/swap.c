#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include <round.h>
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <string.h>

static struct swap mem_swap;

//struct swap
//1. lock: lock to read/write from/to the swap disk
//2. used_map: bitmap for the swap disk to check where is empty part 
//3. disk: disk for save swap data

struct swap
{
    struct lock lock;
    struct bitmap* used_map;
    struct disk* disk;
};


//function: init_swap
//initialize mem_swap for using swap disk
//p.s. disk_get (1, 1); means it get disk for swap
void init_swap (void)
{
    disk_sector_t mem_swap_size;
    
    mem_swap.disk = disk_get (1, 1);
    if (mem_swap.disk == NULL)
       return ;	

    mem_swap_size = disk_size (mem_swap.disk);

    lock_init (&mem_swap.lock);
    mem_swap.used_map = bitmap_create (mem_swap_size);
}

//function: swap_wrtie
//write frame (one page) into the swap disk 
//1. check bitmap for finding empty space in the swap disk
//2. call disk_write to write into the swap disk
//3. return index for allocated swap disk's space
size_t swap_write (void* frame)
{
    size_t page_idx;
    size_t i;

    lock_acquire (&mem_swap.lock);
    page_idx = bitmap_scan_and_flip (mem_swap.used_map, 0, PAGE_DISK_SECTOR, false);
    lock_release (&mem_swap.lock);

    if (page_idx == BITMAP_ERROR) 
	PANIC ("Swap space: Full");
    
    for (i = page_idx; i < page_idx + PAGE_DISK_SECTOR; i++)
    {
	disk_write (mem_swap.disk, i, frame + DISK_SECTOR_SIZE * (i - page_idx));
    }

    return page_idx;
}

//function: swap_read
//read one page starting with index page_idx and write into frame
//1. check bitmap whether wanted swap space is empty or not
//2. call dist_read to read the swap space and write into frmae
//3. return true/false as swap_read successes or fails
bool swap_read (size_t page_idx, void* frame)
{
    size_t i;

    lock_acquire (&mem_swap.lock);
    if (!bitmap_contains (mem_swap.used_map, page_idx, PAGE_DISK_SECTOR - 1, true))
    {
	lock_release (&mem_swap.lock);
	return false;
    }
    lock_release (&mem_swap.lock);

    for (i = page_idx; i < page_idx + PAGE_DISK_SECTOR; i++)
    {
	disk_read (mem_swap.disk, i, frame + DISK_SECTOR_SIZE * (i - page_idx));
    }

    bitmap_set_multiple (mem_swap.used_map, page_idx, PAGE_DISK_SECTOR, false);

    return true;
}
