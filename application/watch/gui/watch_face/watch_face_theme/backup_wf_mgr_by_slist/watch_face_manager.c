#include "watch_face_manager.h"
#include "gx_api.h"
#include "irq.h"
#include "sys/slist.h"
#include <devicetree.h>
#include <errno.h>
#include <kernel.h>
#include <stdint.h>

watch_face_manager_t wf_mgr;

static USHORT watch_face_theme_pixelmap_table_size;
static GX_PIXELMAP **watch_face_theme_curr_pixelmap_table;
static USHORT watch_face_theme_color_table_size;
static GX_COLOR *watch_face_theme_curr_color_table;

theme_node_t *curr_theme_node;

static void *ext_flash_map_base = NULL;

#pragma GCC push_options
#pragma GCC optimize("O0")

static void watch_face_mgr_default(void)
{
	wf_mgr.cnts = 1;
	wf_mgr.magic_num = WF_MAGIC_NUM;
	wf_mgr.curr_theme_index = 0;
	wf_mgr.curr_write_index_of_array = 0;

#if 0
    wf_mgr.theme_nodes[0].partition_id = FLASH_AREA_ID(watch_face_1);
    wf_mgr.theme_nodes[0].theme_ext_or_not = 1;
    wf_mgr.theme_nodes[0].node.next = NULL;
    wf_mgr.theme_nodes[0].offset_of_next_node = 0;
#else
	wf_mgr.theme_nodes[0].partition_id = 0;
	wf_mgr.theme_nodes[0].theme_ext_or_not = 0; // internal theme for test!
	wf_mgr.theme_nodes[0].node.next = NULL;
#endif

	wf_mgr.slist.head = (sys_snode_t *)&wf_mgr.theme_nodes[0];
	wf_mgr.slist.tail = (sys_snode_t *)&wf_mgr.theme_nodes[0];

	wf_mgr.slist_offset.head_offset =
		(char *)&wf_mgr.theme_nodes[0] - (char *)&wf_mgr;
	wf_mgr.slist_offset.tail_offset =
		(char *)&wf_mgr.theme_nodes[0] - (char *)&wf_mgr;
}

static void wf_theme_node_relocate(void)
{
	wf_mgr.slist.head =
		(sys_snode_t *)(wf_mgr.slist_offset.head_offset + (char *)&wf_mgr);
	wf_mgr.slist.tail =
		(sys_snode_t *)(wf_mgr.slist_offset.tail_offset + (char *)&wf_mgr);

	theme_node_t *curr = (theme_node_t *)wf_mgr.slist.head;
	do {
		if (curr->offset_of_next_node) {
			curr->node.next =
				(sys_snode_t *)(curr->offset_of_next_node + (char *)&wf_mgr);
		} else {
			return;
		}
	} while (1);
}

void watch_face_mgr_init(void *map_base)
{
	ext_flash_map_base = map_base;
	const struct flash_area *_fa_p_ext_flash;
	int rc =
		flash_area_open(FLASH_AREA_ID(watch_face_manager), &_fa_p_ext_flash);
	if (rc) // should never happen
		return;

	flash_area_read(_fa_p_ext_flash, 0, (void *)&wf_mgr,
					sizeof(watch_face_manager_t));
	if (wf_mgr.magic_num != WF_MAGIC_NUM) {
		watch_face_mgr_default();
		flash_area_erase(_fa_p_ext_flash, 0,
						 FLASH_AREA_SIZE(watch_face_manager));
		flash_area_write(_fa_p_ext_flash, 0, (void *)&wf_mgr,
						 sizeof(watch_face_manager_t));
	}

	wf_theme_node_relocate();

	if (0 != watch_face_mgr_theme_select(wf_mgr.curr_theme_index)) {
		watch_face_mgr_theme_select(0); // select default theme
	}
}

int watch_face_mgr_cnts_get(void)
{
	return wf_mgr.cnts;
}

static theme_node_t *watch_face_theme_get_node(uint8_t index)
{
	uint8_t index_tmp = 0;
	theme_node_t *curr = (theme_node_t *)sys_slist_peek_head(&wf_mgr.slist);

	if (curr == NULL) {
		return NULL;
	}

	do {
		if (index_tmp == index) {
			return (theme_node_t *)&curr->node;
		} else {
			if (curr->node.next != NULL) {
				curr = (theme_node_t *)curr->node.next;
				index_tmp++;
			} else { // find error! return head node
				return (theme_node_t *)sys_slist_peek_head(&wf_mgr.slist);
			}
		}
	} while (1);

	// should never happen.
	curr = (theme_node_t *)sys_slist_peek_head(&wf_mgr.slist);

	return curr;
}

#include "custom_binres_theme_load.h"
extern struct watch_face_theme1_ctl watch_face_theme_1;

int watch_face_mgr_theme_select(uint8_t theme_id)
{
	// reset pixelmap table and color table
	watch_face_theme_curr_pixelmap_table = NULL;
	watch_face_theme_pixelmap_table_size = 0;
	watch_face_theme_curr_color_table = NULL;
	watch_face_theme_color_table_size = 0;

	custom_binres_theme_unload(); // free old theme info

	if (theme_id > wf_mgr.cnts - 1) {
		return -EINVAL;
	} else {
		theme_node_t *node = watch_face_theme_get_node(theme_id);

		if (node == NULL) {
			return -EFAULT;
		}

		if (0 != node->theme_ext_or_not) {
			const struct flash_area *_fa_p_ext_flash;
			int rc = flash_area_open(node->partition_id, &_fa_p_ext_flash);
			if (0 == rc) {
				void *addr = ext_flash_map_base;

				uint32_t offset = _fa_p_ext_flash->fa_off;

				if (watch_face_theme_init((unsigned char *)addr + offset)) {
					return -EFAULT;
				}

				offset += watch_face_theme_hdr_size_get();

				GX_THEME *theme;
				unsigned int ret =
					custom_binres_theme_load((unsigned char *)addr + offset, 0,
											 &theme); // TODO: memory leak
				if (ret != GX_SUCCESS) { // we should remove it from list?
					watch_face_mgr_theme_remove(theme_id);
					return -EFAULT;
				}

				watch_face_theme_curr_pixelmap_table =
					theme->theme_pixelmap_table;
				watch_face_theme_pixelmap_table_size =
					theme->theme_pixelmap_table_size;

				watch_face_theme_curr_color_table = theme->theme_color_table;
				watch_face_theme_color_table_size =
					theme->theme_color_table_size;
			} else {
				return -EFAULT;
			}
		} else {
			watch_face_theme_init((void *)&watch_face_theme_1);
		}
		wf_mgr.curr_theme_index = theme_id;

		curr_theme_node = node;
	}
	return 0;
}

// return partition id for write, if return 0xff,ext flash is full!
uint8_t watch_face_mgr_get_free_partition(void)
{
	if (wf_mgr.cnts >= WF_THEME_MAX_CNTS) {
		return 0xff;
	}

	uint8_t write_index = wf_mgr.curr_write_index_of_array + 1;
	uint8_t partition_id = 0xff;

	if (write_index >= WF_THEME_MAX_CNTS) {
		write_index = 0;
	}

	do {
		switch (write_index) {
		case 0:
			partition_id = FLASH_AREA_ID(watch_face_1);
			break;
		case 1:
			partition_id = FLASH_AREA_ID(watch_face_2);
			break;
		case 2:
			partition_id = FLASH_AREA_ID(watch_face_3);
			break;
		case 3:
			partition_id = FLASH_AREA_ID(watch_face_4);
			break;
		case 4:
			partition_id = FLASH_AREA_ID(watch_face_5);
			break;
		default:
			break;
		}

		theme_node_t *curr = (theme_node_t *)sys_slist_peek_head(&wf_mgr.slist);
		while (1) {
			if (curr->partition_id == partition_id) {
				break;
			} else {
				if (NULL != curr->node.next) {
					curr = (theme_node_t *)curr->node.next;
				} else {
					return partition_id;
				}
			}
		}

		write_index++;
		if (write_index >= 5) {
			write_index = 0;
		}

	} while (1);

	return 0xff;
}

// return partition id,
int watch_face_mgr_theme_add(uint8_t partition_id)
{
	// TODO: we should check partition is a valiable theme??
	switch (partition_id) {
	case FLASH_AREA_ID(watch_face_1):
		wf_mgr.curr_write_index_of_array = 0;
		break;

	case FLASH_AREA_ID(watch_face_2):
		wf_mgr.curr_write_index_of_array = 1;
		break;

	case FLASH_AREA_ID(watch_face_3):
		wf_mgr.curr_write_index_of_array = 2;
		break;

	case FLASH_AREA_ID(watch_face_4):
		wf_mgr.curr_write_index_of_array = 3;
		break;

	case FLASH_AREA_ID(watch_face_5):
		wf_mgr.curr_write_index_of_array = 4;
		break;

	default:
		return -EINVAL;
	}

	if (wf_mgr.cnts >= WF_THEME_MAX_CNTS) {
		return -ENOSPC;
	}

	wf_mgr.cnts += 1;

	uint8_t write_index = wf_mgr.curr_write_index_of_array;

	wf_mgr.theme_nodes[write_index].partition_id = partition_id;
	wf_mgr.theme_nodes[write_index].theme_ext_or_not =
		(partition_id == 0xff) ? 0 : 1;

	// calc offset of snode from wf_mgr.
	uint32_t offset =
		(char *)&wf_mgr.theme_nodes[write_index] - (char *)&wf_mgr;
	if (sys_slist_peek_tail(&wf_mgr.slist)) {
		z_slist_head_set(&wf_mgr.slist,
						 (sys_snode_t *)&wf_mgr.theme_nodes[write_index]);
		wf_mgr.slist_offset.head_offset = offset;

		z_slist_tail_set(&wf_mgr.slist,
						 (sys_snode_t *)&wf_mgr.theme_nodes[write_index]);
		wf_mgr.slist_offset.tail_offset = offset;

	} else {
		theme_node_t *tail = (theme_node_t *)sys_slist_peek_tail(&wf_mgr.slist);
		z_snode_next_set((sys_snode_t *)tail,
						 (sys_snode_t *)&wf_mgr.theme_nodes[write_index]);
		tail->offset_of_next_node = offset;

		z_slist_tail_set(&wf_mgr.slist,
						 (sys_snode_t *)&wf_mgr.theme_nodes[write_index]);
		wf_mgr.slist_offset.tail_offset = offset;
	}

	// we should write wf_mgr info to manager partition at once!
	const struct flash_area *_fa_p_ext_flash;
	int rc =
		flash_area_open(FLASH_AREA_ID(watch_face_manager), &_fa_p_ext_flash);
	if (0 == rc) {
		flash_area_erase(_fa_p_ext_flash, 0,
						 FLASH_AREA_SIZE(watch_face_manager));
		flash_area_write(_fa_p_ext_flash, 0, (void *)&wf_mgr,
						 sizeof(watch_face_manager_t));
	}

	return 0;
}

// remove from wf_mgr only, theme's partition will erase when add new theme
int watch_face_mgr_theme_remove(uint8_t theme_id)
{
	uint8_t remove_ok = 0;

	if (theme_id >= 5) {
		return -EFAULT;
	}

	sys_snode_t *node = (sys_snode_t *)watch_face_theme_get_node(theme_id);

	sys_snode_t *prev = NULL;
	sys_snode_t *test;

	for (test = sys_slist_peek_head(&wf_mgr.slist); test != NULL;
		 test = sys_slist_peek_next(test)) {
		if (test == node) {

			if (prev == NULL) {
				z_slist_head_set(&wf_mgr.slist, z_snode_next_peek(node));
				wf_mgr.slist_offset.head_offset =
					((theme_node_t *)node)->offset_of_next_node;

				/* Was node also the tail? */
				if (sys_slist_peek_tail(&wf_mgr.slist) == node) {
					z_slist_tail_set(&wf_mgr.slist,
									 sys_slist_peek_head(&wf_mgr.slist));
					wf_mgr.slist_offset.tail_offset =
						((theme_node_t *)node)->offset_of_next_node;
					;
				}
			} else {
				z_snode_next_set(prev, z_snode_next_peek(node));
				((theme_node_t *)prev)->offset_of_next_node =
					((theme_node_t *)node)->offset_of_next_node;

				/* Was node the tail? */
				if (sys_slist_peek_tail(&wf_mgr.slist) == node) {
					z_slist_tail_set(&wf_mgr.slist, prev);
					wf_mgr.slist_offset.tail_offset =
						(char *)prev - (char *)&wf_mgr;
				}
			}
			z_snode_next_set(node, NULL);
			((theme_node_t *)node)->offset_of_next_node = 0;
			remove_ok = 1;
			break;
		}
		prev = test;
	}

	if (remove_ok) {
		const struct flash_area *_fa_p_ext_flash;
		int rc = flash_area_open(FLASH_AREA_ID(watch_face_manager),
								 &_fa_p_ext_flash);
		if (0 == rc) {
			flash_area_erase(_fa_p_ext_flash, 0,
							 FLASH_AREA_SIZE(watch_face_manager));
			flash_area_write(_fa_p_ext_flash, 0, (void *)&wf_mgr,
							 sizeof(watch_face_manager_t));
			return 0;
		}
	}
	return EFAULT;
}
#pragma GCC pop_options

// TODO: need impement
int watch_face_mgr_theme_thumb_get(uint8_t theme_id, void *addr)
{
	if (theme_id >= 5) {
		return -EFAULT;
	}
	return 0;
}

// get GX_PIXELMAP from current theme.
int _watch_face_pixelmap_get(uint32_t index, void **addr)
{
	if (NULL == curr_theme_node) {
		*addr = NULL;
		return -EFAULT;
	}

	if (0 == curr_theme_node->theme_ext_or_not) { // internal theme
		return gx_context_pixelmap_get(index, (GX_PIXELMAP **)addr);
	} else { // load pixelmap from ext flash
		if (index < watch_face_theme_pixelmap_table_size) {
			*addr = watch_face_theme_curr_pixelmap_table[index];
		} else {
			*addr = NULL;
		}
		return 0;
	}
}

// get GX_COLOR from current theme.
int _watch_face_color_get(uint32_t index, void *addr)
{
	if (NULL == curr_theme_node) {
		*(GX_COLOR *)addr = 0;
		return -EFAULT;
	}

	if (0 == curr_theme_node->theme_ext_or_not) { // internal theme
		return gx_context_color_get(index, addr);
	} else { // load pixelmap from ext flash
		if (index < watch_face_theme_color_table_size) {
			*(GX_COLOR *)addr = watch_face_theme_curr_color_table[index];
		} else {
			*(GX_COLOR *)addr = 0;
		}
		return 0;
	}
}

extern void _watch_face_draw_main(GX_WINDOW *widget);
void watch_face_theme_show(GX_WINDOW *widget)
{
	_watch_face_draw_main(widget);
}
