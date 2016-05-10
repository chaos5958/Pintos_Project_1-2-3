#include "vm/swap.h"
#include "devices/disk.h"
#include <bitmap.h>
#include <round.h>
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <string.h>

static struct swap mem_swap;

struct swap
{
    struct lock lock;
    struct bitmap* used_map;
    struct disk* disk;
    uint8_t* base;
};

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

bool swap_read (size_t page_idx, void* frame)
{
    //printf ("swap read call\n");
    size_t i;

    lock_acquire (&mem_swap.lock);
    if (!bitmap_contains (mem_swap.used_map, page_idx, PAGE_DISK_SECTOR - 1, true))
    {
	lock_release (&mem_swap.lock);
	return false;
    }
    lock_release (&mem_swap.lock);

    //printf ("swap read 1\n");
    for (i = page_idx; i < page_idx + PAGE_DISK_SECTOR; i++)
    {
	disk_read (mem_swap.disk, i, frame + DISK_SECTOR_SIZE * (i - page_idx));
    }

    bitmap_set_multiple (mem_swap.used_map, page_idx, PAGE_DISK_SECTOR, false);
    //printf ("swap read 2\n");
    //printf ("frame: %p", frame);

    return true;
}



    




