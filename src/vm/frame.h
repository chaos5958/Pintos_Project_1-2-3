#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include <stdbool.h>
#include "threads/synch.h"
#include <list.h>
#include "vm/page.h"

struct lock frame_lock; //team10: lock for using frame

struct frame{
    struct thread* user; //team10: user that owns current frame
    struct page* sup_page; //team10: sup. page table of page that owns current frame
    uint8_t* vaddr; //team10: virtual address of page that owns current frame
    uint8_t* paddr; //team10: address of the frame
    struct list_elem frame_elem; //team10: the frame list
};

void frame_init (void); //team10: initiates frame settings
struct frame* alloc_frame (uint8_t*); //team10: allocates new frame
void free_frame (void*); //team10: frees frame structure and frame
void free_frame_thread (void); //team10: frees frame structure and frame owned by thread that calls this function
void* evict_frame(void); //team10: evicts frame
struct frame* find_frame (uint8_t *vaddr); //team10: finds frame that is owned by page of vaddr
#endif
