#include "vm/frame.h"
#include <stdio.h>
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"

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
void dump_frame (struct frame*, bool);

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

    //printf("frame get success");
    fr->paddr = palloc_get_page (PAL_USER);

    if (fr->paddr == NULL)
    {
	printf ("=========frame fail===============\n");
	evict_frame ();
	fr->paddr = palloc_get_page (PAL_USER);
    }

    printf ("========frame: %d=======\n", fr->paddr);
    //	return NULL;

    //printf("paddr get success");
    struct thread* curr = thread_current ();
    fr->user = curr; 
    fr->vaddr = uaddr; 

    list_push_back (&frame_table, &fr->frame_elem);

    return fr;
}

void free_frame (struct frame* fr)
{
    list_remove (&fr->frame_elem); 
    palloc_free_page (fr->paddr);
    free (fr);
}

//team10: find frame to evict, and call dump_frame() to actually evict
void evict_frame(void)
{
    printf ("===evict_frace===\n");
    struct list_elem *el, *popel;
    struct frame *fr = NULL;
    bool dirty, accessed;

    int k;

    lock_acquire (&frame_lock);
    for (k = 0; k < 2; k++){

	for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next(el)){ //traverse list to find unreferenced and unchanged frame
	    fr = list_entry (el, struct frame, frame_elem);

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if ((accessed == false) && (dirty == false)){
		dump_frame(fr, dirty);
		lock_release (&frame_lock);
		return;
	    }
	}

	for (el = list_begin (&frame_table); el != list_end (&frame_table); ){ //traverse list to find unreferenced but changed frame
	    fr = list_entry (el, struct frame, frame_elem);

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if ((accessed == false) && (dirty == true)){
		dump_frame(fr, dirty);
		lock_release (&frame_lock);
		return;
	    }
	    if (accessed == true){
		pagedir_set_accessed (fr->user->pagedir, fr->vaddr, false);
		popel = el;
		el = list_remove (popel);
		list_push_back (&frame_table, popel);
	    }
	    else
		el = list_next (el);
	}
    }

    lock_release (&frame_lock);
}    

//team10: actually evict frame
void dump_frame (struct frame* fr, bool dirty)
{
    struct page* pg = find_page (fr->vaddr);
    printf ("=========dump frame===========\n");

    //if dirty, write to swap
	//lock_acquire(&frame_lock);
    if (dirty)
    {
    pg->save_location = IN_SWAP;
    pg->page_idx = swap_write(fr);
	//lock_release(&frame_lock);
    }  
    //regain page
    pagedir_clear_page(fr->user->pagedir, fr->vaddr);
    free_frame (fr);
    // alloc_frame(fr->vaddr);
}
