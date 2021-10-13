#ifndef BASE_U_THREAD_H_
#define BASE_U_THREAD_H_

#ifdef __cplusplus
extern "C"{
#endif

struct k_thread;

struct k_thread *u_thread_get(const char *name);
int u_thread_restart(struct k_thread *thread);
int u_thread_kill_sync(struct k_thread *thread);

#ifdef __cplusplus
}
#endif
#endif /* BASE_U_THREAD_H_ */
