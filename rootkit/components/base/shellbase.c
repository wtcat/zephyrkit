#include <shell/shell.h>

#include "base/event_manager.h"


static int shell_shutdown(const struct shell *shell,
    size_t argc, char **argv)
{
  shell_fprintf(shell, SHELL_NORMAL, "Prepare shutdown system....\n");
  event_report(EV_SYSTEM_SHUTDOWN, 1, 0, 0);
	return 0;
}

static int shell_reboot(const struct shell *shell,
    size_t argc, char **argv)
{
  shell_fprintf(shell, SHELL_NORMAL, "Prepare reboot system....\n");
  event_report(EV_SYSTEM_REBOOT, 1, 0, 0);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_command,
    SHELL_CMD(shutdown, NULL, "Shutdown", shell_shutdown),
    SHELL_CMD(reboot,   NULL, "Reboot",   shell_reboot),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sys, &sub_command, "User administration commands", NULL);
