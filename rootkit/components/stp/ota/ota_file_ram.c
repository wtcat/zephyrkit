#include <string.h>
#include <errno.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(stp_ota);

#include "stp/ota/ota_file.h"

#include <sys/crc.h>

#define FILE_MAX_SIZE (64 * 1024)

static uint8_t ota_file_memory[FILE_MAX_SIZE];

static int ota_ram_open(struct ota_file *ota)
{
    struct ota_filedev *fdev = &ota->fdev;
    fdev->file_offset = 0;
    return 0;
}

static int ota_ram_seek(struct ota_file *ota, off_t offset, int whence)
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
    LOG_INF("%s(): File offset pointer is %d\n", __func__, fdev->file_offset);
    return 0;
}

static ssize_t ota_ram_write(struct ota_file *ota, const char *buffer,
    size_t size)
{
    struct ota_filedev *fdev = &ota->fdev;
    off_t offset = fdev->file_offset;
    size_t copy_bytes;

    if (offset + size > FILE_MAX_SIZE) 
        copy_bytes = FILE_MAX_SIZE - offset;
    else
        copy_bytes = size;
    memcpy(ota_file_memory + offset, buffer, copy_bytes);
    fdev->file_offset = offset + copy_bytes;
    fdev->file_size = fdev->file_offset;
    LOG_INF("File offset pointer(0x%x), size: %d\n", offset, fdev->file_size);
    return copy_bytes;
}

static ssize_t ota_ram_read(struct ota_file *ota, char *buffer, 
    size_t size)
{
    struct ota_filedev *fdev = &ota->fdev;
    off_t offset = fdev->file_offset;
    size_t copy_bytes;

    if (offset + size > FILE_MAX_SIZE) 
        copy_bytes = FILE_MAX_SIZE - offset;
    else
        copy_bytes = size;
    
    memcpy(buffer, ota_file_memory + offset, copy_bytes);
    fdev->file_offset = offset + copy_bytes;
    return copy_bytes;
}
    
static int ota_ram_close(struct ota_file *ota)
{
    return 0;
}

const struct ota_operations ota_ram_ops = {
    .name = "RAM",
    .open = ota_ram_open,
    .seek = ota_ram_seek,
    .write = ota_ram_write,
    .read = ota_ram_read,
    .close = ota_ram_close,
};

