#include <wumoe/kernel.h>
#include <wumoe/memory.h>
#include <wumoe/types.h>
#include <wumoe/print.h>
#include <wumoe/assert.h>
#include <wumoe/stdlib.h>
#include <wumoe/string.h>
#include <wumoe/bitmap.h>

#define ZONE_VALID 1
#define ZONE_RESERVED 2

#define IDX(addr) ((u32)addr >> 12)
#define DIDX(addr) (((u32)addr >> 22) & 0x3ff)
#define TIDX(addr) (((u32)addr >> 12) & 0x3ff)
#define PAGE(idx) ((u32)idx << 12)
#define ASSERT_PAGE(addr) assert((addr & 0xfff) == 0)

static u32 KERNEL_PAGE_TABLE[] = {
    0x2000,
    0x3000,
};

#define KERNEL_MAP_BITS 0x4000

#define KERNEL_MEMORY_SIZE (0x100000 * sizeof(KERNEL_PAGE_TABLE))

bitmap_t kernel_map;

typedef struct ards_t {
    u64 base;
    u64 size;
    u32 type;
} _packed ards_t;

u32 memory_base = 0;
u32 memory_size = 0;
u32 total_pages = 0;
u32 free_pages = 0;

#define used_pages (total_pages - free_pages)

void memory_init(u32 magic, u32 addr) {
    u32 count;
    ards_t *ptr;

    if (magic == WUMOE_MAGIC) {
        count = *(u32 *)addr;
        ptr = (ards_t *)(addr + 4);
        for (size_t i = 0; i < count; ++i, ++ptr) {
            printf("Memory base 0x%p size 0x%p type %d\n",
                 (u32)ptr->base, (u32)ptr->size, (u32)ptr->type);
            if (ptr->type == ZONE_VALID && ptr->size > memory_size) {
                memory_base = (u32)ptr->base;
                memory_size = (u32)ptr->size;
            }
        }
    } else
        printf("Memory init magic unknown 0x%p\n", magic);
    printf("ARDS count %d\n", count);
    printf("Memory base 0x%p\n", (u32)memory_base);
    printf("Memory size 0x%p\n", (u32)memory_size);
    assert(memory_base == MEMORY_BASE);
    assert((memory_size & 0xfff) == 0);
    total_pages = IDX(memory_size) + IDX(MEMORY_BASE);
    free_pages = IDX(memory_size);
    printf("Total pages %d\n", total_pages);
    printf("Free pages %d\n", free_pages);
    if (memory_size < KERNEL_MEMORY_SIZE) {
        printf("System memory is %dM too small, at least %dM needed\n",
              memory_size / MEMORY_BASE, KERNEL_MEMORY_SIZE / MEMORY_BASE);
    }
}

static u32 start_page = 0;
static u8 *memory_map;
static u32 memory_map_pages;

void memory_map_init() {
    memory_map = (u8 *)memory_base;
    memory_map_pages = div_round_up(total_pages, PAGE_SIZE);
    printf("Memory map page count %d\n", memory_map_pages);
    free_pages -= memory_map_pages;
    memset((void *)memory_map, 0, memory_map_pages * PAGE_SIZE);
    start_page = IDX(MEMORY_BASE) + memory_map_pages;
    for (size_t i = 0; i < start_page; ++i)
        memory_map[i] = 1;
    printf("Total pages %d free pages %d\n", total_pages, free_pages);
    u32 length = (IDX(KERNEL_MEMORY_SIZE) - IDX(MEMORY_BASE)) / 8;
    bitmap_init(&kernel_map, (u8 *)KERNEL_MAP_BITS, length, IDX(MEMORY_BASE));
    bitmap_scan(&kernel_map, memory_map_pages);
}

static u32 get_page() {
    for (size_t i = start_page; i < total_pages; ++i) {
        if (!memory_map[i]) {
            memory_map[i] = 1;
            --free_pages;
            assert(free_pages >= 0);
            u32 page = ((u32)i) << 12;
            return page;
        }
    }
    print("Out of Memory!!!");
}

static void put_page(u32 addr) {
    ASSERT_PAGE(addr);
    u32 idx = IDX(addr);
    assert(idx >= start_page && idx < total_pages);
    assert(memory_map[idx] >= 1);
    --memory_map[idx];
    if (!memory_map[idx])
        ++free_pages;
    assert(free_pages > 0 && free_pages < total_pages);
}


u32 get_cr3() {
    asm volatile("movl %cr3, %eax\n");
}

void set_cr3(u32 pde) {
    ASSERT_PAGE(pde);
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

static _inline void enable_page() {
    asm volatile(
        "movl %cr0, %eax\n"
        "orl $0x80000000, %eax\n"
        "movl %eax, %cr0\n");
}

static void entry_init(page_entry_t *entry, u32 index) {
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

void mapping_init() {
    page_entry_t *pde = (page_entry_t *)KERNEL_PAGE_DIR;
    memset(pde, 0, PAGE_SIZE);
    idx_t index = 0;
    for (idx_t didx = 0; didx < (sizeof(KERNEL_PAGE_TABLE) / 4); ++didx) {
        page_entry_t *pte = (page_entry_t *)KERNEL_PAGE_TABLE[didx];
        memset(pte, 0, PAGE_SIZE);
        page_entry_t *dentry = &pde[didx];
        entry_init(dentry, IDX((u32)pte));
        for (idx_t tidx = 0; tidx < 1024; ++tidx, ++index) {
            if (index == 0)
                continue;
            page_entry_t *tentry = &pte[tidx];
            entry_init(tentry, index);
            memory_map[index] = 1;
        }
    }
    page_entry_t *entry = &pde[1023];
    entry_init(entry, IDX(KERNEL_PAGE_DIR));
    set_cr3((u32)pde);
    enable_page();
}

static page_entry_t *get_pde() {
    return (page_entry_t *)(0xfffff000);
}

static page_entry_t *get_pte(u32 vaddr) {
    return (page_entry_t *)(0xffc00000 | (DIDX(vaddr) << 12));
}

static void flush_tlb(u32 vaddr) {
    asm volatile("invlpg (%0)" ::"r"(vaddr)
                 : "memory");
}

static u32 scan_page(bitmap_t *map, u32 count) {
    assert(count > 0);
    int32 index = bitmap_scan(map, count);
    if (index == EOF)
        print("Scan page fail!!!");
    u32 addr = PAGE(index);
    return addr;
}

static void reset_page(bitmap_t *map, u32 addr, u32 count) {
    ASSERT_PAGE(addr);
    assert(count > 0);
    u32 index = IDX(addr);
    for (size_t i = 0; i < count; ++i) {
        assert(bitmap_test(map, index + i));
        bitmap_set(map, index + i, 0);
    }
}

u32 alloc_kpage(u32 count) {
    assert(count > 0);
    u32 vaddr = scan_page(&kernel_map, count);
    return vaddr;
}

void free_kpage(u32 vaddr, u32 count) {
    ASSERT_PAGE(vaddr);
    assert(count > 0);
    reset_page(&kernel_map, vaddr, count);
}
