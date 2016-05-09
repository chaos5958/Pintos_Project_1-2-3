#ifndef VM_SWAP_H
#define VM_SWAP_H
#include <stddef.h>

#define PAGE_DISK_SECTOR (PGSIZE/DISK_SECTOR_SIZE)

void init_swap (void);
size_t swap_write (void*);
void* swap_read (size_t); 


#endif
