#include <wumoe/kernel.h>
#include <wumoe/types.h>
#include <wumoe/console.h>
#include <wumoe/print.h>
#include <wumoe/stdlib.h>
#include <wumoe/interrupt.h>
#include <wumoe/task.h>
#include <wumoe/keyboard.h>
#include <wumoe/operation.h>

extern void interrupt_init();
extern void clock_init();
extern void rtc_init();
extern void time_init();
extern void task_init();
extern void memory_map_init();
extern void mapping_init();
extern void keyboard_init();

void kernel() {
	set_interrupt_state(true);
	while(true);
}

int backups_state;

void console() {
	set_interrupt_state(true);
	char *input;
	while (true) {
		if (backups_state != key_get_result_state()) {
			char *ch = key_get_result();
			operation(ch);
			backups_state = key_get_result_state();
			print("\n[Console] >>> ");
		} else {
			set_interrupt_state(false);
			task_yield();
		}
	}
}

void kernel_init() {
	memory_map_init();
    mapping_init();
    interrupt_init();
    clock_init();
	time_init();
	rtc_init();
    task_init();
	keyboard_init();
    set_interrupt_state(true);

	console_clear();
	print("Walcome to WuMoe OS.\n");
	print("\n[Console] >>> ");
	backups_state = key_get_result_state();
	task_create(kernel, "kernel", 5, KERNEL_USER);
	task_create(console, "console", 5, KERNEL_USER);
	hang();
}
