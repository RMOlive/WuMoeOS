#include <wumoe/interrupt.h>
#include <wumoe/global.h>
#include <wumoe/print.h>
#include <wumoe/stdlib.h>
#include <wumoe/io.h>
#include <wumoe/assert.h>

#define ENTRY_SIZE 0x30

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1
#define PIC_EOI 0x20

gate_t idt[IDT_SIZE];
pointer_t idt_ptr;

handler_t handler_table[IDT_SIZE];
extern handler_t handler_entry_table[ENTRY_SIZE];
extern void syscall_handler();

static char *messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

void send_eoi(int vector) {
    if (vector >= 0x20 && vector < 0x28)
        outb(PIC_M_CTRL, PIC_EOI);
    if (vector >= 0x28 && vector < 0x30) {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}

void set_interrupt_handler(u32 irq, handler_t handler) {
    assert(irq >= 0 && irq < 16);
    handler_table[IRQ_MASTER_NR + irq] = handler;
}

void set_interrupt_mask(u32 irq, bool enable) {
    assert(irq >= 0 && irq < 16);
    u16 port;
    if (irq < 8)
        port = PIC_M_DATA;
    else {
        port = PIC_S_DATA;
        irq -= 8;
    }
    if (enable)
        outb(port, inb(port) & ~(1 << irq));
    else
        outb(port, inb(port) | (1 << irq));
}

bool interrupt_disable() {
    asm volatile(
        "pushfl\n"
        "cli\n"
        "popl %eax\n"
        "shrl $9, %eax\n"
        "andl $1, %eax\n"
    );
}

bool get_interrupt_state() {
    asm volatile(
        "pushfl\n"
        "popl %eax\n"
        "shrl $9, %eax\n"
        "andl $1, %eax\n"
    );
}

void set_interrupt_state(bool state) {
    if (state)
        asm volatile("sti\n");
    else
        asm volatile("cli\n");
}

void default_handler(int vector) {
    send_eoi(vector);
}

void exception_handler(
    int vector,
    u32 edi, u32 esi, u32 ebp, u32 esp,
    u32 ebx, u32 edx, u32 ecx, u32 eax,
    u32 gs, u32 fs, u32 es, u32 ds,
    u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags) {
    char *message = NULL;
    if (vector < 22)
        message = messages[vector];
    else
        message = messages[15];
    printf("\nEXCEPTION : %s \n", message);
    printf("   VECTOR : 0x%02X\n", vector);
    printf("    ERROR : 0x%08X\n", error);
    printf("   EFLAGS : 0x%08X\n", eflags);
    printf("       CS : 0x%02X\n", cs);
    printf("      EIP : 0x%08X\n", eip);
    printf("      ESP : 0x%08X\n", esp);
    hang();
}

void pic_init() {
    outb(PIC_M_CTRL, 0b00010001);
    outb(PIC_M_DATA, 0x20);
    outb(PIC_M_DATA, 0b00000100);
    outb(PIC_M_DATA, 0b00000001);
    outb(PIC_S_CTRL, 0b00010001);
    outb(PIC_S_DATA, 0x28);
    outb(PIC_S_DATA, 2);
    outb(PIC_S_DATA, 0b00000001);
    outb(PIC_M_DATA, 0b11111111);
    outb(PIC_S_DATA, 0b11111111);
}

void idt_init() {
    for (size_t i = 0; i < ENTRY_SIZE; i++) {
        gate_t *gate = &idt[i];
        handler_t handler = handler_entry_table[i];
        gate->offset0 = (u32)handler & 0xffff;
        gate->offset1 = ((u32)handler >> 16) & 0xffff;
        gate->selector = 1 << 3;
        gate->reserved = 0;
        gate->type = 0b1110;
        gate->segment = 0;
        gate->DPL = 0;
        gate->present = 1;
    }
    for (size_t i = 0; i < 0x20; ++i)
        handler_table[i] = exception_handler;

    for (size_t i = 0x20; i < ENTRY_SIZE; ++i)
        handler_table[i] = default_handler;
    idt_ptr.base = (u32)idt;
    idt_ptr.limit = sizeof(idt) - 1;
    asm volatile("lidt idt_ptr\n");
}

void interrupt_init() {
    pic_init();
    idt_init();
}
