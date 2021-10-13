#include "watch_face_theme_customize.h"
#include "sys/sem.h"
#include <settings/settings.h>

static int wf_cfg_direct_loader(const char *name, size_t len,
								settings_read_cb read_cb, void *cb_arg,
								void *param)
{
	const char *next;
	size_t name_len;
	int rc;
	wf_mgr_custom_cfg_t *dst = (wf_mgr_custom_cfg_t *)param;

	name_len = settings_name_next(name, &next);

	if (name_len == 0) {
		rc = read_cb(cb_arg, (void *)dst, sizeof(wf_mgr_custom_cfg_t));
		return 0;
	}

	return -ENOENT;
}

int wft_custom_setting_load(uint32_t theme_sequence_id,
							wf_mgr_custom_cfg_t *data)
{
#if CONFIG_SETTINGS
	char string[30];
	snprintf(string, sizeof(string), "setting/wf_%08d_cfg", theme_sequence_id);
	int rc = settings_load_subtree_direct(string, wf_cfg_direct_loader,
										  (void *)data);
	return rc;
#else
	return -ENOTSUP;
#endif
}

int wft_custom_setting_save(uint32_t theme_sequence_id,
							wf_mgr_custom_cfg_t *data)
{
#if CONFIG_SETTINGS
	char string[30];
	snprintf(string, sizeof(string), "setting/wf_%08d_cfg", theme_sequence_id);
	int rc = settings_save_one(string, data, sizeof(wf_mgr_custom_cfg_t));
	return rc;
#else
	return -ENOTSUP;
#endif
}
