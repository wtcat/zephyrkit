#include <sys/types.h>
#include <kernel.h>
#include <fs/fs.h>
#include <fs/nvs.h>
#include <storage/flash_map.h>
#include <settings/settings.h>

#include "base/workqueue.h"
#include "base/blkio.h"


struct blkio_nvs {
    struct nvs_fs *fs;
    uint16_t id;
    union {
        void *buffer;
        const char *devname;
    };
    size_t len;
};

struct blkio_setting {
    const char *name;
    void *buffer;
    size_t len;
};

struct blkio_fs {
    struct fs_file_t *zfp;
    void *buffer;
    size_t len;
};

struct blkio_flashmap {
    const struct flash_area *fa;
    off_t offset;
    void *buffer;
    size_t len;
};

struct blkio_req {
    struct k_work work;
    struct blkio_req *next;
    struct k_sem *sync;
    uint32_t req;
    int result;
    union {
        struct blkio_nvs nvs;
        struct blkio_setting setting;
        struct blkio_fs fs;
        struct blkio_flashmap fmap;
    };
    blkio_notify_t notify;
};

#ifndef CONFIG_BLKIO_NUM
#define CONFIG_BLKIO_NUM 6
#endif

static WORKQUEUE_DEFINE(blkio_wq, 4096, 15);
static struct blkio_req *blkio_head;
static struct blkio_req blkio_pool[CONFIG_BLKIO_NUM];
static K_SEM_DEFINE(blkio_sem, CONFIG_BLKIO_NUM, CONFIG_BLKIO_NUM); 


static void blkio_notify_null(int result)
{
    ARG_UNUSED(result);
}

static struct blkio_req *blkio_alloc(void)
{
    struct blkio_req *blkio;
    unsigned int key;
    k_sem_take(&blkio_sem, K_FOREVER);
    key = irq_lock();
    blkio = blkio_head;
    if (blkio) {
        blkio_head = blkio->next;
        blkio->next = NULL;
    }
    irq_unlock(key);
    return blkio;
}

static void blkio_free(struct blkio_req *blkio)
{
    unsigned int key = irq_lock();
    blkio->next = blkio_head;
    blkio_head = blkio;
    irq_unlock(key);
    k_sem_give(&blkio_sem);
}

static inline void blkio_done(struct blkio_req *blkio, int result)
{
    if (blkio->req & REQ_NONBLOCK) {
        blkio_notify_t notify = blkio->notify;
        blkio_free(blkio);
        (*notify)(result);
    } else
        blkio->result = result;
    if (blkio->sync)
        k_sem_give(blkio->sync);
}

static int blkio_wait_complete(struct blkio_req *blkio, blkio_notify_t cb)
{
    blkio->notify = cb? cb: blkio_notify_null;
    if (!(blkio->req & REQ_NONBLOCK)) {
        struct k_sem sem;
        int ret;
        blkio->result = 0;
        blkio->sync = &sem;
        k_sem_init(&sem, 0, 1);
        k_work_submit_to_queue(&blkio_wq, &blkio->work);
        k_sem_take(blkio->sync, K_FOREVER);
        ret = blkio->result;
        blkio_free(blkio);
        return ret;
    }
    k_work_submit_to_queue(&blkio_wq, &blkio->work);
    return 0;
}

static void nvs_blkio_adaptor(struct k_work *work)
{
    struct blkio_req *blkio = (struct blkio_req *)work;
    struct blkio_nvs *nvs = &blkio->nvs;
    ssize_t ret;

    switch (REQ_TYPE(blkio->req)) {
    case BLK_REQ_INIT:
        ret = nvs_init(nvs->fs, nvs->devname);
        break;
    case BLK_REQ_WIRTE:
        ret = nvs_write(nvs->fs, nvs->id, nvs->buffer, nvs->len);
        break;
    case BLK_REQ_READ:
        ret = nvs_read(nvs->fs, nvs->id, nvs->buffer, nvs->len);
        break;
    case BLK_REQ_ERASE:
        ret = nvs_delete(nvs->fs, nvs->id);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }
    blkio_done(blkio, ret);
}

static void setting_blkio_adaptor(struct k_work *work)
{
    struct blkio_req *blkio = (struct blkio_req *)work;
    struct blkio_setting *setting = &blkio->setting;
    ssize_t ret;

    ret = settings_subsys_init();
    if (ret) 
        goto _exit;

    switch (REQ_TYPE(blkio->req)) {
    case BLK_REQ_INIT:
        break;
    case BLK_REQ_WIRTE:
        ret = settings_save_one(setting->name, setting->buffer, 
            setting->len);
        break;
    case BLK_REQ_ERASE:
        ret = settings_delete(setting->name);
        break;
    case BLK_REQ_SYNC:
        ret = settings_save();
        break;
    case BLK_REQ_LOAD:
        ret = settings_load_subtree(setting->name);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }

_exit:
    blkio_done(blkio, ret);
}

static void fs_blkio_adaptor(struct k_work *work)
{
    struct blkio_req *blkio = (struct blkio_req *)work;
    struct blkio_fs *fs = &blkio->fs;
    ssize_t ret;

    switch (REQ_TYPE(blkio->req)) {
    case BLK_REQ_WIRTE:
        ret = fs_write(fs->zfp, fs->buffer, fs->len);
        break;
    case BLK_REQ_READ:
        ret = fs_read(fs->zfp, fs->buffer, fs->len);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }
    blkio_done(blkio, ret);
}

static void flashmap_blkio_adaptor(struct k_work *work)
{
    struct blkio_req *blkio = (struct blkio_req *)work;
    struct blkio_flashmap *fmap = &blkio->fmap;
    ssize_t ret;

    switch (REQ_TYPE(blkio->req)) {
    case BLK_REQ_WIRTE:
        ret = flash_area_write(fmap->fa, fmap->offset, fmap->buffer, fmap->len);
        break;
    case BLK_REQ_READ:
        ret = flash_area_read(fmap->fa, fmap->offset, fmap->buffer, fmap->len);
        break;
    case BLK_REQ_ERASE:
        ret = flash_area_erase(fmap->fa, fmap->offset, fmap->len);
        break;
    default:
        ret = -ENOTSUP;
        break;
    }
    blkio_done(blkio, ret);
}

int nvs_blkio_reqeust(uint32_t req, blkio_notify_t cb, struct nvs_fs *fs, 
    uint16_t id, void *data, size_t len)
{
    struct blkio_req *blkio;
    blkio = blkio_alloc();
    k_work_init(&blkio->work, nvs_blkio_adaptor);
    blkio->nvs.fs = fs;
    blkio->nvs.id = id;
    blkio->nvs.len = len;
    return blkio_wait_complete(blkio, cb);
}

int setting_blkio_reqeust(uint32_t req, blkio_notify_t cb, const char *name, 
    void *buffer, size_t len)
{
    struct blkio_req *blkio;
    blkio = blkio_alloc();
    k_work_init(&blkio->work, setting_blkio_adaptor);
    blkio->setting.name = name;
    blkio->setting.buffer = buffer;
    blkio->setting.len = len;
    return blkio_wait_complete(blkio, cb);
}

int fs_blkio_request(uint32_t req, blkio_notify_t cb, struct fs_file_t *zfp, 
    void *buffer, size_t len)
{
    struct blkio_req *blkio;
    blkio = blkio_alloc();
    k_work_init(&blkio->work, fs_blkio_adaptor);
    blkio->fs.zfp = zfp;
    blkio->fs.buffer = buffer;
    blkio->fs.len = len;
    return blkio_wait_complete(blkio, cb);
}

int flashmap_blkio_request(uint32_t req, blkio_notify_t cb, 
    const struct flash_area *fa, off_t off, void *buffer, size_t len)
{
    struct blkio_req *blkio;
    blkio = blkio_alloc();
    k_work_init(&blkio->work, flashmap_blkio_adaptor);
    blkio->fmap.fa = fa;
    blkio->fmap.offset = off;
    blkio->fmap.buffer = buffer;
    blkio->fmap.len = len;
    return blkio_wait_complete(blkio, cb);
}

static int blkio_init(const struct device *dev)
{ 
    ARG_UNUSED(dev); 
    blkio_head = &blkio_pool[0];
    for (int i = 1; i < CONFIG_BLKIO_NUM; i++) 
        blkio_pool[i - 1].next = &blkio_pool[i];
    return 0;
}

SYS_INIT(blkio_init, PRE_KERNEL_1, 
    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
