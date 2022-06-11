#include <wumoe/keyboard.h>
#include <wumoe/keymap.h>
#include <wumoe/us_std.h>
#include <wumoe/types.h>
#include <wumoe/interrupt.h>
#include <wumoe/io.h>
#include <wumoe/string.h>
#include <wumoe/print.h>

#define PORT 0X61

#define KEYBOARD_DATA       0x60

#define KEYBOARD_COMMAND    0x64
#define KEYBOARD_STATUS     0x64
#define KEYBOARD_ACK        0xFA

#define KEYBOARD_OUT_FULL   0x01
#define KEYBOARD_IN_FULL    0x02
#define LED_CODE            0xED
#define MAX_KEYBOARD_ACK_RETRIES    0x1000
#define MAX_KEYBOARD_BUSY_RETRIES   0x1000
#define KEY_BIT             0x80
#define KEY                 0x7f

#define SCROLL_UP       0
#define SCROLL_DOWN     1

#define SCROLL_LOCK	    0x01
#define NUM_LOCK	    0x02
#define CAPS_LOCK	    0x04
#define DEFAULT_LOCK    0x02

#define ESC_SCAN	        0x01
#define SLASH_SCAN	        0x35
#define RSHIFT_SCAN	        0x36
#define HOME_SCAN	        0x47
#define INS_SCAN	        0x52
#define DEL_SCAN	        0x53

bool esc = false;
bool alt_left = false;
bool alt_right = false;
bool alt = false;
bool ctrl_left = false;
bool ctrl_right = false;
bool ctrl = false;
bool shift_left = false;
bool shift_right = false;
bool shift = false;
bool num_down = false;
bool caps_down = false;
bool scroll_down = false;
static u8 lock = DEFAULT_LOCK;

static char numpad_map[] = {'H', 'Y', 'A', 'B', 'D', 'C', 'V', 'U', 'G', 'S', 'T', '@'};

u8 input_buff[KEYBOARD_BUFF_BYTES];
char input_result[KEYBOARD_IN_BYTES];
char input_result_buff[KEYBOARD_IN_BYTES];
int input_count;
int result_buff_count;
int result_state;
u8 *input_free = input_buff;
char *result_free = input_result_buff;
u8 *input_todo = input_buff;

#define map_key0(scan_code) ((u16) keymap[scan_code * MAP_COLS])

char *key_get_result() {
    return input_result;
}

int key_get_result_state() {
    return result_state;
}

u32 map_key(u8 scan_code) {
    if(scan_code == SLASH_SCAN && esc)
        return '/';
    u32 *keys_row = &keymap[scan_code * MAP_COLS];
    u8 lock = lock;
    bool caps = shift;
    if((lock & NUM_LOCK) != 0 && HOME_SCAN <= scan_code && scan_code <= DEL_SCAN)
        caps = !caps;
    if((lock & CAPS_LOCK) && (keys_row[0] & HASCAPS))
        caps = !caps;
    int col = 0;
    if(alt) {
        col = 2;
        if(ctrl || alt_right)
            col = 3;
        if(caps)
            col = 4;
    } else {
        if(caps)
            col = 1;
        if(ctrl)
            col = 5;
    }
    return (keys_row[col] & ~HASCAPS);
}

int keyoard_wait() {
    int retries = 10;
    u8 status;
    while (retries-- > 0 && ((status == inb(KEYBOARD_STATUS) & (KEYBOARD_IN_FULL | KEYBOARD_OUT_FULL)) != 0)) {
        if ((status & KEYBOARD_OUT_FULL) != 0)
            (void) inb(KEYBOARD_DATA);
    }
    return retries;
}

int keyboard_ack() {
    int retries = 10;
    while (retries-- > 0 && (inb(KEYBOARD_DATA) & KEYBOARD_ACK) != 0);
    return retries;
}

void setting_led() {
    keyoard_wait();
    outb(KEYBOARD_DATA, LED_CODE);
    keyboard_ack();
    keyoard_wait();
    outb(KEYBOARD_DATA, lock);
    keyboard_ack();
}

u32 key_make_break(u8 scan_code) {
    bool is_make = (scan_code & KEY_BIT) == 0;
    u32 ch = map_key((scan_code &= KEY));
    bool escape = esc;
    esc = false;

    switch (ch) {
        case CTRL:
            if(escape)
                ctrl_right = is_make;
            else
                ctrl_left = is_make;
            ctrl = ctrl_left | ctrl_right;
            break;
        case ALT:
            if(escape)
                alt_right = is_make;
            else
                alt_left = is_make;
            alt = alt_left | alt_right;
            break;
        case SHIFT:
            if(scan_code == RSHIFT_SCAN)
                shift_right = is_make;
            else
                shift_left = is_make;
            shift = shift_left | shift_right;
            break;
        case CALOCK:
            if(is_make) {
                lock ^= CAPS_LOCK;
                setting_led();
            }
            break;
        case NLOCK:
            if(is_make) {
                lock ^= NUM_LOCK;
                setting_led();
            }
            break;
        case SLOCK:
            if(is_make) {
                lock ^= SCROLL_LOCK;
                setting_led();
            }
            break;
        case EXTKEY:
            esc = true;
            break;
        default:
            if(is_make)
                return ch;
    }
    return -1;
}

u8 scan_key() {
    u8 scan_code = inb(KEYBOARD_DATA);
    u8 val = inb(PORT);
    outb(PORT, val | KEY_BIT);
    outb(PORT, val);
    return scan_code;
}

void keyboard_handler(int vector) {
    send_eoi(vector);
    u8 scan_code = scan_key();
    if (scan_code == 28) {
        memcpy(input_result, input_result_buff, result_buff_count);
        input_result[result_buff_count] = '\0';
        result_free = input_result_buff;
        result_buff_count = 0;
        input_free = input_buff;
        input_count = 0;
        ++result_state;
        print("\n");
        return;
    }

    u32 ch = key_make_break(scan_code);
        if (scan_code == 14) {
        --result_buff_count;
        *result_free--;
        printf("%c", ch);
        return;
    }
    if(ch != -1) {
        printf("%c", ch);
        if (result_buff_count < KEYBOARD_BUFF_BYTES) {
            *result_free++ = ch;
            ++result_buff_count;
            if (result_buff_count == KEYBOARD_BUFF_BYTES) {
                result_free = input_result_buff;
                result_buff_count = 0;
            }
        }
    }
    if (input_count < KEYBOARD_BUFF_BYTES) {
        *input_free++ = scan_code;
        ++input_count;
        if (input_count == KEYBOARD_BUFF_BYTES) {
            input_free = input_buff;
            input_count = 0;
        }
    }
}

void keyboard_init() {
    print("keyboard init...");
    result_buff_count = 0;
    input_count = 0;
    result_state = 0;
    setting_led();
    set_interrupt_handler(IRQ_KEYBOARD, keyboard_handler);
    set_interrupt_mask(IRQ_KEYBOARD, true);
}
