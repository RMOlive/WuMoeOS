#ifndef WUMOE_MEMORY_H
#define WUMOE_MEMORY_H

#include <wumoe/types.h>

#define PAGE_SIZE 0x1000
#define MEMORY_BASE 0x100000

#define KERNEL_PAGE_DIR 0x1000

extern u32 memory_base;
extern u32 memory_size;
extern u32 total_pages;
extern u32 free_pages;

typedef struct page_entry_t {
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 pwt : 1;
    u8 pcd : 1;
    u8 accessed : 1;
    u8 dirty : 1;
    u8 pat : 1;
    u8 global : 1;
    u8 ignored : 3;
    u32 index : 20;
} _packed page_entry_t;

u32 get_cr3();
void set_cr3(u32 pde);

u32 alloc_kpage(u32 count);
void free_kpage(u32 vaddr, u32 count);

#endif
