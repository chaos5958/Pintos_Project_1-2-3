#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include <list.h>
#include "filesys/file.h"
#include <user/syscall.h>

enum location{
    IN_SWAP,
    IN_FILE,
    IN_MMAP,
    NONE
};


//struct page
//1. vaddr: virtual addr of this page
//2. save_addr (for file): file address which page is read from it
//3. save_location: save location of this page when it is evicted 
//4. is_wribable: indicates whether this page is writable or not
//5. is_loading: indicates whether this page is being loaded
//6. read_bytes / zeroytes / ofs: read bytes, zero bytes, offset of the file where this page is located
//7. page_idx: index for swap disk
//8. page_elem: list_elem for traverse page_table list 
struct page{
    uint8_t* vaddr; 
    void* save_addr;
    enum location save_location;
    bool is_writable;
    bool is_loading;
    bool is_loaded;

    //for file
    size_t read_bytes;
    size_t zero_bytes;
    off_t ofs;

    //for swap
    size_t page_idx;

    //page table list element
    struct list_elem page_elem;
};

//struct mmap
//1. mmap_id: mapping id of mmaped file
//2. sup_page: sup. page struct. of page of the mmaped file
//3. mmap_elem: list of mmaped files of a thread
struct mmap{
    mapid_t mmap_id;
    struct page *sup_page;

    struct list_elem mmap_elem;
};

struct page* find_page (void*); //find page of virtual address
void init_page (void); //initiates page settings
bool add_file_to_page (uint8_t*, void*, bool, size_t, size_t, off_t); //crete a supplementary page for loading from file
bool add_new_page (void*, bool);//create a new supplementary page for stack growth
bool load_page (struct page*);//load a page with address pg
bool load_page_file (struct page*);//load a page with address pg from the file
bool load_page_swap (struct page*);//load a page with address pg from swap memory
bool load_page_none (struct page*);
bool add_mmap_to_page (uint8_t* vaddr_, void* save_addr_, uint32_t read_bytes_, uint32_t zero_bytes_, off_t ofs);//create a supplementary page for mmap

#endif
