#include "vm/frame.h"
#include <stdio.h>
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/malloc.h"

/*
struct frame{
    struct thread* user;
    void* vaddr;
    void* paddr; 
    struct list_elem frame_elem;
};


void frame_init (void);
void add_frame (struct frame*);
struct frame* alloc_frame (void*);
void free_frame (struct frame*);
void free_frame_table (struct frame*);
*/

struct list frame_table;
struct lock frame_lock;

void frame_init (void)
{
    list_init (&frame_table);
    lock_init (&frame_lock);
}

struct frame* alloc_frame (uint8_t* uaddr)
{
    struct frame* fr = (struct frame*) malloc (sizeof (struct frame));
    if (fr == NULL)
	return NULL;

    fr->paddr = palloc_get_page (PAL_USER);

    if (fr->paddr == NULL)
	return NULL;

    struct thread* curr = thread_current ();
    fr->user = curr; 
    fr->vaddr = uaddr; 

    list_push_back (&frame_table, &fr->frame_elem);

    return fr;
}

void free_frame (struct frame* fr)
{
    //list_remove (&fr->frame_elem); 
    //palloc_free_page (fr->paddr);
    //free (fr);
}

    
