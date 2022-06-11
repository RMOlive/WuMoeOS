#include <wumoe/kernel.h>
#include <wumoe/task.h>
#include <wumoe/types.h>
#include <wumoe/print.h>
#include <wumoe/memory.h>
#include <wumoe/assert.h>
#include <wumoe/interrupt.h>
#include <wumoe/string.h>
#include <wumoe/bitmap.h>
#include <wumoe/syscall.h>
#include <wumoe/list.h>

extern u32 volatile jiffies;
extern u32 jiffy;
extern bitmap_t kernel_map;
extern void task_switch(task_t *next);

static task_t *task_table[NR_TASKS];
static list_t block_list;
static list_t sleep_list;

static task_t *idle_task;

task_t **get_task_table() {
    return task_table;
}

static task_t *get_free_task() {
    for (size_t i = 0; i < NR_TASKS; ++i) {
        if (task_table[i] == NULL) {
            task_table[i] = (task_t *) alloc_kpage(1);
            return task_table[i];
        }
    }
    print("No more tasks");
}

static task_t *task_search(task_state_t state) {
    assert(!get_interrupt_state());
    task_t *task = NULL;
    task_t *current = running_task();
    for (size_t i = 0; i < NR_TASKS; ++i) {
        task_t *ptr = task_table[i];
        if (ptr == NULL)
            continue;
        if (ptr->state != state)
            continue;
        if (current == ptr)
            continue;
        if (task == NULL || task->ticks < ptr->ticks || ptr->jiffies < task->jiffies)
            task = ptr;
    }
    if (task == NULL && state == TASK_READY)
        task = idle_task;
    return task;
}

void task_yield() {
    schedule();
}

void task_block(task_t *task, list_t *blist, task_state_t state) {
    assert(!get_interrupt_state());
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);
    if (blist == NULL)
        blist = &block_list;
    list_push(blist, &task->node);
    assert(state != TASK_READY && state != TASK_RUNNING);
    task->state = state;
    task_t *current = running_task();
    if (current == task)
        schedule();
}

void task_unblock(task_t *task) {
    assert(!get_interrupt_state());
    list_remove(&task->node);
    assert(task->node.next == NULL);
    assert(task->node.prev == NULL);
    task->state = TASK_READY;
}

void task_sleep(u32 ms) {
    assert(!get_interrupt_state());
    u32 ticks = ms / jiffy;
    ticks = ticks > 0 ? ticks : 1;
    task_t *current = running_task();
    current->ticks = jiffies + ticks;
    list_t *list = &sleep_list;
    list_node_t *anchor = &list->tail;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail; ptr = ptr->next) {
        task_t *task = element_entry(task_t, node, ptr);
        if (task->ticks > current->ticks) {
            anchor = ptr;
            break;
        }
    }
    assert(current->node.next == NULL);
    assert(current->node.prev == NULL);
    list_insert_before(anchor, &current->node);
    current->state = TASK_SLEEPING;
    schedule();
}

void task_wakeup() {
    assert(!get_interrupt_state());
    list_t *list = &sleep_list;
    for (list_node_t *ptr = list->head.next; ptr != &list->tail;) {
        task_t *task = element_entry(task_t, node, ptr);
        if (task->ticks > jiffies)
            break;
        ptr = ptr->next;
        task->ticks = 0;
        task_unblock(task);
    }
}

task_t *running_task() {
    asm volatile(
        "movl %esp, %eax\n"
        "andl $0xfffff000, %eax\n");
}

void schedule() {
    assert(!get_interrupt_state());
    task_t *current = running_task();
    task_t *next = task_search(TASK_READY);
    assert(next != NULL);
    assert(next->magic == WUMOE_MAGIC);
    if (current->state == TASK_RUNNING)
        current->state = TASK_READY;
    if (!current->ticks)
        current->ticks = current->priority;
    next->state = TASK_RUNNING;
    if (next == current)
        return;
    task_switch(next);
}

task_t *task_create(target_t target, const char *name, u32 priority, u32 uid) {
    task_t *task = get_free_task();
    memset(task, 0, PAGE_SIZE);
    u32 stack = (u32)task + PAGE_SIZE;
    stack -= sizeof(task_frame_t);
    task_frame_t *frame = (task_frame_t *)stack;
    frame->ebx = 0x11111111;
    frame->esi = 0x22222222;
    frame->edi = 0x33333333;
    frame->ebp = 0x44444444;
    frame->eip = (void *)target;
    strcpy((char *)task->name, name);
    task->stack = (u32 *)stack;
    task->priority = priority;
    task->ticks = task->priority;
    task->jiffies = 0;
    task->state = TASK_READY;
    task->uid = uid;
    task->vmap = &kernel_map;
    task->pde = KERNEL_PAGE_DIR;
    task->magic = WUMOE_MAGIC;
    return task;
}

static void task_setup() {
    task_t *task = running_task();
    task->magic = WUMOE_MAGIC;
    task->ticks = 1;
    memset(task_table, 0, sizeof(task_table));
}

void idle_thread() {
    set_interrupt_state(true);
    while (true) {
        asm volatile(
            "sti\n"
            "hlt\n"
        );
        yield();
    }
}

void task_init() {
    list_init(&block_list);
    list_init(&sleep_list);
    task_setup();
    idle_task = task_create(idle_thread, "idle", 1, KERNEL_USER);
}
