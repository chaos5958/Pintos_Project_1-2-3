#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

//team 10
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "filesys/off_t.h"
#include "vm/page.h"
#include "vm/frame.h"
#include <user/syscall.h>
#include "userprog/pagedir.h"
#include "threads/init.h"
#include "devices/input.h"
//typedef int pid_t;
//typedef int mapid_t;
#define FD_START 0 

struct file_fd 
{
    struct file* file;
    int fd;
    struct list_elem fd_elem;
    struct list_elem fd_thread; 
};

static struct list file_list;
//static struct lock file_lock; 

static void syscall_handler (struct intr_frame *);

static void halt (void);
static void exit (int status);
static pid_t exec (const char *file);
static int wait (pid_t);
static bool create (const char *file, unsigned initial_size);
static bool remove (const char *file);
static int open (const char *file);
static int filesize (int fd);
static int read (int fd, void *buffer, unsigned length);
static int write (int fd, const void *buffer, unsigned length);
static void seek (int fd, unsigned position);
static unsigned tell (int fd);
static void close (int fd);
//static struct file* find_file (int fd); 
static mapid_t mmap(int fd, void *addr);
static void munmap (mapid_t mapid);
static int get_fd (void);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init (&file_list);
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  int *ptr = f->esp; 

  if (!is_user_vaddr (ptr))  
      goto done;
  
  if (!pagedir_get_page (thread_current ()->pagedir, ptr))
      goto done;

  if (*ptr < SYS_HALT || *ptr > SYS_INUMBER)
      goto done;

  switch (*ptr)

  {
      case SYS_HALT: halt();
		     break;
      case SYS_EXIT: if (!is_user_vaddr (ptr + 1))
    			 goto done;
		     else{
		     exit (*(ptr + 1));
		     break;
     		     }

      case SYS_EXEC: if (!is_user_vaddr (ptr + 1))
			 goto done;
		     else{
			 pid_t ret = exec (*(ptr + 1));
			 f->eax = ret;
			 break;
		     }

      case SYS_WAIT: if (!is_user_vaddr (ptr + 1))
			 goto done;
		     else{
			 int ret = wait (*(ptr + 1));
			 f->eax = ret;
			 break;
		     }

      case SYS_CREATE: if (!is_user_vaddr (ptr + 1) || !is_user_vaddr (ptr + 2))
			   goto done;
		       else{
			   bool ret = create (*(ptr + 1), *(ptr + 2));
			   f->eax = ret;
		           break;
		       }

      case SYS_REMOVE: if (!is_user_vaddr (ptr + 1))
		           goto done;
		       else{
			   bool ret = remove (*(ptr + 1));
			   f->eax = ret;
			   break;
		       }

      case SYS_OPEN: if (!is_user_vaddr (ptr + 1))
			 goto done;
		     else{
			 int ret = open (*(ptr + 1));
			 f->eax = ret;
			 break;
		     }

      case SYS_FILESIZE: if (!is_user_vaddr (ptr + 1))
			     goto done;
			 else{
			     int ret = filesize (*(ptr + 1));
			     f->eax = ret;
			     break;
			 }

      case SYS_READ:  if (!is_user_vaddr (ptr + 1) || !is_user_vaddr (ptr + 2)
			     || !is_user_vaddr (ptr + 3))
			 goto done;
		     else{
			 int ret = read (*(ptr + 1), *(ptr + 2), *(ptr + 3));
			 f->eax = ret;
			 break;
		     }

      case SYS_WRITE:  
		       if (!is_user_vaddr (ptr + 1) || !is_user_vaddr (ptr + 2)
			      || !is_user_vaddr (ptr + 3))
 			   goto done;		        
		      else{
			  int ret = write (*(ptr + 1), *(ptr + 2), *(ptr + 3));
			  f->eax = ret;
			  break;
		      }

      case SYS_SEEK: if(!is_user_vaddr (ptr + 1) || !is_user_vaddr (ptr + 2))
			 goto done;
		     else{
			 seek (*(ptr + 1), *(ptr + 2));
			 break;
		     }

      case SYS_TELL: if(!is_user_vaddr (ptr + 1))
			 goto done;
		     else{
			 unsigned ret = tell (*(ptr + 1));
			 f->eax = ret;
			 break;
		     }

      case SYS_CLOSE: if (!is_user_vaddr (ptr + 1) || !is_user_vaddr (ptr + 2))
			  goto done;
		      else{
			  close (*(ptr + 1));
			  break;
		      }
      case SYS_MMAP: if (!is_user_vaddr (ptr + 1))
			 goto done;
		     {
			 mapid_t ret = mmap (*(ptr + 1), *(ptr + 2));
			 f->eax = ret;
			 break;
		     }
      case SYS_MUNMAP: if (!is_user_vaddr (ptr + 1))
			  goto done;
		      else {
			  munmap (*(ptr + 1));
			  break;
		      }
  }
  return ;
done:
  exit (-1);
}

static void
halt (void) 
{
    power_off();
}

void
exit_ext (int status)
{ 
    struct thread* t = thread_current();
    //if parent waits(exists)
    if (t->parent != NULL){
    //give status to parent
      t->parent->ret_valid = true;
    }
    t->ret_status = status;
    thread_exit();
}

static void
exit (int status)
{ 
    struct thread* t = thread_current();
    //if parent waits(exists) ret_valid is true.
    if (t->parent != NULL){
      t->parent->ret_valid = true;
    }
    t->ret_status = status;
    thread_exit();
}
static pid_t
exec (const char *file)
{
    tid_t tid;

    if (file == NULL || !is_user_vaddr (file)||!pagedir_get_page (thread_current ()->pagedir, file))
	exit (-1);
	   // || !pagedir_get_page (thread_current ()->pagedir, file) || !is_user_vaddr (file))
      	//exit(-1);
    lock_acquire (&file_lock);
    tid = process_execute (file);    
    lock_release (&file_lock);

    return (pid_t) tid;
}

static int
wait (pid_t pid)
{
    return process_wait ((tid_t)pid);
}

static bool
create (const char *file, unsigned initial_size)
{
      if(file == NULL ||!is_user_vaddr (file) || !pagedir_get_page (thread_current ()->pagedir, file))  
	exit (-1);
    

    return filesys_create (file, initial_size);
}

static bool
remove (const char *file)
{
    if (file == NULL || !is_user_vaddr (file) || !pagedir_get_page (thread_current ()->pagedir, file))
        exit(-1);

    return filesys_remove (file);
}

static int
open (const char *file)
{
    int ret = -1; 

    if(file == NULL || !is_user_vaddr (file) || !pagedir_get_page (thread_current ()->pagedir, file))
	exit (-1);

    struct file* file_ = filesys_open (file);
    if (file_ == NULL) 
	goto done; 

    struct file_fd* fd_ = (struct file_fd*) malloc (sizeof (struct file_fd));
    if (fd_ == NULL)
	goto done;

    fd_->file = file_; 
    fd_->fd = get_fd();
    list_push_back (&file_list, &fd_->fd_elem);
    list_push_back (&thread_current()->open_file, &fd_->fd_thread);
    ret = fd_->fd; 

done:
   return ret;

}

static int
filesize (int fd) 
{
    int ret = -1;
    struct file_fd* file_fd; 
    struct list_elem* el;

    for (el = list_begin (&file_list); el != list_end (&file_list);
	   el = list_next (el))
    	{
	    file_fd = list_entry (el, struct file_fd, fd_elem);
    	    if (file_fd->fd == fd && file_fd->file != NULL)
	    {
            	ret = file_length (file_fd->file);
	    }

       }

    return ret;
}

static int
read (int fd, void *buffer, unsigned size)
{
    int ret = -1;
    int iteration; 
    struct file_fd* file_fd; 
    struct list_elem* el;
     
    if (buffer == NULL || !is_user_vaddr (buffer + size)) 
  //|| !pagedir_get_page (thread_current ()->pagedir, buffer + size))  
	exit (-1);


    lock_acquire (&file_lock);
    if (fd == STDIN_FILENO)
    {
	for (iteration = 0; iteration < size; iteration++)
	{
	   ((uint8_t *) buffer)[iteration] = input_getc();
	}
       	ret = size; 	
	goto done;
    }

    else if (fd == STDOUT_FILENO)
	goto done;

    else
    {
       	for (el = list_begin (&file_list); el != list_end (&file_list);
	   el = list_next (el))
    	{
	    file_fd  = list_entry (el, struct file_fd, fd_elem);
    	    if (file_fd->fd == fd && file_fd->file != NULL)
	    {
            	ret = file_read (file_fd->file, buffer, size);
		goto done;
	    }

        }
    }


done:
    lock_release (&file_lock);
    return ret;
}

static int
write (int fd, const void *buffer, unsigned size)
{
    int ret = -1;
    struct file_fd* file_fd; 
    struct list_elem* el;
        
    if(buffer == NULL || !is_user_vaddr (buffer + size) || !pagedir_get_page (thread_current ()->pagedir, buffer + size))
	exit (-1);
   

    if (fd == 1) {
	putbuf (buffer, size);
	return size;
    }

    lock_acquire (&file_lock);
    if (fd == STDIN_FILENO)
	goto done;

    else if (fd == STDOUT_FILENO)
    {
	putbuf (buffer, size);
	ret = size;
    }
    
    else{

       	for (el = list_begin (&file_list); el != list_end (&file_list);
	   el = list_next (el))
    	{
	    file_fd  = list_entry (el, struct file_fd, fd_elem);
    	    if (file_fd->fd == fd && file_fd->file != NULL)
	    {
            	ret = file_write (file_fd->file, buffer, size);
		goto done;
	    }

       }
		
    }

done:
    lock_release (&file_lock);
    return ret;
}

static void
seek (int fd, unsigned position) 
{
  struct file* file;
  struct file_fd* file_fd;
  struct list_elem* el; 

  for (el = list_begin (&file_list); el != list_end (&file_list);
	   el = list_next (el))
    	{
	    file_fd  = list_entry (el, struct file_fd, fd_elem);
    	    if (file_fd->fd == fd && file_fd->file != NULL)
	    {
            	file = file_fd->file;
		file_seek (file, position);
		return ;
	    }
       }
}

static unsigned
tell (int fd) 
{
    struct file_fd* f_fd;
    struct list_elem* el;

    for (el = list_begin (&file_list); el != list_end (&file_list);
         el = list_next (el)){
        f_fd = list_entry (el, struct file_fd, fd_elem);
        if (f_fd->fd == fd && f_fd->file != NULL){
	    return file_tell (f_fd->file);
        }
    } 
}

static void
close (int fd)
{
    struct file_fd* fd_ = NULL;
    struct thread* curr = thread_current();
    struct list_elem* el;
    
    for (el = list_begin (&curr->open_file) ;  el != list_end (&curr->open_file) ;
	     el = list_next (el))
    {
	fd_ = list_entry (el, struct file_fd, fd_thread);
	if (fd_->fd == fd) 
	    break; 
	else
	    fd_ = NULL;
    }

    if (fd_ == NULL) 
	exit (-1);

    list_remove (&fd_->fd_elem);
    list_remove (&fd_->fd_thread);
    file_close (fd_->file);
    free (fd_);

}

static int
get_fd (void)
{
    static int current_fd = 2; 
    current_fd++; 
    return current_fd - 1;
}

/*
static struct file* find_file (int fd)
{
    struct list_elem* el;
    struct thread* curr = thread_current ();
    struct file_fd* f_fd; 
    for (el = list_begin (&file_list);  el != list_end (&file_list);
	 el = list_next (el))
    {
	f_fd = list_entry (el, struct file_fd, fd_elem);
	if (f_fd->fd == fd)
	    return f_fd->file;
	else
	    return NULL;
    }
}
*/

void close_file (struct list_elem* el_)
{
    ASSERT (el_ != NULL);
     
    struct file_fd* f_fd = NULL;
    f_fd = list_entry (el_, struct file_fd, fd_thread);

    list_remove (&f_fd->fd_elem);
    file_close (f_fd->file);
    free (f_fd);
}

static mapid_t mmap (int fd, void *addr)
{
    if (!is_user_vaddr (addr) || addr == NULL || pg_ofs(addr) != 0 || fd == STDIN_FILENO || fd == STDOUT_FILENO)
	return -1;

    struct list_elem* el;
    struct file_fd* f_fd = NULL;
    size_t file_len, pos_ofs;
    struct thread *curr = thread_current ();
    struct file *mmap_file = NULL;
    void *pos;

    lock_acquire (&file_lock);
    for (el = list_begin (&curr->open_file) ;  el != list_end (&curr->open_file) ;
	    el = list_next (el))
    {
	f_fd = list_entry (el, struct file_fd, fd_thread);
	if (f_fd->fd == fd)
	{
	    mmap_file = file_reopen (f_fd->file); 
	    break;
	}

    }
    lock_release (&file_lock);

    //no file or file length is zero
    if (mmap_file == NULL || (file_len = file_length (mmap_file)) <= 0 )
    {
	if (!mmap_file)
	    file_close (mmap_file);
	return -1;
    }

    pos = addr;
    for (pos_ofs = 0; pos_ofs < file_len; ){
	if (find_page(pos) != NULL){//check if page exist	    
	    return -1;
	}
	if ((file_length - pos_ofs) >= PGSIZE){
	    pos_ofs += PGSIZE;
	}
	else{
	    pos_ofs += (file_length - pos_ofs);
	}
	pos += PGSIZE;
    }

    thread_current ()->map_id++;
    off_t ofs = 0;

    while (file_len > 0)
    {
	if (file_len > PGSIZE)
	{
	    if (!add_mmap_to_page (addr, mmap_file, PGSIZE, 0, ofs))
	    {
		munmap (thread_current ()->map_id);
		return -1;
	    }
	    ofs += PGSIZE;
	    file_len -= PGSIZE;
    	}
	else
	{
	    if (!add_mmap_to_page (addr, mmap_file, file_len, PGSIZE-file_len, ofs))
	    {
		munmap (thread_current ()->map_id);
		return -1;
	    }
	    file_len = 0; 
	}
	addr += PGSIZE;
	//printf ("mmap: %p\n", addr);
    }

    return thread_current ()->map_id;

    
}

static void munmap (mapid_t mapid)
{
    struct file *mmap_file = NULL; 
    struct mmap *mp;
    struct page *pg;
    struct frame *fr = NULL;
    struct thread *curr = thread_current ();
    struct list_elem *el = list_begin (&curr->map_list);

//    for (el = list_begin (&curr->map_list); el != list_end (&curr->map_list); el = list_next (el))


    //printf ("munmap is called\n");
    while (el != list_end (&curr->map_list))
    {
	mp = list_entry (el, struct mmap, mmap_elem);
	if (mp->mmap_id == mapid)
	{
	    pg = find_page (mp->vaddr);
	    fr = find_frame (pg->vaddr);
	    //if (fr == 0xc || fr == NULL)
	//	PANIC ("cannot find frame 1");
	    el = list_remove (&mp->mmap_elem);
	    if (!pg)
		continue;
	    else
	    {	
		mmap_file = (struct file*) pg->save_addr;
		pg->is_loading = true;
		if (pg->is_loaded)
		{
		    //printf ("page is loaded\n");
		    //lock_acquire (&frame_lock);
		    //lock_acquire (&curr->page_lock);

		    fr = find_frame (pg->vaddr);
		    if (fr == 0xc || fr == NULL)
		       PANIC ("cannot find frame");	
		    if (pagedir_is_dirty (thread_current ()->pagedir, pg->vaddr))
		    {
			//printf ("pg->save_addr: %p, fr->paddr: %p, pg->readbyts: %zu, pg->pfs: %d pg->save_location: %d\n", pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs, pg->save_location);
//printf ("page is dirty\n");
			lock_acquire (&file_lock);
			file_write_at (pg->save_addr, fr->paddr, pg->read_bytes, pg->ofs);
			lock_release (&file_lock);
		    }

		    //printf ("==1==\n");
		    if (fr == 0xc || fr == NULL)
			PANIC ("wrong frame");
		    free_frame (fr);
		    //lock_release (&frame_lock);

		    //printf ("==2==\n");
		    lock_acquire (&curr->page_lock);
		    list_remove (&pg->page_elem);
		    lock_release (&curr->page_lock);

		    free (pg);
		    free (mp);
		    //printf ("==3==\n");
		}
		else
		{
		    //printf ("page is not loaded\n");
		    lock_acquire (&curr->page_lock);
		    list_remove (&pg->page_elem);
		    lock_release (&curr->page_lock);

		    free (pg);		   
		    free (mp); 
		}
	    }
	}
	else
	    el = list_next (el);

    }
   // printf ("munmap done\n");
    file_close (mmap_file);
}

void thread_munmap (void)
{
    //printf ("thread_unmap called, tid: %d\n", thread_current ()->tid);
    struct mmap *mp;
    struct thread *curr = thread_current ();
    struct list_elem *el = list_begin (&curr->map_list);
    mapid_t mapid = 1;

    //for (mapid = 1 ; mapid <= 1 ; mapid++)
    //
    //printf ("curr_mapid: %d, mapid: %d\n", curr->map_id, mapid);
//	printf ("thread_name: %s thread_tid: %d\n", curr->name, curr->tid);

	munmap (mapid);
    //}
    //printf ("thread_unmap end, tid: %d\n", thread_current ()->tid);
}
