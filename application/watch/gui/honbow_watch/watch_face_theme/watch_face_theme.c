#include "watch_face_theme.h"
#include "sys/util.h"
#include "watch_face_port.h"
#include "watch_face_theme_draw.h"
#include "logging/log.h"

LOG_MODULE_REGISTER(wf_theme);
static struct watch_face_header watch_face_header_curr;
static uint8_t _element_cnts_actual = 0;
static element_style_t element_style_list[ELEMENT_MAX_CNT];

static struct sys_sem sem_lock;
#define SYNC_INIT() sys_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() sys_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() sys_sem_give(&sem_lock)

K_HEAP_DEFINE(wf_theme_mpool, 5 * 1024);

static void *wf_mem_alloc(ULONG size)
{
	return k_heap_alloc(&wf_theme_mpool, size, K_NO_WAIT);
}

static void wf_mem_free(void *ptr)
{
	k_heap_free(&wf_theme_mpool, ptr);
}
static int wf_theme_verify(unsigned char *theme_root_addr)
{
	struct watch_face_header watch_face_header_tmp;
	memcpy((void *)&watch_face_header_tmp, theme_root_addr, sizeof(struct watch_face_header));
	if (watch_face_header_tmp.wfh_magic != WATCH_FACE_HDR_MAGIC_NUM) {
		return -EINVAL;
	}
	return 0;
}

int wft_head_parse(unsigned char *theme_root_addr, struct watch_face_header *head_parsed)
{
	if (wf_theme_verify(theme_root_addr)) {
		return -EINVAL;
	}
	memcpy((void *)head_parsed, theme_root_addr, sizeof(struct watch_face_header));
	return 0;
}

static void wf_child_widget_draw(GX_WIDGET *widget)
{
	element_style_t *element_style = NULL;
	for (uint16_t i = 0; i < _element_cnts_actual; i++) {
		if (element_style_list[i].e_widget == widget) {
			element_style = &element_style_list[i];
		}
	}

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
	case ELEMENT_DISP_TYPE_SYS_FONT:
		wf_draw_sys_font(&element_style->e_info, widget);
		break;
	default:
		break;
	}
}
static void wft_simg_rotate_rect_calc(single_img_param_t *para, GX_RECTANGLE *rect)
{
	int16_t rot_cx, rot_cy;
	if (para->rot_cx == -1) {
		rot_cx = para->width >> 1;
	} else {
		rot_cx = para->rot_cx;
	}

	if (para->rot_cy == -1) {
		rot_cy = para->height >> 1;
	} else {
		rot_cy = para->rot_cy;
	}

	int16_t r = MAX(rot_cy, rot_cy);

	int16_t r_x = para->x_pos + rot_cx - 1;
	int16_t r_y = para->y_pos + rot_cy - 1;

	rect->gx_rectangle_top = r_y - r;
	rect->gx_rectangle_left = r_x - r;
	rect->gx_rectangle_right = rect->gx_rectangle_left + (r << 1);
	rect->gx_rectangle_bottom = rect->gx_rectangle_top + (r << 1);
}

int wft_active(unsigned char *theme_root_addr, GX_WIDGET *parent)
{
	SYNC_LOCK();

	// delete old widget info
	for (uint16_t i = 0; i < _element_cnts_actual; i++) {
		gx_widget_delete(element_style_list[i].e_widget);
		wf_mem_free(element_style_list[i].e_widget);
	}

	uint32_t offset = 0;
	if (wf_theme_verify(theme_root_addr)) {
		_element_cnts_actual = 0;
		SYNC_UNLOCK();
		return -EINVAL;
	}

	memcpy((void *)&watch_face_header_curr, theme_root_addr + offset, sizeof(struct watch_face_header));
	offset += sizeof(struct watch_face_header);

	_element_cnts_actual = watch_face_header_curr.wfh_element_cnt >= ELEMENT_MAX_CNT
							   ? ELEMENT_MAX_CNT
							   : watch_face_header_curr.wfh_element_cnt;

	for (uint16_t i = 0; i < _element_cnts_actual; i++) {
		element_style_t *p_element = &element_style_list[i];
		memcpy((char *)&p_element->e_info, theme_root_addr + offset, sizeof(struct wf_element_style));

		struct wf_element_style *info = &p_element->e_info;
		GX_RECTANGLE childsize;
		gx_utility_rectangle_define(&childsize, info->left_pos, info->top_pos, info->left_pos + info->width - 1,
									info->top_pos + info->height - 1);
		if (p_element->e_info.e_disp_tpye == ELEMENT_DISP_TYPE_SYS_FONT) {
			GX_PROMPT *widget = wf_mem_alloc(sizeof(GX_PROMPT));
			if (widget != NULL) {
				p_element->e_widget = (GX_WIDGET *)widget;
				gx_prompt_create(widget, NULL, parent, 0,
								 GX_STYLE_TRANSPARENT | GX_STYLE_TEXT_CENTER | GX_STYLE_BORDER_NONE, 0, &childsize);
				uint16_t font_id = p_element->e_info.extend_param.ext_disp_para.font_para.font_id;
				uint16_t color_id = p_element->e_info.extend_param.ext_disp_para.font_para.color_id;
				gx_prompt_text_color_set(widget, color_id, color_id, color_id);
				gx_prompt_font_set(widget, font_id);
			}
		} else {
			GX_WIDGET *widget = wf_mem_alloc(sizeof(GX_WIDGET));
			if (widget != NULL) {
				p_element->e_widget = widget;
				struct wf_element_style *info = &p_element->e_info;
				ext_disp_para_t *extend_info = &info->extend_param.ext_disp_para;
				if ((info->e_disp_tpye == ELEMENT_DISP_TYPE_SINGLE_IMG) &&
					(extend_info->single_img_para.element_rotate_type == ELEMENT_ROTATE_TYPE_ROTATING)) {
					GX_RECTANGLE size;
					wft_simg_rotate_rect_calc(&extend_info->single_img_para, &size);
					gx_widget_create(p_element->e_widget, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
									 GX_ID_NONE, &size);
					// calc relative position
					extend_info->single_img_para.x_pos -= size.gx_rectangle_left;
					extend_info->single_img_para.y_pos -= size.gx_rectangle_top;
				} else {
					gx_widget_create(p_element->e_widget, NULL, parent, GX_STYLE_ENABLED | GX_STYLE_TRANSPARENT,
									 GX_ID_NONE, &childsize);
				}
			}
		}
		gx_widget_draw_set(p_element->e_widget, wf_child_widget_draw);
		offset += sizeof(struct wf_element_style);
	}

	SYNC_UNLOCK();

	return 0;
}

void wft_init(void)
{
	SYNC_INIT();
}
void wft_deinit(void)
{
}

uint8_t wft_element_cnts_get(void)
{
	SYNC_LOCK();
	uint8_t result = _element_cnts_actual;
	SYNC_UNLOCK();
	return result;
}

element_style_t *wft_element_styles_get(void)
{
	SYNC_LOCK();
	element_style_t *p_result = element_style_list;
	SYNC_UNLOCK();
	return p_result;
}

bool wft_element_refresh_judge(element_style_t *style)
{
	static uint32_t data_last[ELEMENT_TYPE_MAX];

	uint32_t value_new;
	wf_port_get_value func = wf_get_data_func(style->e_info.e_type);
	if (func != NULL) {
		func(&value_new);
	} else {
		if (style->e_info.e_type != ELEMENT_TYPE_IMG) {
			LOG_ERR("type %d have no data func!", style->e_info.e_type);
		}
		return false;
	}

	if (value_new != data_last[style->e_info.e_type]) {
		data_last[style->e_info.e_type] = value_new;
		return true;
	}

	return false;
}

uint16_t wft_hdr_size_get(void)
{
	SYNC_LOCK();
	uint16_t result = watch_face_header_curr.wfh_hdr_size;
	SYNC_UNLOCK();
	return result;
}

int wft_style_sync(uint8_t element_id, uint8_t type, uint32_t value)
{
	SYNC_LOCK();

	if (element_id >= _element_cnts_actual) {
		SYNC_UNLOCK();
		return -EINVAL;
	}

	struct wf_element_style *style = NULL;
	GX_WIDGET *widget = NULL;

	for (uint8_t i = 0; i < _element_cnts_actual; i++) {
		if (element_style_list[i].e_info.e_id == element_id) {
			style = &element_style_list[i].e_info; // find it
			widget = element_style_list[i].e_widget;
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
		if ((style->e_disp_tpye == ELEMENT_DISP_TYPE_SINGLE_IMG) &&
			(style->extend_param.ext_disp_para.single_img_para.element_rotate_type == ELEMENT_ROTATE_TYPE_ROTATING)) {
			GX_RECTANGLE size;
			single_img_param_t *para = &style->extend_param.ext_disp_para.single_img_para;
			para->x_pos = value & 0xffff;
			para->y_pos = value >> 16;
			wft_simg_rotate_rect_calc(para, &size);
			gx_widget_resize(widget, &size);
			// calc relative position
			para->x_pos -= size.gx_rectangle_left;
			para->y_pos -= size.gx_rectangle_top;
		} else {
			style->left_pos = value & 0xffff;
			style->top_pos = value >> 16;
			GX_RECTANGLE rect;
			rect.gx_rectangle_left = style->left_pos;
			rect.gx_rectangle_top = style->top_pos;
			rect.gx_rectangle_right = style->left_pos + style->width - 1;
			rect.gx_rectangle_bottom = style->top_pos + style->height - 1;
			gx_widget_resize(widget, &rect);
		}
		break;
	case ELEMENT_EDIT_TYPE_RES:
		switch (style->e_disp_tpye) {
		case ELEMENT_DISP_TYPE_SINGLE_IMG: {
			single_img_param_t *s_img_para = &style->extend_param.ext_disp_para.single_img_para;
			s_img_para->res_id = value & 0xffff;
			break;
		}
		case ELEMENT_DISP_TYPE_MULTI_IMG_1: {
			multi_img_1_param_t *para = &style->extend_param.ext_disp_para.multi_img_1_para;
			para->res_id_start = value & 0xffff;
			para->res_id_end = value >> 16;
			break;
		}
		case ELEMENT_DISP_TYPE_MULTI_IMG_N: {
			multi_img_n_param_t *para = &style->extend_param.ext_disp_para.multi_img_n_para;
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
			single_img_param_t *s_img_para = &style->extend_param.ext_disp_para.single_img_para;
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
			multi_img_n_param_t *multi_img_n_para = &style->extend_param.ext_disp_para.multi_img_n_para;
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
