#include <wumoe/kernel.h>
#include <wumoe/operation.h>
#include <wumoe/string.h>
#include <wumoe/print.h>
#include <wumoe/types.h>
#include <wumoe/time.h>
#include <wumoe/task.h>
#include <wumoe/memory.h>
#include <wumoe/console.h>

bool operation_eq(const char *ch, const char *op) {
    while(*op != EOS)
        if (*ch++ != *op++)
            return false;
    return true;
}

char *task_state_to_string(task_state_t state) {
    if (state == TASK_INIT)
        return "init";
    if (state == TASK_RUNNING)
        return "running";
    if (state == TASK_READY)
        return "ready";
    if (state == TASK_BLOCKED)
        return "blocked";
    if (state == TASK_SLEEPING)
        return "sleeping";
    if (state == TASK_WAITING)
        return "waiting";
    if (state == TASK_DIED)
        return "died";
}

char *task_uid_to_string(int uid) {
    if (uid == KERNEL_USER)
        return "KERNEL_USER";
    return "NORMAL_USER";
}

void operation(char *op) {
    if (operation_eq(op, "system")) {
        printf("%s %s\n", OS_NAME, OS_VERSION);
        print("Anther: RMOLive (rmolives@gmail.com)\n");
        printf("Memory Size: %d MiB\n", (memory_size - memory_base) / (1024 * 1024));
        size_t task_size = 0;
        task_t **table = get_task_table();
        for (size_t i = 0; i < NR_TASKS; ++i) {
            if (table[i] == NULL)
                continue;
            ++task_size;
        }
        printf("Task Size: %d", task_size);
    } else if (operation_eq(op, "help")) {
        print("1. help : get help.\n");
        print("2. echo [value] : print value.\n");
        print("3. time : get time.\n");
        print("4. task : get task table.\n");
        print("5. system : get system information.\n");
        print("6. memory : get memory information.\n");
        print("7. clear : console clear.")
    } else if (operation_eq(op, "echo"))
        printf("%s", strchr(op, ' ') + 1);
    else if (operation_eq(op, "time")) {
        tm time;
        time_read(&time);
        printf("20%d-%d-%d %d:%d:%d", time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    } else if (operation_eq(op, "task")) {
        task_t **table = get_task_table();
        print("----------------------");
        for (size_t i = 0; i < NR_TASKS; ++i) {
            task_t *ptr = table[i];
            if (ptr == NULL)
                continue;
            printf("\nname: %s", ptr->name);
            printf("\nuid: %s", task_uid_to_string(ptr->uid));
            printf("\nstate: %s", task_state_to_string(ptr->state));
            print("\n----------------------");
        }
    } else if (operation_eq(op, "memory")) {
        printf("Memory Size: %d MiB", (memory_size - memory_base) / (1024 * 1024));
    } else if (operation_eq(op, "clear"))
        console_clear();
    else
        printf("Undefined operation %s!!!", op);
}


