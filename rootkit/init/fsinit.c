#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <fs/fs.h>
#include <storage/flash_map.h>

#ifdef CONFIG_FAT_FILESYSTEM_ELM
#include <ff.h>
#endif
#ifdef CONFIG_FILE_SYSTEM_LITTLEFS
#include <fs/littlefs.h>

#define LITTLE_FS_DEV (void *)FLASH_AREA_ID(fs)
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(liitefs_storage);
#endif

#if defined(CONFIG_FAT_FILESYSTEM_ELM) || defined(CONFIG_FILE_SYSTEM_LITTLEFS)
static struct fs_mount_t fs_mount_data;
#endif

static int base_fs_mount(struct fs_mount_t *mnt) {
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	static FATFS fat_fs;
	mnt->type = FS_FATFS;
	mnt->fs_data = &fat_fs;
	if (IS_ENABLED(CONFIG_DISK_DRIVER_RAM)) {
		mnt->mnt_point = "/RAM:";
	} else if (IS_ENABLED(CONFIG_DISK_DRIVER_SDMMC)) {
		mnt->mnt_point = "/SD:";
	} else {
		mnt->mnt_point = "/NAND:";
	}
    return fs_mount(mnt);

#elif defined(CONFIG_FILE_SYSTEM_LITTLEFS)
	mnt->type = FS_LITTLEFS;
	mnt->mnt_point = "/rootfs";
	mnt->fs_data = &liitefs_storage;
    mnt->storage_dev = (void *)LITTLE_FS_DEV;
    return fs_mount(mnt);
#endif
    return -EINVAL;
}

static int base_fs_init(const struct device *dev __unused) {
    struct fs_statvfs sbuf;
    int ret;
    ret = base_fs_mount(&fs_mount_data);
    if (ret < 0) {
        printk("FAIL: mount id %u at %s: %d\n",
                (unsigned int)fs_mount_data.storage_dev, 
                fs_mount_data.mnt_point, ret);
        goto _out;
    }
	ret = fs_statvfs(fs_mount_data.mnt_point, &sbuf);
	if (ret < 0) {
		printk("FAIL: statvfs: %d\n", ret);
		goto _out;
	}
	printk("%s: bsize = %lu ; frsize = %lu ;"
	       " blocks = %lu ; bfree = %lu\n",
	       fs_mount_data.mnt_point,
	       sbuf.f_bsize, sbuf.f_frsize,
	       sbuf.f_blocks, sbuf.f_bfree);
_out:
    return ret;
}

SYS_INIT(base_fs_init, 
    APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
