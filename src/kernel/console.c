#include <wumoe/console.h>
#include <wumoe/io.h>
#include <wumoe/string.h>
#include <wumoe/types.h>

#define CRT_ADDR_REG 0x3D4
#define CRT_DATA_REG 0x3D5

#define CRT_START_ADDR_H 0xC
#define CRT_START_ADDR_L 0xD
#define CRT_CURSOR_H 0xE
#define CRT_CURSOR_L 0xF

#define MEM_BASE 0xB8000
#define MEM_SIZE 0x4000
#define MEM_END (MEM_BASE + MEM_SIZE)
#define WIDTH 80
#define HEIGHT 25
#define ROW_SIZE (WIDTH * 2)
#define SCR_SIZE (ROW_SIZE * HEIGHT)

#define ASCII_NUL 0x00
#define ASCII_ENQ 0x05
#define ASCII_BEL 0x07
#define ASCII_BS 0x08
#define ASCII_HT 0x09
#define ASCII_LF 0x0A
#define ASCII_VT 0x0B
#define ASCII_FF 0x0C
#define ASCII_CR 0x0D
#define ASCII_DEL 0x7F

static u32 screen;
static u32 pos;
static u32 x, y;

static u8 attr = 7;
static u16 erase = 0x0720;

static void get_screen() {
	outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    screen = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    screen |= inb(CRT_DATA_REG);
	screen <<= 1;
    screen += MEM_BASE;
}

static void set_screen() {
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

static void get_cursor() {
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    pos = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    pos |= inb(CRT_DATA_REG);
    get_screen();
    pos <<= 1;
    pos += MEM_BASE;
    u32 delta = (pos - screen) >> 1;
    x = delta % WIDTH;
    y = delta / WIDTH;
}

static void set_cursor() {
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 1) & 0xff);
}

void console_clear() {
	screen = MEM_BASE;
    pos = MEM_BASE;
    x = y = 0;
    set_cursor();
    set_screen();
    u16 *ptr = (u16 *)MEM_BASE;
    while (ptr < (u16 *)MEM_END)
        *ptr++ = erase;
}

static void scroll_up() {
    if (screen + SCR_SIZE + ROW_SIZE >= MEM_END) {
        memcpy((void *) MEM_BASE, (void *) screen, SCR_SIZE);
        pos -= (screen - MEM_BASE);
        screen = MEM_BASE;
    }
    u32 *ptr = (u32 *)(screen + SCR_SIZE);
    for (size_t i = 0; i < WIDTH; i++)
        *ptr++ = erase;
    screen += ROW_SIZE;
    pos += ROW_SIZE;
    set_screen();
}

static void command_lf() {
    if (y + 1 < HEIGHT) {
        ++y;
        pos += ROW_SIZE;
        return;
    }
    scroll_up();
}

static void command_cr() {
    pos -= (x << 1);
    x = 0;
}

static void command_bs() {
    if (x) {
        --x;
        pos -= 2;
        *(u16 *)pos = erase;
    }
}

static void command_del() {
    *(u16 *)pos = erase;
}

extern void start_beep();

void console_write(char *buf, u32 count) {
    char ch;
    while (count--) {
        ch = *buf++;
        switch (ch) {
        case ASCII_NUL:
            break;
        case ASCII_BEL:
            start_beep();
            break;
        case ASCII_BS:
            command_bs();
            break;
        case ASCII_HT:
            break;
        case ASCII_LF:
            command_lf();
            command_cr();
            break;
        case ASCII_VT:
            break;
        case ASCII_FF:
            command_lf();
            break;
        case ASCII_CR:
            command_cr();
            break;
        case ASCII_DEL:
            command_del();
            break;
        default:
            if (x >= WIDTH) {
                x -= WIDTH;
                pos -= ROW_SIZE;
                command_lf();
            }
            *((char *) pos) = ch;
            ++pos;
            *((char *) pos) = attr;
            ++pos;
            ++x;
            break;
        }
    }
    set_cursor();
}

void console_init() {
	console_clear();
}
