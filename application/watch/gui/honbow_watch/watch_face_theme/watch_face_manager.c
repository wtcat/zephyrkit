#include "watch_face_manager.h"
#include "custom_binres_theme_load.h"
#include "errno.h"
#include "storage/flash_map.h"
#include "sys/sem.h"
#include "watch_face_theme_1_conf.h"
#include "watch_face_theme_customize.h"
#include <settings/settings.h>

static watch_face_manager_t wf_mgr;
static theme_info_t using_theme = {
	.pixelmap_table_size = 0,
	.color_table_size = 0,
	.theme_buff_addr = NULL,
	.node = NULL,
};
static struct guix_driver *drv_guix = NULL;

static struct sys_sem sem_lock;
#define SYNC_INIT() sys_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() sys_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() sys_sem_give(&sem_lock)

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
static int wf_mgr_data_save(void)
{
#if CONFIG_SETTINGS
	return settings_save_one("setting/watchface/watchfaceMgr", (void *)&wf_mgr, sizeof(watch_face_manager_t));
#else
	return -ENOTSUP;
#endif
}

static int wf_mgr_cfg_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int rc;
	if (settings_name_steq(name, "watchfaceMgr", &next) && !next) {
		if (len != sizeof(watch_face_manager_t)) {
			return -EINVAL;
		}
		rc = read_cb(cb_arg, (void *)&wf_mgr, sizeof(watch_face_manager_t));
		if (rc >= 0) {
			return 0;
		}
		return rc;
	}
	return -ENOENT;
}

struct settings_handler wf_conf = {.name = "setting/watchface", .h_set = wf_mgr_cfg_set};

static int wf_mgr_data_load(void)
{
#if CONFIG_SETTINGS
	int ret = settings_subsys_init();
	if (ret != 0) {
		int rc;
		const struct flash_area *_fa_p_ext_flash;
		rc = flash_area_open(FLASH_AREA_ID(storage), &_fa_p_ext_flash);
		if (!rc) {
			if (!flash_area_erase(_fa_p_ext_flash, 0, _fa_p_ext_flash->fa_size)) {
				settings_subsys_init();
			}
		}
	}
	settings_register(&wf_conf);
	ret = settings_load_subtree("setting/watchface");
	return ret;
#else
	return -ENOTSUP;
#endif
}

extern struct watch_face_theme1_ctl watch_face_theme_1;
static void watch_face_mgr_default(void)
{
	wf_mgr.cnts = 0;
	wf_mgr.magic_num = WF_MAGIC_NUM;
	wf_mgr.curr_theme_index = 0;
#if CONFIG_SETTINGS
	wf_mgr.curr_write_partition_index = FLASH_AREA_ID(watch_face_1);
#else
#endif

#if 0
	wf_mgr_theme_add(FLASH_AREA_ID(watch_face_1));
#else
	struct watch_face_header *head = &watch_face_theme_1.head;
	theme_node_t *node = &wf_mgr.theme_nodes[0];
	node->uniqueID = head->wfh_uniqueID;
	node->partition_id = 0xff;
	node->theme_ext_or_not = WF_THEME_STORE_IN_INT; // internal
	memcpy(node->theme_name, head->name, sizeof(node->theme_name));

	// internal theme, no need get thumb info earlier.
	// node->thumb_info;

	wf_mgr.cnts = 1;
#endif
}

void wf_mgr_init(struct guix_driver *drv)
{
	SYNC_INIT();
	wft_init();
	wf_port_init();
	SYNC_LOCK();

	drv_guix = drv;
	wf_mgr_data_load();
	if (wf_mgr.magic_num != WF_MAGIC_NUM) {
		watch_face_mgr_default();
		wf_mgr_data_save();
	}

	SYNC_UNLOCK();
}

uint8_t wf_mgr_cnts_get(void)
{
	SYNC_LOCK();
	uint8_t cnts = wf_mgr.cnts;
	SYNC_UNLOCK();
	return cnts;
}

// used return true,else return false.
static bool wft_partition_used(uint8_t partition_id)
{
	uint8_t node_id;

	for (node_id = 0; node_id < wf_mgr.cnts; node_id++) {
		if (wf_mgr.theme_nodes[node_id].partition_id == partition_id) {
			return true;
		}
	}

	return false;
}

static theme_node_t *wft_get_node(uint8_t index)
{
	return &wf_mgr.theme_nodes[index];
}

static int parse_theme_info(uint8_t partition_id, GX_THEME **theme_parsed, struct watch_face_header *wf_head,
							void **theme_alloc_addr)
{
	const struct flash_area *_fa_p_ext_flash;
	if (flash_area_open(partition_id, &_fa_p_ext_flash)) {
		return -EINVAL;
	}

	void *addr = drv_guix->dev->mmap(~0u);
	if (addr == INVALID_MAP_ADDR) {
		return -EINVAL;
	}

	uint32_t offset = _fa_p_ext_flash->fa_off;
	if (wft_head_parse((unsigned char *)addr + offset, wf_head)) {
		drv_guix->dev->unmap(addr);
		return -EINVAL;
	}

	offset += wft_hdr_size_get();

	GX_THEME *theme;
	unsigned int ret = custom_binres_theme_load((unsigned char *)addr + offset, 0, &theme, theme_alloc_addr);

	drv_guix->dev->unmap(addr);

	if (ret != GX_SUCCESS) {
		return -EINVAL;
	} else {
		*theme_parsed = theme;
		return 0;
	}
}

static int wf_mgr_theme_active(theme_node_t *node, GX_WIDGET *widget)
{
	int rc;
	if (node->theme_ext_or_not) {
		uint8_t partition_id = node->partition_id;
		const struct flash_area *_fa_p_ext_flash;
		if (flash_area_open(partition_id, &_fa_p_ext_flash)) {
			return -EINVAL;
		}

		void *addr = drv_guix->dev->mmap(~0u);
		if (addr == INVALID_MAP_ADDR) {
			return -EINVAL;
		}

		uint32_t offset = _fa_p_ext_flash->fa_off;
		rc = wft_active((unsigned char *)addr + offset, widget);
		drv_guix->dev->unmap(addr);
	} else {
		rc = wft_active((void *)&watch_face_theme_1, widget);
	}

	return rc;
}

uint8_t wf_mgr_get_default_id(void)
{
	return wf_mgr.curr_theme_index;
}

// if theme_id select fail,it will keep old select.
int wf_mgr_theme_select(uint8_t theme_id, GX_WIDGET *widget)
{
	SYNC_LOCK();
	if (theme_id > wf_mgr.cnts - 1) {
		SYNC_UNLOCK();
		return -EINVAL;
	}
	void *theme_buff_addr_backup = NULL;
	if (using_theme.node != NULL) {
		if (wft_get_node(theme_id) == using_theme.node) {
			SYNC_UNLOCK();
			return 0;
		} else {
			if (using_theme.node->theme_ext_or_not == WF_THEME_STORE_IN_EXT) {
				theme_buff_addr_backup = using_theme.theme_buff_addr;
			}
		}
	}
#if 0
	// reset pixelmap table and color table
	curr_theme_info.pixelmap_table = NULL;
	curr_theme_info.pixelmap_table_size = 0;
	curr_theme_info.color_table = NULL;
	curr_theme_info.color_table_size = 0;
	custom_binres_theme_unload(
		&curr_theme_info.theme_buff_addr); // free old theme info
#endif
	theme_node_t *node = wft_get_node(theme_id);
	if (WF_THEME_STORE_IN_EXT == node->theme_ext_or_not) {
		GX_THEME *theme;
		struct watch_face_header wf_head;
		if (0 == parse_theme_info(node->partition_id, &theme, &wf_head, &using_theme.theme_buff_addr)) {
			using_theme.pixelmap_table = theme->theme_pixelmap_table;
			using_theme.pixelmap_table_size = theme->theme_pixelmap_table_size;
			using_theme.color_table = theme->theme_color_table;
			using_theme.color_table_size = theme->theme_color_table_size;
			if (theme_buff_addr_backup != NULL) {
				custom_binres_theme_unload(&theme_buff_addr_backup); // free old
			}
		} else { // should never happen,since it was checked when add.
			SYNC_UNLOCK();
			wf_mgr_theme_remove(theme_id);
			if (using_theme.theme_buff_addr != theme_buff_addr_backup) {
				printk("err! %s:%s[%d]\n", __FILE__, __FUNCTION__, __LINE__);
				using_theme.theme_buff_addr = theme_buff_addr_backup;
			}
			return -EINVAL;
		}
	}
	wf_mgr.curr_theme_index = theme_id;
	using_theme.node = node;
	SYNC_UNLOCK();

	if (!wf_mgr_theme_active(node, widget)) {
		wf_mgr_custom_cfg_t cfg;
		cfg.element_cnts = 0; // must first set to 0
		uint32_t uniqueID = using_theme.node->uniqueID;
		if (!wft_custom_setting_load(uniqueID, &cfg)) {
			for (uint8_t i = 0; i < cfg.element_cnts; i++) {
				struct element_edit_cfg *tmp = &cfg.element_cfg_nodes[i];
				wft_style_sync(tmp->element_id, tmp->edit_type, tmp->value);
			}
		}
		return 0;
	}
	return -EINVAL;
}

// return partition id for write, if return 0xff,ext flash is full!
int wf_mgr_get_free_partition(uint8_t *partition_id)
{
	int ret = 0;
#if CONFIG_FLASH_MAP
	SYNC_LOCK();
	if (wf_mgr.cnts >= WF_THEME_MAX_CNTS) {
		ret = -ENOSPC;
		goto final;
	}

	uint8_t partition_id_test = wf_mgr.curr_write_partition_index < WF_MAX_PARTITION_ID
									? wf_mgr.curr_write_partition_index + 1
									: WF_MIN_PARTITION_ID;

	uint8_t retry_cnts;
	for (retry_cnts = 0; retry_cnts < WF_THEME_MAX_CNTS; retry_cnts++) {
		if (false == wft_partition_used(partition_id_test)) {
			*partition_id = partition_id_test;
			ret = 0;
			goto final;
		}
		if (partition_id_test < WF_MAX_PARTITION_ID) {
			partition_id_test++;
		} else {
			partition_id_test = WF_MIN_PARTITION_ID;
		}
	}
	ret = -ENOSPC;

final:
	SYNC_UNLOCK();
#else
	return -ENOTSUP;
#endif
	return ret;
}

int wf_mgr_theme_add(uint8_t partition_id)
{
	SYNC_LOCK();
	if (wf_mgr.cnts >= WF_THEME_MAX_CNTS) {
		SYNC_UNLOCK();
		return -ENOSPC;
	}

	int ret = 0;
	GX_THEME *theme;
	void *buff_addr = NULL;
	struct watch_face_header wf_head;
	if (!parse_theme_info(partition_id, &theme, &wf_head, &buff_addr)) {
		for (uint8_t i = 0; i < wf_mgr.cnts; i++) {
			theme_node_t *node = &wf_mgr.theme_nodes[i];
			if ((wf_head.wfh_uniqueID == node->uniqueID) || (partition_id == node->partition_id)) {
				ret = -EINVAL; // attempt to install an	 exit theme!
				goto final;
			}
		}
		uint8_t index = wf_mgr.cnts;
		theme_node_t *node = &wf_mgr.theme_nodes[index];
		memcpy(node->theme_name, wf_head.name, sizeof(node->theme_name));
		node->thumb_info = *theme->theme_pixelmap_table[wf_head.wfh_thumb_resource_id];
		node->uniqueID = wf_head.wfh_uniqueID;
		node->partition_id = partition_id;
		node->theme_ext_or_not = WF_THEME_STORE_IN_EXT;
		wf_mgr.curr_write_partition_index = partition_id;
		wf_mgr.cnts += 1;
		wf_mgr_data_save();
	} else {
		ret = -EINVAL;
	}
final:
	if (buff_addr != NULL) {
		custom_binres_theme_unload(&buff_addr);
	}
	SYNC_UNLOCK();
	return ret;
}

// remove from wf_mgr only, theme's partition will erase when add new theme
// if u want remove current useing theme, you should first call
// watch_face_mgr_theme_select to select a new theme, then u can call
// watch_face_mgr_theme_remove to remove it.
int wf_mgr_theme_remove(uint8_t theme_id)
{
	int rc = 0;
	SYNC_LOCK();
	if (theme_id >= wf_mgr.cnts) {
		rc = -EINVAL;
		goto final;
	}

	if (wf_mgr.cnts == 1) { // can not delete
		rc = -EINVAL;
		goto final;
	}

	uint8_t curr_theme_index_old = wf_mgr.curr_theme_index;
	if (curr_theme_index_old > theme_id) {
		curr_theme_index_old -= 1;
	} else if (curr_theme_index_old == theme_id) {
		printk("ERR: remove a using theme!\n");
		rc = -EINVAL;
		goto final;
	}
	wf_mgr.curr_theme_index = curr_theme_index_old;
	for (uint8_t i = theme_id; i < wf_mgr.cnts; i++) {
		wf_mgr.theme_nodes[i] = wf_mgr.theme_nodes[i + 1];
	}
	using_theme.node = &wf_mgr.theme_nodes[wf_mgr.curr_theme_index];
	wf_mgr.cnts -= 1;
	wf_mgr_data_save();
final:
	SYNC_UNLOCK();
	return rc;
}
//#pragma GCC pop_options

// TODO: should called when draw
int wf_mgr_theme_thumb_get(uint8_t theme_id, GX_PIXELMAP **map)
{
	SYNC_LOCK();
	if (theme_id >= WF_THEME_MAX_CNTS) {
		SYNC_UNLOCK();
		return -EINVAL;
	}

	if (wf_mgr.theme_nodes[theme_id].theme_ext_or_not == WF_THEME_STORE_IN_EXT) {
		*map = &wf_mgr.theme_nodes[theme_id].thumb_info;
	} else {
		extern GX_CONST GX_PIXELMAP *honbow_disp_theme_1_pixelmap_table[];
		//*map = honbow_disp_theme_1_pixelmap_table[GX_PIXELMAP_ID_WF_THUMB];
		if (0 == theme_id) {
			gx_context_pixelmap_get(GX_PIXELMAP_ID_WF_THUMB, map);
		}
	}
	SYNC_UNLOCK();
	return 0;
}

int wf_mgr_theme_name_get(uint8_t theme_id, char **name, uint8_t length_max)
{
	SYNC_LOCK();
	if (theme_id >= WF_THEME_MAX_CNTS) {
		SYNC_UNLOCK();
		return -EINVAL;
	}
	*name = wf_mgr.theme_nodes[theme_id].theme_name;
	SYNC_UNLOCK();
	return 0;
}

// get GX_PIXELMAP from current theme.
int wf_mgr_pixelmap_get(uint32_t index, void **addr)
{
	SYNC_LOCK();

	int rc = 0;
	if (NULL == using_theme.node) {
		*addr = NULL;
		rc = -EINVAL;
		goto final;
	}

	if (WF_THEME_STORE_IN_INT == using_theme.node->theme_ext_or_not) { // internal theme
		rc = gx_context_pixelmap_get(index, (GX_PIXELMAP **)addr);
	} else { // load pixelmap from ext flash
		if (index < using_theme.pixelmap_table_size) {
			*addr = using_theme.pixelmap_table[index];
		} else {
			*addr = NULL;
			rc = -EINVAL;
		}
	}
final:
	SYNC_UNLOCK();
	return rc;
}

// get GX_COLOR from current theme.
int wf_mgr_color_get(uint32_t index, void *addr)
{
	SYNC_LOCK();
	int rc = 0;
	if (NULL == using_theme.node) {
		*(GX_COLOR *)addr = 0;
		rc = -EINVAL;
		goto final;
	}

	if (WF_THEME_STORE_IN_INT == using_theme.node->theme_ext_or_not) {
		rc = gx_context_color_get(index, addr);
	} else { // load pixelmap from ext flash
		if (index < using_theme.color_table_size) {
			*(GX_COLOR *)addr = using_theme.color_table[index];
			rc = 0;
		} else {
			*(GX_COLOR *)addr = 0;
			rc = -EINVAL;
		}
	}
final:
	SYNC_UNLOCK();
	return rc;
}

void wf_theme_show(GX_WINDOW *widget)
{
	gx_widget_children_draw(widget);
}

int wf_mgr_theme_style_edit(uint32_t theme_sequence_id, uint8_t element_id, uint8_t type, uint32_t value)
{
	wf_mgr_custom_cfg_t cfg;
	cfg.element_cnts = 0;

	int rc = wft_custom_setting_load(theme_sequence_id, &cfg);

	if (0 == rc) {
		for (uint8_t i = 0; i < cfg.element_cnts; i++) {
			struct element_edit_cfg *tmp = &cfg.element_cfg_nodes[i];
			if ((tmp->element_id == element_id) && (tmp->edit_type == type)) { // find it!
				tmp->value = value;
				if (theme_sequence_id == using_theme.node->uniqueID) {
					wft_style_sync(element_id, type, value);
				}
				wft_custom_setting_save(theme_sequence_id, &cfg);
				return 0;
			}
		}
	} else { // load fail, give a default value
		cfg.element_cnts = 0;
	}

	if (cfg.element_cnts < WF_THEME_CUSTOM_MAX_CNT) { // add it to array
		cfg.element_cfg_nodes[cfg.element_cnts].edit_type = type;
		cfg.element_cfg_nodes[cfg.element_cnts].element_id = element_id;
		cfg.element_cfg_nodes[cfg.element_cnts].value = value;
		cfg.element_cnts += 1;
		if (theme_sequence_id == using_theme.node->uniqueID) {
			wft_style_sync(element_id, type, value);
		}
		wft_custom_setting_save(theme_sequence_id, &cfg);
		return 0;
	}

	return -EINVAL;
}