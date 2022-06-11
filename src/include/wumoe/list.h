#ifndef WUMOE_LIST_H
#define WUMOE_LIST_H

#include <wumoe/types.h>

#define element_offset(type, member) (u32)(&((type *)0)->member)
#define element_entry(type, member, ptr) (type *)((u32)ptr - element_offset(type, member))

typedef struct list_node_t {
    struct list_node_t *prev;
    struct list_node_t *next;
} list_node_t;

typedef struct list_t {
    list_node_t head;
    list_node_t tail;
} list_t;

void list_init(list_t *list);

void list_insert_before(list_node_t *anchor, list_node_t *node);

void list_insert_after(list_node_t *anchor, list_node_t *node);

void list_push(list_t *list, list_node_t *node);

list_node_t *list_pop(list_t *list);

void list_pushback(list_t *list, list_node_t *node);

list_node_t *list_popback(list_t *list);

bool list_search(list_t *list, list_node_t *node);

void list_remove(list_node_t *node);

bool list_empty(list_t *list);

u32 list_size(list_t *list);

#endif
