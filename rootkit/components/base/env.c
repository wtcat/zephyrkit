#include "base/env.h"

#define _ENV_ITEM(key, val) \
    [key] = {val}

    
/*
 * Environment variables list
 */
struct env_arg _env_room[ENV_END] = {
    _ENV_ITEM(IENV_HOME_KEY_TIMEOUT, 3000), /* 3000ms */
};

