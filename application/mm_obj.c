/*
 * CopyRight 2022 wtcat
 */
#include <errno.h>

#include "mm_obj.h"

#define MM_HASH(v) \
    (((uintptr_t)(v) ^ ((uintptr_t)(v) >> MAX_MOBJ_LOGN)) & (MAX_MOBJ_HASH - 1))

static void mm_iterator(struct mm_obj* node, void* context) {
    (void)context;
    PRINTF("$Memory <0x%p %6lu> (%s)\n", node->ptr, node->len, node->caller);
}

static struct mm_obj* mmo_alloc(struct mm_head* head) {
    char* free_node = head->free_list;
    MM_ASSERT(free_node != NULL);
    if (free_node) {
        head->free_list = *(char**)free_node;
        head->free_blocks--;
        return (struct mm_obj*)free_node;
    }
    return NULL;
}

static void mmo_free(struct mm_head* head, void* node) {
    MM_ASSERT(node != NULL);
    if (node) {
        *(char**)node = head->free_list;
        head->free_list = node;
        head->free_blocks++;
    }
}

int mm_add(struct mm_head* head, const char* name,
    const void* obj, size_t len) {
    MUTEX_LOCK_DECLARE
    MUTEX_LOCK(head);
    struct mm_obj* node = mmo_alloc(head);
    int index = MM_HASH(obj);
    node->caller = name;
    node->ptr = obj;
    node->len = len;
    node->next = head->heads[index];
    head->heads[index] = node;
    MUTEX_UNLOCK(head);
    return 0;
}

struct mm_obj* mm_find(struct mm_head* head, const void* obj) {
    MUTEX_LOCK_DECLARE
    MUTEX_LOCK(head);
    int index = MM_HASH(obj);
    for (struct mm_obj** node = &head->heads[index];
        *node != NULL;
        node = &(*node)->next) {
        if ((*node)->ptr == obj) {
            MUTEX_UNLOCK(head);
            return *node;
        }
    }
    MUTEX_UNLOCK(head);
    return NULL;
}

int mm_del_locked(struct mm_head* head, const void* obj) {
    struct mm_obj** node, * tmp;
    int index = MM_HASH(obj);
    for (node = &head->heads[index]; *node != NULL;
        node = &(*node)->next) {
        if ((*node)->ptr == obj) {
            tmp = *node;
            *node = tmp->next;
            tmp->next = NULL;
            mmo_free(head, tmp);
            return 0;
        }
    }
    return -EINVAL;
}

int mm_del(struct mm_head* head, const void* obj,
    void (*mfree)(void* obj)) {
    MM_ASSERT(mfree != NULL);
    MUTEX_LOCK_DECLARE
        if (obj == NULL)
            return -EINVAL;
    MUTEX_LOCK(head);
    head->free_ptr = (void*)obj;
    mfree((void*)obj);
    mm_del_locked(head, obj);
    MUTEX_UNLOCK(head);
    return 0;
}

void mm_iterate(struct mm_head* head,
    void (*iterator)(struct mm_obj*, void*),
    void* context) {
    unsigned int sum = 0;
    PRINTF("\n***************Heap Dump***************\n");
    MUTEX_LOCK_DECLARE
    if (iterator == NULL)
        iterator = mm_iterator;
    MUTEX_LOCK(head);
    for (int i = 0; i < (int)MAX_MOBJ_HASH; i++) {
        for (struct mm_obj* node = head->heads[i];
            node != NULL; node = node->next) {
            MUTEX_UNLOCK(head);
            sum += node->len;
            iterator(node, context);
            MUTEX_LOCK(head);
        }
    }
    MUTEX_UNLOCK(head);
    PRINTF("Memory Used: %u\n", sum);
}

void mm_init(struct mm_head* head) {
    char* p = (char*)head->nodes;
    head->free_blocks = MAX_MOBJ_NR;
    head->free_list = NULL;
    for (int i = 0; i < head->free_blocks; i++) {
        *(char**)p = head->free_list;
        head->free_list = p;
        p += sizeof(struct mm_obj);
    }
}


#ifdef USE_UNIT_TEST
#include <string.h>
#define STD_MALLOC(size, _func)  mm_malloc(&mm_instant, malloc, size, _func)
#define STD_FREE(ptr)            mm_free(&mm_instant, free, ptr)

static struct mm_head mm_instant;
static int m_ready;

void *lvgl_malloc(size_t size, const char *func) {
    if (!m_ready) {
        m_ready = 1;
        mm_init(&mm_instant);
    }
    void* ptr = STD_MALLOC(size + sizeof(size_t), func);
    *(size_t*)ptr = size;
    return (char*)ptr + sizeof(size_t);
}

void lvgl_free(void* ptr) {
    void* mem = (char*)ptr - sizeof(size_t);
    STD_FREE(mem);
}

void* lvgl_realloc(void* ptr, size_t size, const char *func) {
    if (size == 0) {
        if (ptr != NULL)
            lvgl_free(ptr);
        return NULL;
    }
    void* mem = lvgl_malloc(size, func);
    if (mem && ptr) {
        size_t prevsize = *(size_t*)((char*)ptr - sizeof(size_t));
        memcpy(mem, ptr, prevsize < size? prevsize: size);
        lvgl_free(ptr);
    }
    return mem;
}

void lvgl_mdump(void) {
    mm_iterate(&mm_instant, NULL, NULL);
}


#if defined(__linux__)
int main(int argc, char *argv[]) {
    void *ptrs[4];

    mm_init(&mm_instant);
    m_ready = 1;
    lvgl_mdump();

    for (int i = 0; i < sizeof(ptrs)/sizeof(ptrs[0]); i++) {
        ptrs[i] = lvgl_malloc((i + 8) * (i * 1), __func__);
    }
    lvgl_mdump();

    for (int i = 0; i < sizeof(ptrs)/sizeof(ptrs[0]); i++) {
        if (ptrs[i] != NULL)
            lvgl_free(ptrs[i]);
    }
    lvgl_mdump();
    return 0;
}
#endif
#endif //USE_UNIT_TEST
