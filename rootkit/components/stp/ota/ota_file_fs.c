#include <string.h>
#include <errno.h>
#include <fs/fs.h>

#include "stp/ota/ota_file.h"

static int ota_fs_open(struct ota_file *ota)
{
    char name[64] = {"/1:/"};
    strcat(name, ota->fd.name);
    return fs_open(&ota->fd.file, name, 
        FS_O_CREATE|FS_O_RDWR);
}

static int ota_fs_seek(struct ota_file *ota, off_t offset, int whence)
{
    return fs_seek(&ota->fd.file, offset, whence);
}

static ssize_t ota_fs_write(struct ota_file *ota, const char *buffer,
    size_t size)
{
    return fs_write(&ota->fd.file, buffer, size);
}

static ssize_t ota_fs_read(struct ota_file *ota, char *buffer, 
    size_t size)
{
    return fs_read(&ota->fd.file, buffer, size);
}
    
static int ota_fs_close(struct ota_file *ota)
{
    return fs_close(&ota->fd.file);
}

const struct ota_operations ota_file_ops = {
    .name = "Filesystem",
    .open = ota_fs_open,
    .seek = ota_fs_seek,
    .write = ota_fs_write,
    .read = ota_fs_read,
    .close = ota_fs_close,
};

