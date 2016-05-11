#include "vm/frame.h"
#include <stdio.h>
#include "threads/palloc.h"
#include "vm/page.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "vm/swap.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
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

static struct list frame_table;
static struct lock frame_lock;

void frame_init (void)
{
    list_init (&frame_table);
    lock_init (&frame_lock);
}

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
/*
    if (is_evict){
	lock_release (&frame_lock);
    }
*/  
    //printf ("current thread tid : %d\n", thread_current ()->tid);

    //printf ("alloc user: %d, user_name: %s\n", fr->user->tid, fr->user->name);

    //printf ("alloc uaddr : %p\n", uaddr);

    //printf ("current page list: %zu\n", list_size (&curr->page_table));

    lock_acquire (&frame_lock);
    list_push_back (&frame_table, &fr->frame_elem);
    lock_release (&frame_lock);

    return fr;
}

void free_frame (struct frame* fr)
{
    list_remove (&fr->frame_elem); 
    palloc_free_page (fr->paddr);
    free (fr);
}


void free_frame_thread (void)
{
    struct list_elem *el;
    struct thread* curr = thread_current ();
    struct frame *fr = NULL;

    int iteration = 0;
    lock_acquire (&frame_lock);
    for (el = list_begin (&frame_table); el != list_end (&frame_table);)
    {
	fr = list_entry (el, struct frame, frame_elem);
	if (fr->user == curr)
	{
	    el = list_remove (el);
	    //palloc_free_page (fr->paddr);
	    free (fr);
	}
	else
	    el = list_next (el);
    }    
    lock_release (&frame_lock);
}



//team10: find frame to evict, and call dump_frame() to actually evict

void* evict_frame(void)
{
    //printf ("===evict_frame===\n");
    struct list_elem *el, *popel;
    struct frame *fr = NULL;
    struct page *frpg = NULL;
    bool dirty, accessed;
    void* page = NULL;

    lock_acquire (&frame_lock);
    while (true){
	//printf("searching for frame to evict...\n");
	for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next(el)){ //traverse list to find unreferenced and unchanged frame
	    fr = list_entry (el, struct frame, frame_elem);
	    frpg = fr->sup_page;

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if (!accessed && !dirty && !(frpg->is_loading)){
		page = dump_frame(fr, dirty);
		//printf("evict frame exit\n");
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
		//printf("evict frame exit\n");
		return page;
	    }
	    if (accessed && !(frpg->is_loading)){
	        pagedir_set_accessed (fr->user->pagedir, fr->vaddr, false);/*
		//put the frame to the rear part of the list
		//erase list_next(el) on for loop; make else el=next(el) to go to the next this part is not executed.
		popel = el;
		el = list_remove (popel);
		list_push_back (&frame_table, popel); */
	    }/*
	    else
		el = list_next (el);*/
	}
    }

    lock_release (&frame_lock);
    return page;
}    


/*
void* evict_frame (void)
{
    //printf ("evict start|thread_tid : %d\n", thread_current()->tid);
    lock_acquire (&frame_lock);
    struct list_elem *e = list_begin(&frame_table);
    int iteration = 0; 
   
    while (true)
    {
	struct frame *fte = list_entry(e, struct frame, frame_elem);
	struct page *pg = fte->sup_page;
#if 0 
	//if (pg == NULL)
	if (1)
	{
	    printf ("=======eviction fail========\n");
	    if (fte->user == NULL)
		printf ("user NULL\n");
	    printf ("user: %s\n", fte->user->name);
	    printf ("user: %d\n", fte->user->tid);
	    printf ("user page: %p\n", fte->paddr);
	    printf ("curr: %d\n", thread_current ()->tid);
	    printf ("pg: %p, pg_vaddr: %p, pg_save_location: %d\n", pg, pg->vaddr, pg->save_location);
	    //e = list_next (e);
	    //continue;
	}
#endif
	if (!pg->is_loading)
	{
	    struct thread *t = fte->user;
	    if (pagedir_is_accessed(t->pagedir, fte->vaddr))
	    {
		pagedir_set_accessed(t->pagedir, fte->vaddr, false);
	    }
	    else
	    {
		printf("%d, %d\n", pagedir_is_dirty(t->pagedir, fte->vaddr), pg->save_location);
		if (pagedir_is_dirty(t->pagedir, fte->vaddr)
			||	pg->save_location != IN_FILE)
		{
		    pg->save_location = IN_SWAP;
		    pg->page_idx = swap_write(fte->paddr);
		    //printf ("swap_write: %zu, tid: %d\n", pg->page_idx, thread_current ()->tid);
		    //printf ("===evict to swap===");

		}
		list_remove(&fte->frame_elem);
		pagedir_clear_page(t->pagedir, fte->vaddr);
		palloc_free_page(fte->paddr);
		//lock_release (&frame_lock);
		free(fte);
		return palloc_get_page (PAL_USER);
	    }
	}

	//printf ("before %p, next %p\n", e->prev, e->next);
	//printf ("list end: %p\n", list_end (&frame_table));
	e = list_next (e);		

	if (e == list_end (&frame_table))
	{
	    e = list_begin (&frame_table);
	}

	iteration ++;
    }
}
*/
//team10: actually evict frame
void* dump_frame (struct frame* fr, bool dirty)
{
    //printf("dump frame\n");
    struct page* pg = fr->sup_page;

    if ((dirty) || (pg->save_location != IN_FILE))
    {
	pg->save_location = IN_SWAP;
	pg->page_idx = swap_write(fr->paddr);
    }
    pagedir_clear_page(fr->user->pagedir, fr->vaddr);
    free_frame (fr);
    //printf("dump frame exit\n");
    return palloc_get_page (PAL_USER);
}
