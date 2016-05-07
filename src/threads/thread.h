#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>

//team 10
#include "threads/synch.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#define LK_DEFAULT (PRI_MIN - 1)

#define NICE_MIN -20
#define NICE_DEFAULT 0
#define NICE_MAX 20

#define P 17
#define Q 14
#define FRACTION (1<<Q)

#define CONVERT_FP(N) ((N)*(FRACTION))
#define CONVERT_INT_ZERO(X) ((X)/(FRACTION))
#define CONVERT_INT_NEAR(X) ((X) >= 0 ? ((X)+(FRACTION)/2)/(FRACTION) : ((X)-(FRACTION)/2)/(FRACTION))
#define ADD_XN(X, N) ((X) + (N)*(FRACTION))
#define SUB_XN(X, N) ((X) - (N)*(FRACTION))
#define SUB_NX(N, X) ((N)*(FRACTION) - (X))
#define MULTI_XX(X, Y) ((((int64_t)(X))*(Y))/(FRACTION))
#define MULTI_XN(X, N) ((X)*(N))
#define DIV_XX(X, Y) ((((int64_t)(X))*(FRACTION))/(Y))
#define DIV_XN(X, N) ((X)/(N))


/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority; 		        /* Priority. */
    int nice; 				/* team10: nice */    

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    /* team10: used by timer_alarm.c */
    int64_t sleep_ticks;

    /* team10: the lock which this thread want*/
    struct lock* target_lock;

    /* team10: lock list which this thread hold */
    struct list lock_list; 

    /* team10: original priority */
    int ori_priority;

    int recent_cpu;

    struct list_elem elem_cpu;

    //project3 
    struct list page_table;
    struct lock page_lock;

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */

    struct file* execute_file; //executable of this thread
    struct list open_file; 
    struct list child_list;
    struct list_elem child;
    struct thread* parent;
    struct semaphore wait; 

    int ret_status;
    bool ret_valid; //whether thread has been terminated by user or kernel
    bool past_exit; //whether thread already called  by thread_exit

#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */

extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/* team 10 functions list */ 
void thread_set_priority_target (int, struct thread*);
bool more_priority(const struct list_elem*, const struct list_elem*, void *aux UNUSED);
bool less_priority(const struct list_elem*, const struct list_elem*, void *aux UNUSED);
void thread_yield_custom (void);
void thread_yield_timer (void);
void update_load_avg (void);
void update_recent_cpu (struct thread*);
void update_recent_cpu_all (void);
void update_priority_all (void);
void update_priority (struct thread*);
void is_idle_thread (struct thread*);

/* project 2 */
struct thread* is_valid_tid (tid_t tid);

#endif /* threads/thread.h */
