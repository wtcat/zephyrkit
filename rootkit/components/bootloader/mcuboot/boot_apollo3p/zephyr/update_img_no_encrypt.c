#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <drivers/flash.h>
#include <drivers/timer/system_timer.h>
#include <usb/usb_device.h>
#include <soc.h>
#include <linker/linker-defs.h>

#include "kernel.h"
#include "storage/flash_map.h"
#include "sys/printk.h"
#include "target.h"

#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil_priv.h" 

#include "boot_event.h"

static int
boot_image_load_header(const struct flash_area *fa_p,
                       struct image_header *hdr)
{
    uint32_t size;
    int rc = flash_area_read(fa_p, 0, hdr, sizeof *hdr);

    if (rc != 0) {
        rc = BOOT_EFLASH;
        //BOOT_LOG_ERR("Failed reading image header");
	    return BOOT_EFLASH;
    }

    if (hdr->ih_magic != IMAGE_MAGIC) {
        //BOOT_LOG_ERR("Bad image magic 0x%lx", (unsigned long)hdr->ih_magic);
        return BOOT_EBADIMAGE;
    }

    if (hdr->ih_flags & IMAGE_F_NON_BOOTABLE) {
        //BOOT_LOG_ERR("Image not bootable");
        return BOOT_EBADIMAGE;
    }

    if (!boot_u32_safe_add(&size, hdr->ih_img_size, hdr->ih_hdr_size) ||
        size >= fa_p->fa_size) {
        return BOOT_EBADIMAGE;
    }

    return 0;
}

#ifdef MCUBOOT_VALIDATE_PRIMARY_SLOT
/**
 * Validate hash of a primary boot image.
 *
 * @param[in]	fa_p	flash area pointer
 * @param[in]	hdr	boot image header pointer
 *
 * @return		FIH_SUCCESS on success, error code otherwise
 */
inline static fih_int
boot_image_validate(const struct flash_area *fa_p,
                    struct image_header *hdr)
{
    static uint8_t tmpbuf[BOOT_TMPBUF_SZ];
    fih_int fih_rc = FIH_FAILURE;

    /* NOTE: The first argument to boot_image_validate, for enc_state pointer,
     * is allowed to be NULL only because the single image loader compiles
     * with BOOT_IMAGE_NUMBER == 1, which excludes the code that uses
     * the pointer from compilation.
     */
    /* Validate hash */
    FIH_CALL(bootutil_img_validate, fih_rc, NULL, 0, hdr, fa_p, tmpbuf,
             BOOT_TMPBUF_SZ, NULL, 0, NULL);

    FIH_RET(fih_rc);
}
#endif /* MCUBOOT_VALIDATE_PRIMARY_SLOT */
/**
 * @brief check need update or not
 *
 * check update or not, check hdr only
 *
 * @param[in]  src src flash area
 * @param[in]  dst dst flash area
 *
 * @return  true if need update
 */
bool check_update_valid(const struct flash_area * src, const struct flash_area * dst)
{
    if ((NULL == src) || (NULL == dst)) {
        return false;
    }

    struct image_header _hdr_src = { 0 };
    struct image_header _hdr_dst = { 0 };

    if (boot_image_load_header(src, &_hdr_src)) {  //src is not valid, return false.
        return false;
    }

    if (boot_image_load_header(dst, &_hdr_dst)) { //dst is not valid, src is valid,return true.
        return true;
    }

    // version example: 1.2.55+123456
    if (_hdr_dst.ih_ver.iv_major < _hdr_src.ih_ver.iv_major) { //major adjust
        return true;
    } else if (_hdr_src.ih_ver.iv_major == _hdr_dst.ih_ver.iv_major){
        if (_hdr_dst.ih_ver.iv_minor < _hdr_src.ih_ver.iv_minor) { //minor adjust
            return true;
        } else if (_hdr_dst.ih_ver.iv_minor == _hdr_src.ih_ver.iv_minor) {
            if (_hdr_dst.ih_ver.iv_revision < _hdr_src.ih_ver.iv_revision) { //revision adjust
                return true;
            } else if (_hdr_dst.ih_ver.iv_revision == _hdr_src.ih_ver.iv_revision) {
                if (_hdr_dst.ih_ver.iv_build_num < _hdr_src.ih_ver.iv_build_num) { //build num adjust
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * @brief check img valiable or not
 *
 * img check,check img is ok or not
 *
 * @param[in]  dst dst flash area
 * @parami[out]	hdr	for dst image, on success
 *
 * @return  FIH_SUCCESS if check pass
 */
fih_int check_img_valid(const struct flash_area * dst, struct image_header * hdr)
{
    int rc = -1;
    fih_int fih_rc = FIH_FAILURE;

    if (NULL == dst) {
        goto out;
    }

    rc = boot_image_load_header(dst, hdr);
    if (rc != 0)
        goto out;

#ifdef MCUBOOT_VALIDATE_PRIMARY_SLOT
    FIH_CALL(boot_image_validate, fih_rc, dst, hdr);
    if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
        goto out;
    }
#else
    fih_rc = FIH_SUCCESS;
#endif /* MCUBOOT_VALIDATE_PRIMARY_SLOT */

out:
    FIH_RET(fih_rc);
}
#pragma GCC push_options
#pragma GCC optimize ("Og")
/**
 * @brief img update process
 *
 * @param[in]  src src flash area
 * @param[in]  src_hdr src hdr info
 * @param[in]  dst dst flash area
 *
 * @return  FIH_SUCCESS if check pass
 */                            
fih_int img_update_process(const struct flash_area * src, const struct image_header * src_hdr ,
                            const struct flash_area * dst)
{
    if ( (NULL == src) || (NULL == src_hdr) || (NULL == dst) ) {
        return FIH_FAILURE;
    }
    //step 1: calc total size for update from src img
    size_t total_size = 0;
    struct image_tlv_info info;
    int rc;

    uint32_t off_ = BOOT_TLV_OFF(src_hdr);
    if (LOAD_IMAGE_DATA(src_hdr, src, off_, &info, sizeof(info))) {
        return FIH_FAILURE;
    }

    if (info.it_magic == IMAGE_TLV_PROT_INFO_MAGIC) {
        if (src_hdr->ih_protect_tlv_size != info.it_tlv_tot) {
            return FIH_FAILURE;
        }

        if (LOAD_IMAGE_DATA(hdr, src, off_ + info.it_tlv_tot,
                            &info, sizeof(info))) {
            return FIH_FAILURE;
        }
    } else if (src_hdr->ih_protect_tlv_size != 0) {
        return FIH_FAILURE;
    }

    total_size = src_hdr->ih_hdr_size + src_hdr->ih_img_size 
                    + info.it_tlv_tot;  //calc total size: hdr+img+tlv

    //step 2: eras dst flash, with total size by step 1

    if ( flash_area_erase(dst, 0, dst->fa_size) ) {
        return FIH_FAILURE;
    }

    //step 3: read img from src, then writ it to dst. finally read back 
    //        for check writed is ok or not.
    off_t off_from_img = 0;
    size_t remain_size = total_size;
    do {
        static char buf_write[1024];
        static char buf_read[1024];
        rc = flash_area_read(src, off_from_img, buf_write, sizeof(buf_write));
        if (rc != 0) {
            return FIH_FAILURE;
        }

        rc = flash_area_write(dst, off_from_img, buf_write, sizeof(buf_write));
        if (rc != 0) {
            return FIH_FAILURE;
        }

        //read back to check!
        rc = flash_area_read(dst, off_from_img, buf_read, sizeof(buf_read));
        if (rc != 0) {
            return FIH_FAILURE;
        }

        if (memcmp(buf_write, buf_read, sizeof(buf_read)))
        {
            //BOOT_LOG_ERR("write flash error!\n");
            return FIH_FAILURE;
        }

        if (remain_size > sizeof(buf_write)) {
            remain_size -= sizeof(buf_write);
        } else {
            remain_size = 0;
        }

        uint8_t percent = 100 - remain_size*100/total_size;
        send_event_to_display(EVENT_STATUS_UPDATING, percent);

        k_sleep(K_MSEC(20));
        
        off_from_img += sizeof(buf_write);
    } while (remain_size);

    return FIH_SUCCESS;
}
#pragma GCC pop_options
