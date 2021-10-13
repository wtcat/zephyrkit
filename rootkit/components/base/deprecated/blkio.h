#ifndef BASE_BLKIO_H_
#define BASE_BLKIO_H_

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

struct nvs_fs;
struct fs_file_t;
struct flash_area;

typedef void (*blkio_notify_t)(int);

/*
 * Request type
 */ 
#define REQ_TYPE(req) ((req) & 0x7FFF)
#define REQ_NONBLOCK  0x8000

#define BLK_REQ_INIT  0x0000
#define BLK_REQ_WIRTE 0x0001
#define BLK_REQ_READ  0x0002
#define BLK_REQ_ERASE 0x0003
#define BLK_REQ_SYNC  0x0010
#define BLK_REQ_LOAD  0x0011


int nvs_blkio_reqeust(uint32_t req, blkio_notify_t cb, struct nvs_fs *fs, 
    uint16_t id, void *data, size_t len);

int setting_blkio_reqeust(uint32_t req, blkio_notify_t cb, const char *name, 
    void *buffer, size_t len);

int fs_blkio_request(uint32_t req, blkio_notify_t cb, struct fs_file_t *zfp, 
    void *buffer, size_t len);

 int flashmap_blkio_request(uint32_t req, blkio_notify_t cb, 
    const struct flash_area *fa, off_t off, void *buffer, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* BASE_BLKIO_H_ */