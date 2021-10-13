#include <zephyr.h>
#include <device.h>
#include <disk/disk_access.h>
#include <drivers/flash.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

#include <string.h>

LOG_MODULE_REGISTER(fstest);


static int flash_test(void)
{
    const struct device *dev = device_get_binding("SPI_FLASH_RW");
    const char text[] = {"Flash test text"};
    char buff[32];
    int ret;
    
    if (!dev)
        return -EINVAL;

    memset(buff, 0, sizeof(buff));
    flash_read(dev, 0, buff, sizeof(text));
    
    ret = flash_erase(dev, 0, 4096);
    if (ret)
        goto _out;

    memset(buff, 0, sizeof(buff));
    flash_read(dev, 0, buff, sizeof(text));
    
    ret = flash_write(dev, 0, text, sizeof(text));
    if (ret)
        goto _out;

    memset(buff, 0, sizeof(buff));
    ret = flash_read(dev, 0, buff, sizeof(text));
    if (ret)
        goto _out;

    if (memcmp(buff, text, sizeof(text)))
        return -EINVAL;

_out:
    return ret;
}


static int fat_fs_mount(const char *volume, const char *mp)
{
    static FATFS fat_fs;
    static struct fs_mount_t mount_info = {
    	.type = FS_FATFS,
    	.fs_data = &fat_fs,
    };
    int err;

    mount_info.mnt_point = mp;
    err = disk_access_init(volume);
    if (err) {
        LOG_ERR("%s is not exist\n", volume);
        return err;
    }

    err = fs_mount(&mount_info);
    if (err != FR_OK)
        LOG_ERR("Mounting %s to %s failed with error: %d\n", volume, mp, err);

    return err;
}

static void fs_test_thread(void)
{
    char text[] = {"Hello, Zephyr!"};
    char buffer[sizeof(text) + 1];
    struct fs_file_t fd;
    int ret;

    LOG_INF("Starting test flash device...\n");
    ret = flash_test();
    if (ret)
        goto _exit;
    
    LOG_INF("Starting test filesystem...\b");
    ret = fat_fs_mount(CONFIG_DISK_FLASH_VOLUME_NAME, "/1:");
    if (ret)
        goto _exit;
    
    ret = fs_open(&fd, "/1:/hello.txt", FS_O_CREATE|FS_O_WRITE);
    if (ret < 0)
        goto _exit;

    ret = fs_write(&fd, text, sizeof(text));
    if (ret < 0)
        goto _close;

    ret = fs_close(&fd);
    if (ret < 0)
        goto _exit;

    k_sleep(K_MSEC(2000));

    ret = fs_open(&fd, "/1:/hello.txt", FS_O_READ);
    if (ret < 0)
        goto _exit;

    memset(buffer, 0, sizeof(buffer));
    ret = fs_read(&fd, buffer, sizeof(text));
    if (ret < 0)
        goto _close;

    if (memcmp(text, buffer, sizeof(text))) {
        LOG_ERR("File system test failed!!!!!!!");
        goto _close;
    }

    LOG_INF("File system test successful\n");

_close:
    fs_close(&fd);
_exit:
    LOG_INF("fs_test thread is exit now\n");
    k_thread_abort(k_current_get());
}

K_THREAD_DEFINE(fs_test, 2048, 
                fs_test_thread, NULL, NULL, NULL,
                10, 0, 0);

