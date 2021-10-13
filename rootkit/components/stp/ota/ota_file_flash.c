#include <zephyr/types.h>
#include <sys/types.h>
#include <device.h>
#include <drivers/flash.h>

#include "stp/ota/ota_file.h"

static int ota_flash_open(struct ota_file *ota)
{
    struct ota_filedev *fdev = &ota->fdev;
    if (!fdev->name)
        return -EINVAL;
    fdev->dev = device_get_binding(fdev->name);
    if (!fdev->dev)
        return -EEXIST;

    fdev->file_offset = 0;
    return 0;
}

static int ota_flash_seek(struct ota_file *ota, off_t offset, int whence)
{
    struct ota_filedev *fdev = &ota->fdev;
    
    switch (whence) {
    case OTA_SEEK_SET:
        fdev->file_offset = offset;
        break;
    case OTA_SEEK_CUR:
        fdev->file_offset += offset;
        break;
    case OTA_SEEK_END:
        fdev->file_offset = offset + ota->offset;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static ssize_t ota_flash_write(struct ota_file *ota, const char *buffer,
    size_t size)
{
    struct ota_filedev *fdev = &ota->fdev;
    off_t offset = fdev->file_start + fdev->file_offset;
    int ret;

    ret = flash_write_protection_set(fdev->dev, false);
    if (ret)
        goto _exit;
    
    ret = flash_erase(fdev->dev, offset, fdev->blk_size);
    if (ret)
        goto _wp_enable;

    ret = flash_write(fdev->dev, offset, buffer, size);
    if (ret)
        goto _wp_enable;

    fdev->file_offset += size;
    
_wp_enable:
    flash_write_protection_set(fdev->dev, true);
_exit:
    return ret;
}

static ssize_t ota_flash_read(struct ota_file *ota, char *buffer, 
    size_t size)
{
    struct ota_filedev *fdev = &ota->fdev;
    off_t offset = fdev->file_start + fdev->file_offset;
    int ret;

    ret = flash_read(fdev->dev, offset, buffer, size);
    if (!ret)
        fdev->file_offset += size;
    return size;
}
    
static int ota_flash_close(struct ota_file *ota)
{
    return 0;
}

const struct ota_operations ota_flash_ops = {
    .name = "FLASH",
    .open = ota_flash_open,
    .seek = ota_flash_seek,
    .write = ota_flash_write,
    .read = ota_flash_read,
    .close = ota_flash_close,
};

