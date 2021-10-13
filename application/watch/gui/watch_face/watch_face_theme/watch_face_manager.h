#ifndef __WATCH_FACE_MANAGER_H__
#define __WATCH_FACE_MANAGER_H__
#include "custom_binres_theme_load.h"
#include "errno.h"
#include "gx_api.h"
#include "irq.h"
#include "stdint.h"
#include "storage/flash_map.h"
#include "sys/sem.h"
#include "sys/slist.h"
#include "watch_face_theme_1_conf.h"
#include "watch_face_theme_customize.h"
#include "watch_face_theme_draw.h"
#include <devicetree.h>
#include <errno.h>
#include <kernel.h>
#include <settings/settings.h>
#include <stdint.h>

#define WF_THEME_MAX_CNTS 5

#define WF_MAGIC_NUM 0x867435DF

#define WF_THEME_STORE_IN_INT 0
#define WF_THEME_STORE_IN_EXT 1

#define WF_THEME_DEFAULT_ID 0

#define WF_MIN_PARTITION_ID FLASH_AREA_ID(watch_face_1)
#define WF_MAX_PARTITION_ID FLASH_AREA_ID(watch_face_5)

typedef struct _theme_node {
	char theme_name[16];
	uint32_t uniqueID;
	uint8_t partition_id;
	uint8_t theme_ext_or_not; // theme store in ext flash or not.
	char reserved[2];
	GX_PIXELMAP thumb_info; // need preparse partition info [size = 28]
} theme_node_t;

typedef struct _watch_face_manager {
	uint32_t magic_num;
	uint8_t cnts;
	uint8_t curr_theme_index;
	uint8_t curr_write_partition_index;
	theme_node_t theme_nodes[WF_THEME_MAX_CNTS];
} watch_face_manager_t;

typedef struct {
	USHORT pixelmap_table_size;
	GX_PIXELMAP **pixelmap_table;
	USHORT color_table_size;
	GX_COLOR *color_table;
	theme_node_t *theme_node;
	void *theme_buff_addr;
} theme_info_t;

// void watch_face_mgr_init(void);
// void watch_face_mgr_init(void * map_base);
void wf_mgr_init(struct guix_driver *drv);

uint8_t wf_mgr_cnts_get(void);

uint8_t wf_mgr_get_default_id(void);

// int wf_mgr_theme_select(uint8_t theme_id);
int wf_mgr_theme_select(uint8_t theme_id, GX_WIDGET *widget);

int wf_mgr_theme_thumb_get(uint8_t theme_id, GX_PIXELMAP **map);

int wf_mgr_theme_name_get(uint8_t theme_id, char **name, uint8_t length_max);

void wf_theme_show(GX_WINDOW *widget);

int wf_mgr_get_free_partition(uint8_t *partition_id);

int wf_mgr_theme_add(uint8_t partition_id);

int wf_mgr_theme_remove(uint8_t theme_id);

int wf_mgr_theme_style_edit(uint32_t theme_sequence_id, uint8_t element_id,
							uint8_t type, uint32_t value);

#endif
