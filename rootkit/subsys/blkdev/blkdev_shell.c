#include <shell/shell.h>
#include <inttypes.h>

#include "blkdev/blkdev.h"

static int blkdev_statistics(const struct shell *shell,
  size_t argc, char **argv) {
  const struct k_blkdev_stats *stats;
  struct k_blkdev *bdev;
  uint32_t media_block_size;
  uint32_t media_block_count;
  uint32_t block_size;

  if (argc != 2) {
    shell_fprintf(shell, SHELL_ERROR, "Invalid format\n");
    return -EINVAL;
  }
  bdev = k_blkdev_get(argv[1]);
  if (bdev == NULL) {
    shell_fprintf(shell, SHELL_ERROR, "(%s) partition is not exist\n", argv[1]);
    return -EEXIST;
  }
  media_block_size = bdev->dd.media_block_size;
  media_block_count = bdev->dd.media_blocks_per_block *
    bdev->dd.block_count;
  block_size = bdev->dd.block_size;
  stats = &bdev->dd.stats;
  shell_fprintf(shell, SHELL_NORMAL,
     "-------------------------------------------------------------------------------\n"
     "                               DEVICE STATISTICS\n"
     "----------------------+--------------------------------------------------------\n"
     " MEDIA BLOCK SIZE     | %" PRIu32 "\n"
     " MEDIA BLOCK COUNT    | %" PRIu32 "\n"
     " BLOCK SIZE           | %" PRIu32 "\n"
     " READ HITS            | %" PRIu32 "\n"
     " READ MISSES          | %" PRIu32 "\n"
     " READ AHEAD TRANSFERS | %" PRIu32 "\n"
     " READ AHEAD PEEKS     | %" PRIu32 "\n"
     " READ BLOCKS          | %" PRIu32 "\n"
     " READ ERRORS          | %" PRIu32 "\n"
     " WRITE TRANSFERS      | %" PRIu32 "\n"
     " WRITE BLOCKS         | %" PRIu32 "\n"
     " WRITE ERRORS         | %" PRIu32 "\n"
     "----------------------+--------------------------------------------------------\n",
     media_block_size,
     media_block_count,
     block_size,
     stats->read_hits,
     stats->read_misses,
     stats->read_ahead_transfers,
     stats->read_ahead_peeks,
     stats->read_blocks,
     stats->read_errors,
     stats->write_transfers,
     stats->write_blocks,
     stats->write_errors
  );
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_command,
    SHELL_CMD(stats, NULL, 
      "Block device statistics information\n blkdev stats [partition name]", 
      blkdev_statistics),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(blkdev, &sub_command, "Block device", NULL);
