#include <wumoe/kernel.h>
#include <wumoe/mutex.h>
#include <wumoe/task.h>
#include <wumoe/interrupt.h>
#include <wumoe/assert.h>

void mutex_init(mutex_t *mutex) {
    mutex->value = false;
    list_init(&mutex->waiters);
}

void mutex_lock(mutex_t *mutex) {
    bool intr = interrupt_disable();
    task_t *current = running_task();
    while (mutex->value == true)
        task_block(current, &mutex->waiters, TASK_BLOCKED);
    assert(mutex->value == false);
    ++mutex->value;
    assert(mutex->value == true);
    set_interrupt_state(intr);
}

void mutex_unlock(mutex_t *mutex) {
    bool intr = interrupt_disable();
    assert(mutex->value == true);
    --mutex->value;
    assert(mutex->value == false);
    if (!list_empty(&mutex->waiters)) {
        task_t *task = element_entry(task_t, node, mutex->waiters.tail.prev);
        assert(task->magic == WUMOE_MAGIC);
        task_unblock(task);
        task_yield();
    }
    set_interrupt_state(intr);
}

void spin_init(spinlock_t *lock) {
    lock->holder = NULL;
    lock->repeat = 0;
    mutex_init(&lock->mutex);
}

void spin_lock(spinlock_t *lock) {
    task_t *current = running_task();
    if (lock->holder != current) {
        mutex_lock(&lock->mutex);
        lock->holder = current;
        assert(lock->repeat == 0);
        lock->repeat = 1;
    } else
        ++lock->repeat;
}

void spin_unlock(spinlock_t *lock) {
    task_t *current = running_task();
    assert(lock->holder == current);
    if (lock->repeat > 1) {
        lock->repeat--;
        return;
    }
    assert(lock->repeat == 1);
    lock->holder = NULL;
    lock->repeat = 0;
    mutex_unlock(&lock->mutex);
}
