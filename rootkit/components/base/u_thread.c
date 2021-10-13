#include <version.h>
#include <kernel.h>
#include <string.h>
#include <errno.h>

struct kiter_data {
    struct k_thread *thread;
    const char *name;
};

#if !defined(CONFIG_THREAD_MONITOR) || \
    !defined(CONFIG_THREAD_STACK_INFO)

 #error "Invalid system configuration"
#endif /* */

static void thread_iterator(const struct k_thread *thread, void *data)
{
    struct kiter_data *kv = data;
    const char *name;

    if (kv->thread)
        return;
    name = k_thread_name_get((struct k_thread *)thread);
    if (!strcmp(name, kv->name))
        kv->thread = (struct k_thread *)thread;
}

// TODO: Add syscall
int u_thread_restart(struct k_thread *thread)
{
#if KERNEL_VERSION_NUMBER < ZEPHYR_VERSION(2,6,0)
    void (*fn_abort)(struct k_thread *);
	void *stack_ptr;
	size_t stack_size;
	
	if (!(thread->base.thread_state & _THREAD_DEAD))
		return -EBUSY;
#ifdef CONFIG_USERSPACE		
	stack_ptr = Z_THREAD_STACK_BUFFER((k_thread_stack_t *)thread->stack_info.start) -
				2 * K_THREAD_STACK_RESERVED;
	stack_size = thread->stack_info.size;
#else
	stack_ptr = Z_KERNEL_STACK_BUFFER((k_thread_stack_t *)thread->stack_info.start) - 
				2 * K_KERNEL_STACK_RESERVED;
	stack_size = thread->stack_info.size;
#endif
    fn_abort = thread->fn_abort;
	k_thread_create(thread, 
			(k_thread_stack_t *)stack_ptr,
			stack_size,
			thread->entry.pEntry,
			thread->entry.parameter1,
			thread->entry.parameter2,
			thread->entry.parameter3,
			thread->base.prio,
			thread->base.user_options,
			K_NO_WAIT);
    thread->fn_abort = fn_abort;
#endif
	return 0;
}

struct k_thread *u_thread_get(const char *name)
{
    struct kiter_data kv;

    if (name == NULL)
        return NULL;
    kv.thread = NULL;
    kv.name = name;
    k_thread_foreach(thread_iterator, &kv);
    return kv.thread;    
}

int u_thread_kill_sync(struct k_thread *thread)
{
    if (thread) {
        k_thread_abort(thread);
        k_thread_join(thread, K_FOREVER);
        return 0;
    }
    return -EINVAL;
}
