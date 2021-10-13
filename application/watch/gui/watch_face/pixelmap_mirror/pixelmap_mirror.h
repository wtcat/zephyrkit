#include <stdint.h>
#ifndef __PIXEL_MAP_MIRROR_H__
#define __PIXEL_MAP_MIRROR_H__
#include "gx_api.h"

typedef struct __mirror_obj {
	GX_PIXELMAP map;
	GX_CANVAS canvas;
	uint16_t obj_width;
	uint16_t obj_height;
	uint8_t in_use;
} mirror_obj_t;

void mirror_obj_init(void);

int mirror_obj_create(mirror_obj_t **obj, uint16_t width, uint16_t height,
					  uint8_t color_bytes);

int mirror_obj_create_copy(mirror_obj_t **obj, const GX_CANVAS *canvas_src);

void mirror_obj_delete(mirror_obj_t **obj);

int mirror_obj_copy(GX_CANVAS *dst, GX_CANVAS *src, uint16_t offset);

#endif