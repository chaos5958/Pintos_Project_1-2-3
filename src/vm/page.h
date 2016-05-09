#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include <list.h>
#include "filesys/file.h"

enum location{
    IN_SWAP,
    IN_FILE,
    NONE
};

struct page{
    uint8_t* vaddr; 
    void* save_addr;
    enum location save_location;
    bool is_writable;

    //for file
    size_t read_bytes;
    size_t zero_bytes;
    off_t ofs;

    //for swap
    size_t page_idx;

    //page table list element
    struct list_elem page_elem;
};

//void init_page (struct page*);

struct page* find_page (void*);
void init_page (void);
bool add_file_to_page (uint8_t*, void*, bool, size_t, size_t, off_t);
bool add_new_page (void*, bool);
bool load_page (struct page*);
void free_page (struct page*);
void free_page_table (struct list*);
bool load_page_file (struct page*);
bool load_page_swap (struct page*);
bool load_page_none (struct page*);

#endif
