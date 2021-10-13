#ifndef __WATCH_FACE_MANAGER_H__
#define __WATCH_FACE_MANAGER_H__
#include "errno.h"
#include "gx_api.h"
#include "stdint.h"
#include "storage/flash_map.h"
#include "sys/slist.h"
#include "watch_face_theme_1_conf.h"

#define WF_THEME_MAX_CNTS 5

#define WF_MAGIC_NUM 0x867435DF

typedef struct _theme_node {
	sys_snode_t node;
	uint32_t offset_of_next_node;
	uint8_t partition_id;
	uint8_t theme_ext_or_not; // theme store in ext flash or not.
} theme_node_t;

typedef struct _slist_offse {
	uint32_t head_offset;
	uint32_t tail_offset;
} slist_offset_t;

typedef struct _watch_face_manager {
	uint32_t magic_num;
	uint8_t cnts;
	uint8_t curr_theme_index;
	uint8_t curr_write_index_of_array;
	uint8_t reserved;
	slist_offset_t slist_offset;
	sys_slist_t slist;
	theme_node_t theme_nodes[WF_THEME_MAX_CNTS];
} watch_face_manager_t;

// void watch_face_mgr_init(void);
void watch_face_mgr_init(void *map_base);

int watch_face_mgr_cnts_get(void);

int watch_face_mgr_theme_select(uint8_t theme_id);

int watch_face_mgr_theme_thumb_get(uint8_t theme_id, void *addr);

void watch_face_theme_show(GX_WINDOW *widget);

uint8_t watch_face_mgr_get_free_partition(void);

int watch_face_mgr_theme_add(uint8_t partition_id);

int watch_face_mgr_theme_remove(uint8_t theme_id);

#endif
