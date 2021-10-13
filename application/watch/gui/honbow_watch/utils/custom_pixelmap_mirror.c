#include "custom_pixelmap_mirror.h"
#include "gx_api.h"
#include "gx_widget.h"
#include "sys/printk.h"
#include <errno.h>
#include <kernel.h>
#include <stdint.h>
#include "section_tags.h"

#define USE_MIRROR 0

#if USE_MIRROR
struct k_mem_slab mirror_buff_pool;
__psram_noinit static uint8_t __aligned(sizeof(void *)) mirror_buff_pool_buf[259200];

static K_MEM_SLAB_DEFINE(mirror_obj_pool, sizeof(mirror_obj_t), 4, 4);
#endif

void mirror_obj_init(void)
{
// do nothing
#if USE_MIRROR
	k_mem_slab_init(&mirror_buff_pool, mirror_buff_pool_buf, sizeof(mirror_buff_pool_buf), 1);
#endif
}

int mirror_obj_create(mirror_obj_t **obj, uint16_t width, uint16_t height, uint8_t color_bytes)
{
#if USE_MIRROR
	int err;
	GX_UBYTE *buff;

	err = k_mem_slab_alloc(&mirror_obj_pool, (void **)obj, K_NO_WAIT);

	if (err) {
		return -ENOSPC;
	}

	err = k_mem_slab_alloc(&mirror_buff_pool, (void **)&buff, K_NO_WAIT);

	if (!err) {
		memset(&(*obj)->map, 0, sizeof(GX_PIXELMAP));

		(*obj)->map.gx_pixelmap_data = (GX_UBYTE *)buff;
		(*obj)->map.gx_pixelmap_data_size = width * height * color_bytes;
		(*obj)->map.gx_pixelmap_height = height;
		(*obj)->map.gx_pixelmap_width = width;
		(*obj)->map.gx_pixelmap_format = GX_COLOR_FORMAT_565BGR;

		memset(&(*obj)->canvas, 0, sizeof(GX_CANVAS));
		/* Create a canvas for drawing mirror. */
		gx_canvas_create(&(*obj)->canvas, "mirror", NULL, GX_CANVAS_SIMPLE, width, height,
						 (GX_COLOR *)(*obj)->map.gx_pixelmap_data, (*obj)->map.gx_pixelmap_data_size);
		(*obj)->in_use = 1;
	} else {
		k_mem_slab_free(&mirror_obj_pool, (void **)&buff);
		return -ENOSPC;
	}
	return 0;
#else
	*obj = NULL;
	return -ENOSPC;
#endif
}

int mirror_obj_create_copy(mirror_obj_t **obj, const GX_CANVAS *canvas_src)
{
#if USE_MIRROR
	int err;
	GX_UBYTE *buff;

	err = k_mem_slab_alloc(&mirror_obj_pool, (void **)obj, K_NO_WAIT);

	if (err) {
		return -ENOSPC;
	}

	err = k_mem_slab_alloc(&mirror_buff_pool, (void **)&buff, K_NO_WAIT);

	if (!err) {
		memset(&(*obj)->map, 0, sizeof(GX_PIXELMAP));

		GX_VALUE height = canvas_src->gx_canvas_display->gx_display_height;
		GX_VALUE width = canvas_src->gx_canvas_display->gx_display_width;
		uint8_t color_bytes;

		if (canvas_src->gx_canvas_display->gx_display_color_format == GX_COLOR_FORMAT_565RGB) {
			color_bytes = 2;
		} else {
			printk("error!!!! now not support other color format, except 565bgr");
			return -ENOTSUP;
		}

		(*obj)->map.gx_pixelmap_data = (GX_UBYTE *)buff;
		(*obj)->map.gx_pixelmap_data_size = width * height * color_bytes;
		(*obj)->map.gx_pixelmap_height = height;
		(*obj)->map.gx_pixelmap_width = width;
		(*obj)->map.gx_pixelmap_format = canvas_src->gx_canvas_display->gx_display_color_format;

		memset(&(*obj)->canvas, 0, sizeof(GX_CANVAS));
		/* Create a canvas for drawing mirror. */
		gx_canvas_create(&(*obj)->canvas, "mirror", canvas_src->gx_canvas_display, GX_CANVAS_SIMPLE, width, height,
						 (GX_COLOR *)(*obj)->map.gx_pixelmap_data, (*obj)->map.gx_pixelmap_data_size);
		(*obj)->in_use = 1;
	} else {
		k_mem_slab_free(&mirror_obj_pool, (void **)&buff);
		return -ENOSPC;
	}
	return 0;
#else
	*obj = NULL;
	return -ENOSPC;
#endif
}

void mirror_obj_delete(mirror_obj_t **obj)
{
#if USE_MIRROR
	if (*obj != NULL) {
		gx_canvas_delete(&(*obj)->canvas);
		k_mem_slab_free(&mirror_buff_pool, (void **)&(*obj)->map.gx_pixelmap_data);
		k_mem_slab_free(&mirror_obj_pool, (void **)obj);
		(*obj)->in_use = 0;
		*obj = NULL;
	}
#endif
	return;
}

// dst: dest canvas, mirror_obj_t's canvas instance
// src: src canvas, usaually, we can set it to default canvas of the display.
//     or you can draw the canvas of mirror obj by your own draw func.
int mirror_obj_copy(GX_CANVAS *dst, GX_CANVAS *src, uint16_t offset)
{
#if USE_MIRROR
	if ((dst == NULL) || (src == NULL)) {
		return -EINVAL;
	}

	memcpy((char *)dst->gx_canvas_memory, (char *)src->gx_canvas_memory + offset, dst->gx_canvas_memory_size);
#endif
	return 0;
}
