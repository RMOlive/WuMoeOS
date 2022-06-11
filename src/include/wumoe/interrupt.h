#ifndef WUMOE_INTERRUPT_H
#define WUMOE_INTERRUPT_H

#include <wumoe/types.h>

#define IDT_SIZE 256

#define IRQ_CLOCK 0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2
#define IRQ_SERIAL_2 3
#define IRQ_SERIAL_1 4
#define IRQ_PARALLEL_2 5
#define IRQ_FLOPPY 6
#define IRQ_PARALLEL_1 7
#define IRQ_RTC 8
#define IRQ_REDIRECT 9
#define IRQ_MOUSE 12
#define IRQ_MATH 13
#define IRQ_HARDDISK 14
#define IRQ_HARDDISK2 15

#define IRQ_MASTER_NR 0x20
#define IRQ_SLAVE_NR 0x28

typedef struct gate_t {
    u16 offset0;
    u16 selector;
    u8 reserved;
    u8 type : 4;
    u8 segment : 1;
    u8 DPL : 2;
    u8 present : 1;
    u16 offset1;
} _packed gate_t;

typedef void *handler_t;

void send_eoi(int vector);

void set_interrupt_handler(u32 irq, handler_t handler);
void set_interrupt_mask(u32 irq, bool enable);

bool interrupt_disable();
bool get_interrupt_state();
void set_interrupt_state(bool state);

#endif
