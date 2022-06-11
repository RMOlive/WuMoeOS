#ifndef WUMOE_KEYBOARD_H
#define WUMOE_KEYBOARD_H

#include <wumoe/types.h>

#define KEYBOARD_IN_BYTES 32
#define KEYBOARD_BUFF_BYTES (KEYBOARD_IN_BYTES * 2)

char *key_get_result();

int key_get_result_state();

#endif
