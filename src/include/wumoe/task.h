#ifndef WUMOE_TASK_H
#define WUMOE_TASK_H

#include <wumoe/types.h>
#include <wumoe/list.h>

#define KERNEL_USER 0
#define NORMAL_USER 1

#define TASK_NAME_LEN 16

#define NR_TASKS 64

typedef void target_t();

typedef enum task_state_t {
    TASK_INIT,
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_WAITING,
    TASK_DIED,
} task_state_t;

typedef struct task_t {
    u32 *stack;
    list_node_t node;
    task_state_t state;
    u32 priority;
    u32 ticks;
    u32 jiffies;
    char name[TASK_NAME_LEN];
    u32 uid;
    u32 pde;
    struct bitmap_t *vmap;
    u32 magic;
} task_t;

typedef struct task_frame_t {
    u32 edi;
    u32 esi;
    u32 ebx;
    u32 ebp;
    void (*eip)(void);
} task_frame_t;

task_t **get_task_table();

task_t *running_task();
void schedule();

void task_yield();
void task_block(task_t *task, list_t *blist, task_state_t state);
void task_unblock(task_t *task);

void task_sleep(u32 ms);
void task_wakeup();

task_t *task_create(target_t target, const char *name, u32 priority, u32 uid);

#endif
