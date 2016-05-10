#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <string.h>
#include "threads/synch.h"
#include "userprog/syscall.h"
#include "threads/palloc.h"
#include "vm/swap.h"
/*
enum location{
    IN_MEM,
    IN_FILE,
    IN_SWAP
};

struct page{
    void* vaddr; 
    void* save_addr;
    enum location save_location;
    bool is_writable;

    //for file
    uint32_t read_bytes;
    uint32_t zero_bytes;
    off_t ofs;
}
*/

/*
void init_page (struct page*)
{
    page->vaddr = NULL;
    page->save_Addr = NULL;
    page->is_writable = false;

    page->read_bytes = 0;
    page->zero_bytes = 0;
    page->ofs = 0;
}
*/
struct lock page_lock;

void init_page (void)
{
    lock_init (&page_lock);
}


struct page* find_page (void* vaddr_)
{
    struct thread* curr = thread_current ();
    struct list_elem* el;
    struct page* pg = NULL;

    lock_acquire (&curr->page_lock);
    for (el = list_begin (&curr->page_table); el != list_end (&curr->page_table); el = list_next (el))
    {
	pg = list_entry (el, struct page, page_elem);
	if (pg->vaddr == vaddr_)
	{
	    lock_release (&curr->page_lock);
	    return pg; 
	}
	else 
	    continue;
    }
    lock_release (&curr->page_lock);

    return NULL;
}	

bool add_file_to_page (uint8_t* vaddr_, void* save_addr_, bool is_writable_, uint32_t read_bytes_, uint32_t zero_bytes_, off_t ofs_)
{
    struct page* pg = (struct page*) malloc (sizeof (struct page));
    if (pg == NULL)
	return false;
    struct thread* curr = thread_current ();
    pg->vaddr = vaddr_; 
    pg->save_addr = save_addr_;
    pg->save_location = IN_FILE;
    pg->is_writable = is_writable_;
    pg->read_bytes = read_bytes_;
    pg->zero_bytes = zero_bytes_;
    pg->ofs = ofs_;

    //printf ("page entry num: %zu", list_size (&curr->page_table));
    
    list_push_back (&curr->page_table, &pg->page_elem);
    return true;
}

bool add_new_page (void* vaddr_, bool is_writable_)
{
    
    struct page* pg = (struct page*) malloc (sizeof (struct page));
    if (pg == NULL)
	return false;
    struct thread* curr = thread_current ();
    pg->vaddr = vaddr_; 
    pg->save_location = NONE;
    pg->is_writable = is_writable_;

    //printf ("page entry num: %zu", list_size (&curr->page_table));
    
    list_push_back (&curr->page_table, &pg->page_elem);
    return true;
}

bool load_page (struct page* pg)
{
    switch (pg->save_location)
    {
	case IN_FILE: if (!load_page_file (pg))
			  return false;
		      break;

	case IN_SWAP: if (!load_page_swap (pg))
			  return false;
		      break;
	
	case NONE:  if (!load_page_none (pg))
			  return false;
		      break;
    }    

    return true;
}

bool load_page_file (struct page* pg)
{
    //printf ("===file===\n");
    struct frame* fr = alloc_frame (pg->vaddr);
    
    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }

    if (fr->paddr == NULL)
	PANIC ("frame paddr null");

    if (pg->read_bytes > 0)
    {
	lock_acquire (&page_lock);
	if (file_read_at (pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs) != (int) pg->read_bytes)
	{
	    lock_release (&page_lock);
	    free_frame (fr);
	    return false;
	}

	lock_release (&page_lock);
    }

    if (pg->zero_bytes != 0)
	memset (fr->paddr + pg->read_bytes, 0, pg->zero_bytes);

    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }

    return true;
}

bool load_page_swap (struct page* pg)
{
    //printf ("===swap read===\n");
    struct frame* fr = alloc_frame (pg->vaddr);
    
    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }

    /*
    if (!swap_read (pg->page_idx, fr->paddr))
    {
	free_frame (fr);
	return false;
    }
    */

   
    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }

   
    if (!swap_read (pg->page_idx, fr->paddr))
    {
	free_frame (fr);
	return false;
    }
    

    //printf ("swap_read: %zu\n", pg->page_idx);

    return true;
}

bool load_page_none (struct page* pg)
{
    struct frame* fr = alloc_frame (pg->vaddr);
    
    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }

    //memset (fr->paddr, 0, PGSIZE);

    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }	
 
    memset (fr->paddr, 0, PGSIZE);

    pg->save_location = IN_SWAP;
    return true;
}

void free_page (struct page* pg)
{
}

void free_page_table (struct list* page_table)
{
}
