#ifndef __WATCH_FACE_THEME_CUSTOMIZE_H__
#define __WATCH_FACE_THEME_CUSTOMIZE_H__

#include "stdio.h"
#include "sys/sem.h"
#include "watch_face_theme.h"
#include <settings/settings.h>

#define WF_THEME_CUSTOM_MAX_CNT 20

struct element_edit_cfg {
	uint8_t element_id;
	uint8_t edit_type;
	uint8_t reserved_2[2];
	uint32_t value;
	uint8_t reserved_4[4];
} __aligned(4);

typedef struct wf_mgr_custom_cfg {
	uint8_t element_cnts;
	uint8_t reserved_3[3];
	struct element_edit_cfg element_cfg_nodes[WF_THEME_CUSTOM_MAX_CNT];
} wf_mgr_custom_cfg_t __aligned(4);

int wf_theme_custom_setting_load(uint32_t theme_sequence_id,
								 wf_mgr_custom_cfg_t *data);
int wf_theme_custom_setting_save(uint32_t theme_sequence_id,
								 wf_mgr_custom_cfg_t *data);

#endif