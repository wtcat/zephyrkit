#ifndef STP_OTA_FILE_H_
#define STP_OTA_FILE_H_

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <sys/types.h>

#include <fs/fs.h>

#ifdef __cplusplus
extern "C"{
#endif

struct ota_file;

/* Command */
#define OTA_GET_WRITE_BLKSIZE 0x01

/* Whence */
#define OTA_SEEK_SET FS_SEEK_SET
#define OTA_SEEK_CUR FS_SEEK_CUR
#define OTA_SEEK_END FS_SEEK_END


struct ota_filebkpt {
    uint16_t maxno; /* max packet number */
    uint16_t no;      /* current packet number */
    uint32_t crc;     /* current file checksum */
} __packed;

struct ota_reqack {
    uint16_t maxsize;  /* unit: kbytes */
    uint8_t battery;   /* 0 - 100 */
} __packed;

struct ota_operations {
    const char *name;
    int (*open)(struct ota_file *ota);
    int (*seek)(struct ota_file *ota, off_t offset, int whence);
    ssize_t (*write)(struct ota_file *ota, const char *buffer, size_t size);
    ssize_t (*read)(struct ota_file *ota, char *buffer, size_t size);
    int (*close)(struct ota_file *ota);
    int (*control)(struct ota_file *ota, unsigned int cmd, void *arg);
};


struct ota_filedev {
    const char *name;
    const struct device *dev;
    off_t file_start;
    off_t file_offset;
    size_t file_size;
    size_t blk_size;
};

struct ota_filefs {
    const char name[32];
    struct fs_file_t file;
};

struct ota_file {
    union {
        struct ota_filedev fdev;
        struct ota_filefs fd;
    };
    const struct ota_operations *f_ops;
    char *buffer;
    off_t blen;
    size_t bsize;
    off_t offset;
    uint16_t packets;
    uint16_t packet_no;
    uint16_t packet_size;
    uint32_t checksum;
};


static inline int ota_file_open(struct ota_file *ota)
{
    return ota->f_ops->open(ota);
}

static inline int ota_file_seek(struct ota_file *ota, 
    off_t offset, int whence)
{
    return ota->f_ops->seek(ota, offset, whence);
}

static inline ssize_t ota_file_write(struct ota_file *ota, 
    const char *buffer, size_t size)
{
    return ota->f_ops->write(ota, buffer, size);
}

static inline ssize_t ota_file_read(struct ota_file *ota, 
    char *buffer, size_t size)
{
    return ota->f_ops->read(ota, buffer, size);
}

static inline int ota_file_close(struct ota_file *ota)
{
    return ota->f_ops->close(ota);
}

static inline int ota_file_control(struct ota_file *ota, 
    unsigned int cmd, void *arg)
{
    if (ota->f_ops->control)
        return ota->f_ops->control(ota, cmd, arg);
    return -ENOTSUP;
}


extern const struct ota_operations ota_ram_ops;
extern const struct ota_operations ota_file_ops;
extern const struct ota_operations ota_flash_ops;

uint32_t ota_file_checksum(struct ota_file *ota);
bool ota_file_get_context(struct ota_file *ota, struct ota_filebkpt *ctx);
uint8_t ota_file_receive(struct ota_file *ota, const void *buf, size_t size);
    
#ifdef __cplusplus
}
#endif
#endif /* STP_OTA_FILE_H_ */
