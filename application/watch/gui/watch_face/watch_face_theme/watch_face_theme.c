#include "watch_face_theme.h"
#include "sys/sem.h"
#include "sys/util.h"
#include "watch_face_theme_draw.h"
static struct watch_face_header watch_face_header_curr;
// static struct wf_element_style element_styles[ELEMENT_MAX_CNT];
static uint8_t _element_cnts_actual = 0;
element_style_t element_style_list[ELEMENT_MAX_CNT];
static struct sys_sem sem_lock;
#define SYNC_INIT() sys_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() sys_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() sys_sem_give(&sem_lock)

static int wf_theme_verify(unsigned char *theme_root_addr)
{
	struct watch_face_header watch_face_header_tmp;
	memcpy((void *)&watch_face_header_tmp, theme_root_addr,
		   sizeof(struct watch_face_header));
	if (watch_face_header_tmp.wfh_magic != WATCH_FACE_HDR_MAGIC_NUM) {
		return -EINVAL;
	}
	return 0;
}

int wf_theme_head_parse(unsigned char *theme_root_addr,
						struct watch_face_header *head_parsed)
{
	if (wf_theme_verify(theme_root_addr)) {
		return -EINVAL;
	}
	memcpy((void *)head_parsed, theme_root_addr,
		   sizeof(struct watch_face_header));
	return 0;
}

static void wf_child_widget_draw(GX_WIDGET *widget)
{
	element_style_t *element_style =
		CONTAINER_OF(widget, element_style_t, e_widget);
	switch (element_style->e_info.e_disp_tpye) {
	case ELEMENT_DISP_TYPE_SINGLE_IMG:
		wf_draw_single_img(&element_style->e_info, widget);
		break;
	case ELEMENT_DISP_TYPE_MULTI_IMG_1:
		wf_draw_multi_img_one(&element_style->e_info, widget);
		break;
	case ELEMENT_DISP_TYPE_MULTI_IMG_N:
		wf_draw_multi_img_n(&element_style->e_info, widget);
		break;
	case ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR:
		wf_draw_arc(&element_style->e_info, widget);
		break;
	default:
		break;
	}
}

int wf_theme_active(unsigned char *theme_root_addr, GX_WIDGET *parent)
{
	SYNC_LOCK();

	// delete old widget info
	for (uint16_t i = 0; i < _element_cnts_actual; i++) {
		gx_widget_delete(&element_style_list[i].e_widget);
	}

	uint32_t offset = 0;
	if (wf_theme_verify(theme_root_addr)) {
		_element_cnts_actual = 0;
		SYNC_UNLOCK();
		return -EINVAL;
	}

	memcpy((void *)&watch_face_header_curr, theme_root_addr + offset,
		   sizeof(struct watch_face_header));
	offset += sizeof(struct watch_face_header);

	_element_cnts_actual =
		watch_face_header_curr.wfh_element_cnt >= ELEMENT_MAX_CNT
			? ELEMENT_MAX_CNT
			: watch_face_header_curr.wfh_element_cnt;

	for (uint16_t i = 0; i < _element_cnts_actual; i++) {
		element_style_t *p_element = &element_style_list[i];
		memcpy((char *)&p_element->e_info, theme_root_addr + offset,
			   sizeof(struct wf_element_style));

		struct wf_element_style *info = &p_element->e_info;
		GX_RECTANGLE childsize;
		gx_utility_rectangle_define(&childsize, info->left_pos, info->top_pos,
									info->left_pos + info->width - 1,
									info->top_pos + info->height - 1);
		gx_widget_create(&p_element->e_widget, NULL, parent,
						 GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT, GX_ID_NONE,
						 &childsize);
		gx_widget_draw_set(&p_element->e_widget, wf_child_widget_draw);
		offset += sizeof(struct wf_element_style);
	}

	SYNC_UNLOCK();

	return 0;
}

void wf_theme_init(void)
{
	SYNC_INIT();
}
void wf_theme_deinit(void)
{
}

uint8_t wf_theme_element_cnts_get(void)
{
	SYNC_LOCK();
	uint8_t result = _element_cnts_actual;
	SYNC_UNLOCK();
	return result;
}

element_style_t *wf_theme_element_styles_get(void)
{
	SYNC_LOCK();
	element_style_t *p_result = element_style_list;
	SYNC_UNLOCK();
	return p_result;
}

bool wf_theme_element_refresh_adjust(element_style_t *style)
{
	switch (style->e_info.e_type) {
	case ELEMENT_TYPE_SEC: {
		static uint8_t sec_old = 0xff;
		uint8_t sec_now = watch_face_get_sec();
		if (sec_now != sec_old) {
			sec_old = sec_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_MIN: {
		static uint8_t min_old = 0xff;
		uint8_t min_now = watch_face_get_min();
		if (min_now != min_old) {
			min_old = min_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_HOUR: {
		static uint8_t hour_old = 0xff;
		uint8_t hour_now = watch_face_get_hour();
		if (hour_now != hour_old) {
			hour_old = hour_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_DAY: {
		static uint8_t day_old = 0xff;
		uint8_t day_now = watch_face_get_day();
		if (day_now != day_old) {
			day_old = day_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_MONTH: {
		static uint8_t mon_old = 0xff;
		uint8_t mon_now = watch_face_get_month();
		if (mon_now != mon_old) {
			mon_old = mon_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_YEAR: {
		static int value_old = 0xff;
		int value_now = watch_face_get_year();
		if (value_now != value_old) {
			value_old = value_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_AM_PM: {
		static uint8_t value_old = 0xff;
		uint8_t value_now = watch_face_get_am_pm_type();
		if (value_now != value_old) {
			value_old = value_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_WEEK_DAY: {
		static uint8_t value_old = 0xff;
		uint8_t value_now = watch_face_get_week();
		if (value_now != value_old) {
			value_old = value_now;
			return true;
		}
		break;
	}
	case ELEMENT_TYPE_STEPS:
	case ELEMENT_TYPE_CALORIE:
	case ELEMENT_TYPE_HEART_RATE:
	case ELEMENT_TYPE_BATTERY_CAPACITY:
	case ELEMENT_TYPE_WEATHER:
	case ELEMENT_TYPE_TEMP:
		return true;
		break;
	default:
		return false;
	}
	return true;
}

uint16_t wf_theme_hdr_size_get(void)
{
	SYNC_LOCK();
	uint16_t result = watch_face_header_curr.wfh_hdr_size;
	SYNC_UNLOCK();
	return result;
}

int wf_theme_style_sync(uint8_t element_id, uint8_t type, uint32_t value)
{
	SYNC_LOCK();

	if (element_id >= _element_cnts_actual) {
		SYNC_UNLOCK();
		return -EINVAL;
	}

	struct wf_element_style *style = NULL;

	for (uint8_t i = 0; i < _element_cnts_actual; i++) {
		if (element_style_list[i].e_info.e_id == element_id) {
			style = &element_style_list[i].e_info; // find it
			break;
		}
	}

	if (NULL == style) {
		SYNC_UNLOCK();
		return -EINVAL;
	}

	switch (type) {
	case ELEMENT_EDIT_TYPE_HIDDEN_FLAG:
		style->hidden_flag = value;
		break;
	case ELEMENT_EDIT_TYPE_POS:
		style->left_pos = value & 0xffff;
		style->top_pos = value >> 16;
		break;
	case ELEMENT_EDIT_TYPE_RES:
		switch (style->e_disp_tpye) {
		case ELEMENT_DISP_TYPE_SINGLE_IMG: {
			single_img_param_t *s_img_para =
				&style->extend_param.ext_disp_para.single_img_para;
			s_img_para->res_id = value & 0xffff;
			break;
		}
		case ELEMENT_DISP_TYPE_MULTI_IMG_1: {
			multi_img_1_param_t *para =
				&style->extend_param.ext_disp_para.multi_img_1_para;
			para->res_id_start = value & 0xffff;
			para->res_id_end = value >> 16;
			break;
		}
		case ELEMENT_DISP_TYPE_MULTI_IMG_N: {
			multi_img_n_param_t *para =
				&style->extend_param.ext_disp_para.multi_img_n_para;
			para->res_id_start = value & 0xffff;
			para->res_id_end = value >> 16;
			break;
		}
		default:
			break;
		}
		break;
	case ELEMENT_EDIT_TYPE_ANGLE:
		switch (style->e_disp_tpye) {
		case ELEMENT_DISP_TYPE_SINGLE_IMG: {
			single_img_param_t *s_img_para =
				&style->extend_param.ext_disp_para.single_img_para;
			s_img_para->angle_start = value & 0xffff;
			s_img_para->angle_end = value >> 16;
			break;
		}
		case ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR: {
			arc_para_t *arc_para = &style->extend_param.ext_disp_para.arc_para;
			arc_para->angle_start = value & 0xffff;
			arc_para->angle_end = value >> 16;
			break;
		}
		default:
			break;
		}
		break;
	case ELEMENT_EDIT_TYPE_COLOR: // TODO: need confirm edit real value or
								  // resource id
		break;
	case ELEMENT_EDIT_TYPE_WIDTH:
		switch (style->e_disp_tpye) {
		case ELEMENT_DISP_TYPE_MULTI_IMG_N: {
			multi_img_n_param_t *multi_img_n_para =
				&style->extend_param.ext_disp_para.multi_img_n_para;
			multi_img_n_para->width_per_unit = value;
			break;
		}
		case ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR:
			style->extend_param.ext_disp_para.arc_para.diameter = value;
			break;
		default:
			break;
		}
		break;
	case ELEMENT_EDIT_TYPE_BRUSH_WIDTH:
		switch (style->e_disp_tpye) {
		case ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR:
			style->extend_param.ext_disp_para.arc_para.brush_width = value;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	SYNC_UNLOCK();

	return 0;
}
