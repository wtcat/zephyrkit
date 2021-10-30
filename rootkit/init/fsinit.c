#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <fs/fs.h>
#include <storage/flash_map.h>


#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <fs/littlefs.h>

#define LITTLE_FS_DEV (void *)FLASH_AREA_ID(fs)
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(liitefs_storage);
#endif

static struct fs_mount_t fsmount_table[] = {
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
    {
        .type = FS_LITTLEFS,
        .fs_data = &liitefs_storage,
        .storage_dev = (void *)LITTLE_FS_DEV,
        .mnt_point = "/rootfs"
    },
#endif
};

static int base_fs_init(const struct device *dev)
{
    int ret = 0;
    
    ARG_UNUSED(dev);
    for (int i = 0; i < ARRAY_SIZE(fsmount_table); i++) {
        struct fs_mount_t *mp = &fsmount_table[i];
        ret = fs_mount(mp);
        if (ret < 0) {
            printk("FAIL: mount id %u at %s: %d\n",
                   (unsigned int)mp->storage_dev, mp->mnt_point, ret);
            break;
        }
    }
    return ret;
}

SYS_INIT(base_fs_init, 
    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
