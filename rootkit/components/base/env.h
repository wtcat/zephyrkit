#ifndef BASE_ENV_H_
#define BASE_ENV_H_

#include <stdint.h>
#include <sys/__assert.h>

#ifdef __cplusplus
extern "C"{
#endif

struct env_arg;


#define ENV(v) (uintptr_t)(v)

struct env_arg {
    uintptr_t value;
};

/*
 * Environment key
 *
 * IENV_xxxxx: integer type
 * SENV_xxxxx: string type
 *
 */
enum env_type {
    /*
     * Long press timeout time
     */
    IENV_HOME_KEY_TIMEOUT,

    /* */
    ENV_END
};

extern struct env_arg _env_room[];

static inline void set_environment_var(enum env_type key, uintptr_t value)
{
    __ASSERT(key < ENV_END, "");
    _env_room[key].value = value;
}

static inline uintptr_t get_environment_var(enum env_type key)
{
    __ASSERT(key < ENV_END, "");
    return _env_room[key].value;
}


#ifdef __cplusplus
}
#endif
#endif /* BASE_ENV_H_ */
