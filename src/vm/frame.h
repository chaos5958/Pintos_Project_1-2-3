#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include <stdbool.h>
#include "threads/synch.h"
#include <list.h>
#include "vm/page.h"

struct frame{
    struct thread* user;
    struct page* sup_page;
    uint8_t* vaddr;
    uint8_t* paddr; 
    struct list_elem frame_elem;
};

//void init_frame (struct frame*);

void frame_init (void);
void add_frame (struct frame*);
struct frame* alloc_frame (uint8_t*);
void free_frame (struct frame*);
void free_frame_thread (void);
void free_frame_table (struct frame*);
void* evict_frame(void);
void* dump_frame (struct frame*, bool);
#endif
