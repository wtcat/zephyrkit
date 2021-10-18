#ifndef BASE_CONTAINER_NOTIFIER_H_
#define BASE_CONTAINER_NOTIFIER_H_

#ifdef __cplusplus
extern "C"{
#endif


struct observer_base;
typedef int (*observer_fn_t)(struct observer_base *nb,
			unsigned long action, void *data);

struct observer_base {
	observer_fn_t update;
	struct observer_base  *next;
	int priority;
};


#define NOTIFY_DONE		0x0000		/* Don't care */
#define NOTIFY_OK		0x0001		/* Suits me */
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)
						/* Bad/Veto action */
/*
 * Clean way to return from the notifier and stop further calls.
 */
#define NOTIFY_STOP		(NOTIFY_OK|NOTIFY_STOP_MASK)


#define OBSERVER_STATIC_INIT(call, pri) \
	{ \
		.update = call, \
		.next = NULL, \
		.priority = pri \
	}


int observer_register(struct observer_base **nl,
		struct observer_base *n);

int observer_cond_register(struct observer_base **nl,
		struct observer_base *n);

int observer_unregister(struct observer_base **nl,
		struct observer_base *n);

int observer_notify(struct observer_base **nl,
			       unsigned long val,
                   void *v);

#ifdef __cplusplus
}
#endif
#endif // BASE_CONTAINER_NOTIFIER_H_
