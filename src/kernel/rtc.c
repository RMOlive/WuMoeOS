#include <wumoe/types.h>
#include <wumoe/interrupt.h>
#include <wumoe/io.h>
#include <wumoe/time.h>
#include <wumoe/assert.h>
#include <wumoe/stdlib.h>
#include <wumoe/print.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

u8 cmos_read(u8 addr) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    return inb(CMOS_DATA);
}

void cmos_write(u8 addr, u8 value) {
    outb(CMOS_ADDR, CMOS_NMI | addr);
    outb(CMOS_DATA, value);
}

void set_alarm(u32 secs) {
    tm time;
    time_read(&time);

    u8 sec = secs % 60;
    secs /= 60;
    u8 min = secs % 60;
    secs /= 60;
    u32 hour = secs;

    time.tm_sec += sec;
    if (time.tm_sec >= 60) {
        time.tm_sec %= 60;
        time.tm_min += 1;
    }

    time.tm_min += min;
    if (time.tm_min >= 60) {
        time.tm_min %= 60;
        time.tm_hour += 1;
    }

    time.tm_hour += hour;
    if (time.tm_hour >= 24)
        time.tm_hour %= 24;

    cmos_write(CMOS_HOUR, bin_to_bcd(time.tm_hour));
    cmos_write(CMOS_MINUTE, bin_to_bcd(time.tm_min));
    cmos_write(CMOS_SECOND, bin_to_bcd(time.tm_sec));
}

void rtc_handler(int vector) {
    assert(vector == 0x28);
    send_eoi(vector);
    cmos_read(CMOS_C);
    set_alarm(1);
}

void rtc_init() {
    u8 prev;
    cmos_write(CMOS_B, 0b00100010);
    cmos_read(CMOS_C);
    set_alarm(2);
    outb(CMOS_A, (inb(CMOS_A) & 0xf) | 0b1110);
    set_interrupt_handler(IRQ_RTC, rtc_handler);
    set_interrupt_mask(IRQ_RTC, true);
    set_interrupt_mask(IRQ_CASCADE, true);
}
