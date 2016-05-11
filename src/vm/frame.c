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

    if (thread_current ()->tid == 3)
	printf ("alloc frame 1||thread: %d\n", thread_current ()->tid);

    struct frame* fr = (struct frame*) malloc (sizeof (struct frame));
    if (fr == NULL)
	return NULL;
    if (thread_current ()->tid == 3)
	printf ("alloc frame 2||thread: %d\n", thread_current ()->tid);


    //printf("frame get success");
    fr->paddr = palloc_get_page (PAL_USER);

    if (fr->paddr == NULL)
	is_evict = true;

    while (fr->paddr == NULL)
    {
	fr->paddr = evict_frame ();
    }
    
    fr->user = thread_current ();
    fr->vaddr = uaddr; 

    if (is_evict)
	lock_release (&frame_lock);

    //bool lock_held_by_current_thread (const struct lock *);

    printf ("alloc user: %d, user_name: %s\n", fr->user->tid, fr->user->name);

    printf ("alloc uaddr : %p\n", uaddr);
    //printf ("========frame: %p=======\n", fr->paddr);
    //	return NULL;
    if (thread_current ()->tid == 3)
	printf ("alloc frame 3||thread: %d\n", thread_current ()->tid);

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

/*
void free_frame_thread (void)
{
    struct list_elem *el;
    struct thread* curr = thread_current ();

    lock_acquire (&frame_lock);
}
*/


//team10: find frame to evict, and call dump_frame() to actually evict
/*
void* evict_frame(void)
{
    printf ("===evict_frace===\n");
    struct list_elem *el, *popel;
    struct frame *fr = NULL;
    bool dirty, accessed;
    void* page = NULL;
    int k;

    lock_acquire (&frame_lock);
    for (k = 0; k < 2; k++){

	for (el = list_begin (&frame_table); el != list_end (&frame_table); el = list_next(el)){ //traverse list to find unreferenced and unchanged frame
	    fr = list_entry (el, struct frame, frame_elem);

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if ((accessed == false) && (dirty == false)){
		page = dump_frame(fr, dirty);
		lock_release (&frame_lock);
		return page;
	    }
	}

	for (el = list_begin (&frame_table); el != list_end (&frame_table); ){ //traverse list to find unreferenced but changed frame
	    fr = list_entry (el, struct frame, frame_elem);

	    dirty = pagedir_is_dirty(fr->user->pagedir, fr->vaddr);
	    accessed = pagedir_is_accessed(fr->user->pagedir, fr->vaddr);
	    if ((accessed == false) && (dirty == true)){
		page = dump_frame(fr, dirty);
		lock_release (&frame_lock);
		return page;
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
	    return page;
	    }    
	    */


void* evict_frame (void)
{
    printf ("evict start|thread_tid : %d\n", thread_current()->tid);
    lock_acquire (&frame_lock);
    struct list_elem *e = list_begin(&frame_table);
    int iteration = 0; 
   
    while (true)
    {
        printf ("iteration evict: %d", iteration);
	printf ("frame size: %zu\n", list_size (&frame_table));
	struct frame *fte = list_entry(e, struct frame, frame_elem);
	struct page *pg = fte->sup_page;
#if 1 
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
	    printf ("======eviction context\n");
	    struct thread *t = fte->user;
	    if (pagedir_is_accessed(t->pagedir, fte->vaddr))
	    {
		printf ("======access=======\n");
		pagedir_set_accessed(t->pagedir, fte->vaddr, false);
	    }
	    else
	    {
		if (pagedir_is_dirty(t->pagedir, fte->vaddr)
			||	pg->save_location != IN_FILE)
		{
		    pg->save_location = IN_SWAP;
		    pg->page_idx = swap_write(fte->paddr);
		    printf ("swap_write: %zu, tid: %d\n", pg->page_idx, thread_current ()->tid);
		    //printf ("===evict to swap===");
#if 0		
		    char* buf = (char *) malloc (PGSIZE);
		    memcpy (buf, "hhyeo swap", PGSIZE);
		    printf ("before swap: %s\n", buf);
		    size_t idx = swap_write (buf);
		    memset (buf, 0, PGSIZE);
		    printf ("swap write success\n");
		    swap_read (idx, buf);
		    printf ("after swap: %s\n", buf);
		    free (buf);

#endif 			
		}
		list_remove(&fte->frame_elem);
		pagedir_clear_page(t->pagedir, fte->vaddr);
		palloc_free_page(fte->paddr);
		//lock_release (&frame_lock);
		free(fte);
		printf ("===========eviction %d========\n", iteration);
		void *ret = palloc_get_page (PAL_USER);
		//if (ret == NULL)
		//    printf ("========eviction return fail=======\n");
		//return palloc_get_page(PAL_USER);
		return ret;
	    }
	}

	printf ("before %p, next %p\n", e->prev, e->next);
	printf ("list end: %p\n", list_end (&frame_table));
	e = list_next (e);		

	if (e == list_end (&frame_table))
	{
	    e = list_begin (&frame_table);
	    printf ("change to begin\n");
	}
	printf ("before %p, next %p\n", e->prev->prev, e->prev->next);
	printf ("list end: %p\n", list_end (&frame_table));

	/*
	if (e == list_end(&frame_table))
	{
	    e = list_begin(&frame_table);
	    printf ("change to begin\n");
	}
	*/
        //e = list_next (e);

	iteration ++;
    }
}

//team10: actually evict frame
void* dump_frame (struct frame* fr, bool dirty)
{
    struct page* pg = find_page (fr->vaddr);

    //if dirty, write to swap
    //lock_acquire(&frame_lock);
    //if (dirty)

    //if (pg->save_location != IN_FILE)
    if (dirty || pg->save_location != IN_FILE)
    {
	pg->save_location = IN_SWAP;
	pg->page_idx = swap_write(fr->paddr);


	//lock_release(&frame_lock);

	//regain page
    }

    pagedir_clear_page(fr->user->pagedir, fr->vaddr);
    free_frame (fr);

    return palloc_get_page (PAL_USER);


    // alloc_frame(fr->vaddr);
}
