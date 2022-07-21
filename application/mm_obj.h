/*
 * CopyRight 2022 wtcat
 */
#ifndef MM_OBJECT_H_
#define MM_OBJECT_H_

#include <stdint.h>
#include "mm_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MM_DEFINE
#define __mm_obj
#else
#define __mm_obj extern
#endif

#ifndef MM_LOCK_DECLARE
#define MM_LOCK_DECLARE(l)
#endif

struct mm_obj {
    struct mm_obj* next;
    const void* ptr;
    size_t len;
    union {
        const char* caller;
        const void* caller_addr;
    };
};

struct mm_head {
#define MAX_MOBJ_NR     2000
#define MAX_MOBJ_LOGN   8
#define MAX_MOBJ_HASH    (0x1ul << MAX_MOBJ_LOGN)

    MM_LOCK_DECLARE(lock)
    int free_blocks;
    char* free_list;
    void* free_ptr;
    struct mm_obj* heads[MAX_MOBJ_HASH];
    struct mm_obj nodes[MAX_MOBJ_NR];
};

#define MM_OBJHEAD_DECLARE(_name) \
    __mm_obj struct mm_head _name


void mm_init(struct mm_head* head);
int mm_add(struct mm_head* head, const char* name, const void* obj, size_t len);
struct mm_obj* mm_find(struct mm_head* head, const void* obj);
int mm_del_locked(struct mm_head* head, const void* obj);
int mm_del(struct mm_head* head, const void* obj, void (*mfree)(void* obj));
void mm_iterate(struct mm_head* head, void (*iterator)(struct mm_obj*, void*),
    void* context);

static inline void mm_free(struct mm_head* head,
    void (*free)(void*), void* ptr) {
    mm_del(head, ptr, free);
}

static inline void* mm_malloc(struct mm_head* head,
    void* (*malloc)(size_t), size_t size, const char* func) {
    void* ptr = malloc(size);
    MM_ASSERT(ptr != NULL);
    mm_add(head, (const char*)func, ptr, size);
    return ptr;
}

static inline void* mm_calloc(struct mm_head* head,
    void* (*calloc)(size_t, size_t), size_t n, size_t size,
    const char* func) {
    void* ptr = calloc(n, size);
    MM_ASSERT(ptr != NULL);
    mm_add(head, (const char*)func, ptr, size);
    return ptr;
}

#ifdef __cplusplus
}
#endif
#endif /* MM_OBJECT_H_ */
