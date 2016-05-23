#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>

#define CLOSE_ALL -1

struct lock file_lock; 

void syscall_init (void);
void exit_ext (int status);
void close_file (struct list_elem*);
void thread_munmap (void);

#endif /* userprog/syscall.h */
