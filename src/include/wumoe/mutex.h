#ifndef WUMOE_MUTEX_H
#define WUMOE_MUTEX_H

#include <wumoe/types.h>
#include <wumoe/list.h>

typedef struct mutex_t {
    bool value;
    list_t waiters;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

typedef struct spinlock_t {
    struct task_t *holder;
    mutex_t mutex;
    u32 repeat;
} spinlock_t;

void spin_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif
