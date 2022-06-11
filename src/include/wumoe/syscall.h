#ifndef WUMOE_SYSCALL_H
#define WUMOE_SYSCALL_H

#include <wumoe/types.h>

typedef enum syscall_t {
    SYS_NR_TEST,
    SYS_NR_SLEEP,
    SYS_NR_YIELD,
} syscall_t;

void yield();
void sleep(u32 ms);

#endif
