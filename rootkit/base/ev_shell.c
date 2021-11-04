#include <stdlib.h>
#include <shell/shell.h>

#include "base/ev_types.h"
#include "base/evmgr.h"



static int ev_shell_message(const struct shell *shell,
    size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    int ret = ev_submit(EV_PRIO_MESSAGE, 0, 0);
    ev_sync();
    return ret;
}

static int ev_shell_ota(const struct shell *shell,
    size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    int ret = ev_submit(EV_PRIO_OTA, 0, 0);
    ev_sync();
    return ret;
}

static int ev_shell_goal(const struct shell *shell,
    size_t argc, char **argv)
{
    unsigned long type;
    unsigned long val;
    int ret;
    
    if (argc != 3) {
        shell_fprintf(shell, SHELL_ERROR, 
            "Invalid format:\n %s\n", shell->ctx->active_cmd.help);
        return -EINVAL;
    }
    type = strtoul(argv[1], NULL, 10);
    val = strtoul(argv[2], NULL, 10);
    if (type >= 4) {
        shell_fprintf(shell, SHELL_ERROR, 
            "Invalid value: must be less than 4");
        return -EINVAL;
    }
    switch (type) {
    case 0:
        ret = ev_submit(EV_PRIO_GOAL1_COMPLETED, val, sizeof(val));
        break;
    case 1:
        ret = ev_submit(EV_PRIO_GOAL2_COMPLETED, val, sizeof(val));
        break;
    case 2:
        ret = ev_submit(EV_PRIO_GOAL3_COMPLETED, val, sizeof(val));
        break;
    case 3:
        ret = ev_submit(EV_PRIO_GOAL4_COMPLETED, val, sizeof(val));
        break;
    default:
        shell_fprintf(shell, SHELL_ERROR, 
            "Invalid value: must be less than 4");
        return -EINVAL;
    }
    return ret;
}

static int ev_shell_charging(const struct shell *shell,
    size_t argc, char **argv)
{
    long value;

    if (argc == 2) {
        value = strtol(argv[1], NULL, 10);
        return ev_submit(EV_PRIO_CHARGING, (int)value, 
            sizeof(int));
    } 
    shell_fprintf(shell, SHELL_ERROR, "Invalid format\n");
    return -EINVAL;
}

static int ev_shell_lowpower(const struct shell *shell,
    size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    shell_fprintf(shell, SHELL_WARNING, 
                    "Not support\n");
    return 0;
}

static int ev_shell_ringing(const struct shell *shell,
    size_t argc, char **argv)
{
    int ret;

    (void) argc;
    (void) argv;
    if (argc != 1) {
        shell_fprintf(shell, SHELL_ERROR, 
            "Invalid format:\n %s\n", shell->ctx->active_cmd.help);
        return -EINVAL;
    }
    ret = ev_submit(EV_PRIO_RINGING, 0, 0);
    ev_sync();
    return ret;
}

static int ev_shell_alarm(const struct shell *shell,
    size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    shell_fprintf(shell, SHELL_WARNING, 
                    "Not support\n");
    return 0;
} 

static int ev_shell_sporting(const struct shell *shell,
    size_t argc, char **argv)
{
    (void) argc;
    (void) argv;
    shell_fprintf(shell, SHELL_WARNING, 
                    "Not support\n");
    return 0;
} 

SHELL_STATIC_SUBCMD_SET_CREATE(subcmd_set,
    SHELL_CMD(message, NULL, "", ev_shell_message),
    SHELL_CMD(ota, NULL, "", ev_shell_ota),
    SHELL_CMD(goal, NULL, "Format: goal [type] [val]", ev_shell_goal),
    SHELL_CMD(charging, NULL, "Format: chg [val]", ev_shell_charging),
    SHELL_CMD(lowpwr, NULL, "", ev_shell_lowpower),
    SHELL_CMD(call, NULL, "", ev_shell_ringing),
    SHELL_CMD(alarm, NULL, "", ev_shell_alarm),
    SHELL_CMD(sport, NULL, "", ev_shell_sporting),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ev, &subcmd_set, 
    "system event commands", 
    NULL);

