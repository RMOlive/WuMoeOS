#include <wumoe/vsprintf.h>
#include <wumoe/stdarg.h>
#include <wumoe/print.h>
#include <wumoe/string.h>
#include <wumoe/console.h>

static char buf[1024];

void print(char *s) {
    console_write(s, strlen(s));
}

void printf(char *fmt, ...) {
    va_list args;
    int i;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);
    console_write(buf, i);
}
