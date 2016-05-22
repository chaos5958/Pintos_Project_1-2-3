#include "vm/frame.h"
#include <stdio.h>
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

#include "userprog/syscall.h"

static struct list frame_table;
static struct lock frame_lock;

void* dump_frame (struct frame* , bool);

/*team10: initiates frame table and lock*/
void frame_init (void)
{
    list_init (&frame_table);
    lock_init (&frame_lock);
}

/*team10: allocate new frame to virtual address uaddr
 * returns pointer to frame structure of newly allocated frame*/
struct frame* alloc_frame (uint8_t* uaddr)
{
    bool is_evict = false;

    struct frame* fr = (struct frame*) malloc (sizeof (struct frame));
    if (fr == NULL)
	return NULL;

    fr->paddr = palloc_get_page (PAL_USER);

    if (fr->paddr == NULL)
	is_evict = true;

    while (fr->paddr == NULL)
    {
	fr->paddr = evict_frame ();
	lock_release (&frame_lock);
    }
    
    fr->user = thread_current ();
    fr->vaddr = uaddr; 

    lock_acquire (&frame_lock);
    list_push_back (&frame_table, &fr->frame_elem);
    lock_release (&frame_lock);

    return fr;
}

/*frees frma fr*/
void free_frame (struct frame* fr)
{
    list_remove (&fr->frame_elem); 
    palloc_free_page (fr->paddr);
    pagedir_clear_page (fr->user->pagedir, fr->vaddr);
    free (fr);
}

/*team10: releases all frames owned by current thread*/
void free_frame_thread (void)
{
    struct list_elem *el;
    struct thread* curr = thread_current ();
    struct frame *fr = NULL;

    lock_acquire (&frame_lock);
    for (el = list_begin (&frame_table); el != list_end (&frame_table);)
    {
	fr = list_entry (el, struct frame, frame_elem);
	if (fr->user == curr)
	{
	    el = list_remove (el);
	    palloc_free_page (fr->paddr);
	    pagedir_clear_page (curr->pagedir, fr->vaddr);
	    free (fr);
	}
	else
	    el = list_next (el);
    }    
    lock_release (&frame_lock);
}

/*team10: find frame to evict, and call dump_frame() to actually evict
 * returns newly allocated page*/
void* evict_frame(void)
{
    struct list_elem *el;
    struct frame *fr = NULL;
    struct page *frpg = NULL;
    bool dirty, accessed;
    void* page = NULL;

    lock_acquire (&frame_lock);
    
    while (true){
	for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next(el)){ //traverse list to find unreferenced and unchanged frame
	    fr = list_entry (el, struct frame, frame_elem);
	    frpg = fr->sup_page;

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if (!accessed && !dirty && !(frpg->is_loading)){
		page = dump_frame(fr, dirty);
		return page;
	    }
	}

	for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next (el)){ //traverse list to find unreferenced but changed frame
	    fr = list_entry (el, struct frame, frame_elem);
	    frpg = fr->sup_page;

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if (!accessed && dirty && !(frpg->is_loading)){
		page = dump_frame(fr, dirty);
		return page;
	    }
	    if (accessed && !(frpg->is_loading)){
	        pagedir_set_accessed (fr->user->pagedir, fr->vaddr, false);
	    }
	}
    }

    lock_release (&frame_lock);
    return page;
}    

/*team10: evict frame fr according to its dirty value(dirty value indicates whether the frame has been modified)
 * returns address of new page allocated after eviction*/
void* dump_frame (struct frame* fr, bool dirty)
{
    struct page* pg = fr->sup_page;

    if ((dirty) || (pg->save_location == IN_SWAP))
    {
	if (pg->save_location == IN_MMAP)
	{
	    lock_acquire (&file_lock);
	    file_write_at (pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs);
	    lock_release (&file_lock);
	}
	else
	{
	    pg->save_location = IN_SWAP;
	    pg->page_idx = swap_write(fr->paddr);
	}
    }
    pg->is_loaded = false;
    free_frame (fr);

    return palloc_get_page (PAL_USER);
}

struct frame *find_frame (uint8_t *vaddr)
{
    struct frame *fr;
    struct list_elem* el;

    lock_acquire (&frame_lock);
    for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next (el))
    {
	fr = list_entry (el, struct frame, frame_elem);
	
	if (fr->vaddr == vaddr)
	{
	    lock_release (&frame_lock);
	    return fr;
	}
    }
    lock_release (&frame_lock);
    return NULL;
}


