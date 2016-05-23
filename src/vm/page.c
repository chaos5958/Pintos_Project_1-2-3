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

//lock for using page_table
struct lock page_lock;

//function: init_page
//initialize page_lock
void init_page (void)
{
    lock_init (&page_lock);
}

//function: find_page
//find a page in the supplementary page table which virtual address is vaddr_
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

//function: add_file_to_page
//crete a supplementary page for loading from file 
//1. create a supplementary page
//2. fill page element information
//3. push into the supplementary page table  
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
    pg->is_loading = false;
    pg->is_loaded = false;

    lock_acquire (&curr->page_lock); 
    list_push_back (&curr->page_table, &pg->page_elem);
    lock_release (&curr->page_lock);
    return true;
}

bool add_mmap_to_page (uint8_t* vaddr_, void* save_addr_, uint32_t read_bytes_, uint32_t zero_bytes_, off_t ofs_)
{
    //create mmap sup-page
    struct page* pg = (struct page*) malloc (sizeof (struct page));
    if (pg == NULL)
	return false;
    struct thread* curr = thread_current ();
    pg->vaddr = vaddr_; 
    pg->save_addr = save_addr_;
    pg->save_location = IN_MMAP;
    pg->is_writable = true;
    pg->read_bytes = read_bytes_;
    pg->zero_bytes = zero_bytes_;
    pg->ofs = ofs_;
    pg->is_loading = false;
    pg->is_loaded = false;
    //create mmap to track it 
    struct mmap *mp = (struct mmap *) malloc (sizeof (struct mmap));
    if (mp == NULL)
    {
	free (pg);
	return false;
    }

    mp->mmap_id = thread_current ()->map_id;
    mp->sup_page = pg;

    lock_acquire (&curr->page_lock); 
    list_push_back (&curr->page_table, &pg->page_elem);
    list_push_back (&curr->map_list, &mp->mmap_elem);
    lock_release (&curr->page_lock);

    return true;
}

//function: add_new_page
//create a new supplementary page for stack growth 
//1. create a supplementary page
//2. fill page elemt infromation
//3. push into the supplementary page table
bool add_new_page (void* vaddr_, bool is_writable_)
{
    
    struct page* pg = (struct page*) malloc (sizeof (struct page));
    if (pg == NULL)
	return false;
    struct thread* curr = thread_current ();
    pg->vaddr = vaddr_; 
    pg->save_location = NONE;
    pg->is_writable = is_writable_;
    pg->is_loading = false;
    pg->is_loaded = false;

    lock_acquire (&curr->page_lock); 
    list_push_back (&curr->page_table, &pg->page_elem);
    lock_release (&curr->page_lock);
    
    //impl
    //mmap table && doing sth...


    return true;
}

//fucntion: load_page
//load a page with address pg
bool load_page (struct page* pg)
{

    pg->is_loading = true;

    switch (pg->save_location)
    {
	//page is read from a file
	case IN_FILE: if (!load_page_file (pg))
			  return false;
		      break;

  	//page is read from the swap disk		      
	case IN_SWAP: if (!load_page_swap (pg))
			  return false;
		      break;
	
	case IN_MMAP: if (!load_page_file (pg))
			  return false;
		      break; 

	//page is created for stack growth		      
	case NONE:  if (!load_page_none (pg))
			  return false;
		      break;
    }    

    pg->is_loading = false;
    pg->is_loaded = true;
    return true;
}

//fuction: load_page_file
//load a page with address pg from the file
bool load_page_file (struct page* pg)
{

    struct frame* fr = alloc_frame (pg->vaddr);
    fr->sup_page = pg; 

    //if (pg->save_location == IN_MMAP)
//	printf ("mmap is located in %p\n", fr->paddr);

    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }

    if (fr->paddr == NULL)
	PANIC ("frame paddr null");

    if (pg->read_bytes > 0)
    {
	//read file from pg->save_addr and write into a frame with physicall address address fr->paddr
	lock_acquire (&file_lock);
	//printf ("pg->save_addr: %p, fr->paddr: %p, pg->readbyts: %zu, pg->pfs: %d pg->save_location: %d\n", pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs, pg->save_location);
	if (file_read_at (pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs) != (int) pg->read_bytes)
	{
	    lock_release (&file_lock);
	    free_frame (fr);
	    return false;
	}

	lock_release (&file_lock);
    }

    if (pg->zero_bytes != 0)
	memset (fr->paddr + pg->read_bytes, 0, pg->zero_bytes);

    //register pg at the page table 
    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }
    if (thread_current ()->tid ==3)

    return true;
}


//function: laod_page_swap
//load a page with address pg from the swap disk
bool load_page_swap (struct page* pg)
{
    struct frame* fr = alloc_frame (pg->vaddr);
    fr->sup_page = pg;

    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }
   
    //register pg at the page table
    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }

    //read swap disk from index page_idx and write into a frame with physicall addresss fr->paddr
    if (!swap_read (pg->page_idx, fr->paddr))
    {
	free_frame (fr);
	return false;
    }
    


    return true;
}

//function: load_page_none
//load a page for stack growth
bool load_page_none (struct page* pg)
{
    struct frame* fr = alloc_frame (pg->vaddr);    
    fr->sup_page = pg;

    if (fr == NULL)
    {
	free_frame (fr);
	return false;
    }


    //register pg at the page table
    if (!install_page (pg->vaddr, fr->paddr, pg->is_writable))
    {
	free_frame (fr);
	return false;
    }	
 
    //set all of a page contents as 0
    memset (fr->paddr, 0, PGSIZE);

    //change save_location to the swap disk 
    //because it will be save at the swap disk when this page is evicted
    pg->save_location = IN_SWAP;
    return true;
}
