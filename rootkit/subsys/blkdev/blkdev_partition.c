#define DT_DRV_COMPAT fixed_partitions

#include <zephyr.h>
#include "blkdev/blkdev.h"

#define BLKDEV_PARTITION(part)						\
	{.partition = DT_LABEL(part),				\
	 .offset = DT_REG_ADDR(part),					\
	 .devname = DT_LABEL(DT_MTD_FROM_FIXED_PARTITION(part)),	\
	 .size = DT_REG_SIZE(part)},

#define FOREACH_PARTITION(n) \
    DT_FOREACH_CHILD(DT_DRV_INST(n), BLKDEV_PARTITION)

static const struct blkdev_partition blkdev_partition_table[] = {
	DT_INST_FOREACH_STATUS_OKAY(FOREACH_PARTITION)
};
static const int blkdev_entries = ARRAY_SIZE(blkdev_partition_table);

void blkdev_partition_foreach(
    void (*user_cb)(const struct blkdev_partition *, void *), 
    void *user_data) {
	for (int i = 0; i < blkdev_entries; i++)
		user_cb(&blkdev_partition_table[i], user_data);
}
