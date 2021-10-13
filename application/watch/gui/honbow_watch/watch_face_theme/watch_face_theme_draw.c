#include "watch_face_theme_draw.h"
//#include "guix_simple_resources.h"
//#include "guix_simple_specifications.h"
#include "watch_face_manager.h"
#include "watch_face_port.h"
extern int wf_mgr_color_get(uint32_t index, void *addr);
extern int wf_mgr_pixelmap_get(uint32_t index, void **addr);

static uint16_t wf_get_rotate_degree(element_type_enum type, uint16_t degree_total)
{
	uint16_t degree_steps = 0;
	uint16_t degree;

	switch (type) {
	case ELEMENT_TYPE_HOUR: {
		int16_t hour, min = 0;
		wf_get_data_func(ELEMENT_TYPE_HOUR)(&hour);
		wf_get_data_func(ELEMENT_TYPE_MIN)(&min);
		degree_steps = degree_total / 12;
		degree = hour % 12 * degree_steps;
		degree += min * degree_steps / 60;
		break;
	}
	case ELEMENT_TYPE_MIN: {
		int value = 0;
		wf_get_data_func(ELEMENT_TYPE_MIN)(&value);
		degree_steps = degree_total / 60;
		degree = value * degree_steps;
		break;
	}
	case ELEMENT_TYPE_SEC: {
		int value = 0;
		wf_get_data_func(type)(&value);
		degree_steps = degree_total / 60;
		degree = value * degree_steps;
		break;
	}
	case ELEMENT_TYPE_BATTERY_CAPACITY: {
		uint8_t value = 0;
		wf_get_data_func(type)(&value);
		degree_steps = degree_total / 20;
		degree = value / 5 * degree_steps;
		break;
	}
	default: // TODO: support more element!
		degree = 0;
		break;
	}
	return degree;
}

// single img only need check rotate or not
void wf_draw_single_img(struct wf_element_style *style, GX_WIDGET *widget)
{
	GX_PIXELMAP *map = NULL;

	single_img_param_t *s_img_para = &style->extend_param.ext_disp_para.single_img_para;

	int16_t degree_total = s_img_para->angle_end - s_img_para->angle_start;
	degree_total = degree_total <= 0 ? degree_total + 360 : (degree_total > 360 ? degree_total % 360 : degree_total);
	wf_mgr_pixelmap_get(s_img_para->res_id, (void **)&map);
	if (map == NULL) {
		return;
	}

	GX_RECTANGLE *rect = &widget->gx_widget_size;
	if (s_img_para->element_rotate_type == ELEMENT_ROTATE_TYPE_ROTATING) {
		uint16_t degree = wf_get_rotate_degree(style->e_type, degree_total);
		degree += s_img_para->angle_start;
		GX_VALUE top = s_img_para->y_pos + rect->gx_rectangle_top;
		GX_VALUE left = s_img_para->x_pos + rect->gx_rectangle_left;
		gx_canvas_pixelmap_rotate(left, top, map, degree, s_img_para->rot_cx, s_img_para->rot_cy);
	} else {
		if (style->e_type == ELEMENT_TYPE_IMG) {
			GX_VALUE top = rect->gx_rectangle_top;
			GX_VALUE left = rect->gx_rectangle_left;
			gx_canvas_pixelmap_draw(left, top, map);
		} else if (style->e_type == ELEMENT_TYPE_BG) {
			// TODO: we should save the bg img in a independent partition?
		}
	}
}

// select one image from supplied images
void wf_draw_multi_img_one(struct wf_element_style *style, GX_WIDGET *widget)
{
	GX_PIXELMAP *map = NULL;
	multi_img_1_param_t *para = &style->extend_param.ext_disp_para.multi_img_1_para;
	uint8_t resource_cnts = para->res_id_end - para->res_id_start + 1;
	uint16_t id = 0;

	switch (style->e_type) {
	case ELEMENT_TYPE_MONTH: {
		if (resource_cnts != 12) {
			return;
		}
		uint8_t curr_month = 0; // month: 0~11
		wf_port_get_value func = wf_get_data_func(style->e_type);
		if (func != NULL) {
			func(&curr_month);
		}
		if ((curr_month > 11)) {
			curr_month = 0;
		}
		id = curr_month + para->res_id_start;
		if (id > para->res_id_end) {
			id = para->res_id_end;
		}
		break;
	}
	case ELEMENT_TYPE_WEEK_DAY: {
		if (resource_cnts != 7) {
			return;
		}
		uint8_t curr_week_day = 0; // week: 0~6
		wf_port_get_value func = wf_get_data_func(style->e_type);
		if (func != NULL) {
			func(&curr_week_day);
		}
		if (curr_week_day >= 7) {
			curr_week_day = 0;
		}
		if (0 == curr_week_day) {
			id = para->res_id_start + 6;
		} else {
			id = para->res_id_start + curr_week_day - 1;
		}
		if (id > para->res_id_end) {
			id = para->res_id_end;
		}
		break;
	}
	case ELEMENT_TYPE_WEATHER:
		if (resource_cnts != 15) {
			return;
		}
		uint8_t curr_weather_type = 0;
		wf_port_get_value func = wf_get_data_func(style->e_type);
		if (func != NULL) {
			func(&curr_weather_type);
		}

		if (curr_weather_type > CLOCK_SKIN_WEATHER_CLOUDY_NIGHT) {
			curr_weather_type = CLOCK_SKIN_WEATHER_SUNNY;
		}
		id = curr_weather_type + para->res_id_start;
		if (id > para->res_id_end) {
			id = para->res_id_end;
		}
		break;
	case ELEMENT_TYPE_BATTERY_CAPACITY: {
		if (resource_cnts != 11) {
			return;
		}
		uint8_t curr_cap = 0;
		wf_port_get_value func = wf_get_data_func(style->e_type);
		if (func != NULL) {
			func(&curr_cap);
		}
		if (curr_cap > 100) {
			curr_cap = 100;
		}
		id = curr_cap / 10 + para->res_id_start;
		if (id > para->res_id_end) {
			id = para->res_id_end;
		}
		break;
	}
	case ELEMENT_TYPE_AM_PM: {
		if (resource_cnts != 2) {
			return;
		}
		uint8_t curr_hour = 0;
		wf_port_get_value func = wf_get_data_func(ELEMENT_TYPE_HOUR);
		if (func != NULL) {
			func(&curr_hour);
		}

		if (curr_hour >= 12) {
			id = para->res_id_start + 1;
		} else {
			id = para->res_id_start;
		}
		break;
	}
	case ELEMENT_TYPE_HOUR_TENS:
	case ELEMENT_TYPE_HOUR_UNITS:
	case ELEMENT_TYPE_MIN_TENS:
	case ELEMENT_TYPE_MIN_UNITS:
	case ELEMENT_TYPE_SEC_TENS:
	case ELEMENT_TYPE_SEC_UNITS: {
		uint8_t curr_value = 0;
		wf_port_get_value func = wf_get_data_func(style->e_type);
		if (func != NULL) {
			func(&curr_value);
		}
		id = para->res_id_start + curr_value;
		if (id > para->res_id_end) {
			return;
		}
		break;
	}
	default: // do nothing
		return;
	}
	wf_mgr_pixelmap_get(id, (void **)&map);
	if (map == NULL) {
		return;
	}

	GX_VALUE top = widget->gx_widget_size.gx_rectangle_top;
	GX_VALUE left = widget->gx_widget_size.gx_rectangle_left;
	gx_canvas_pixelmap_draw(left, top, map);
}

static int32_t power_of_ten(uint8_t n)
{
	int32_t value = 1;
	for (uint8_t i = 0; i < n; i++) {
		value = value * 10;
	}
	return value;
}

/*****************************************************
	0 1 2 3 4 5 6 7 8 9 0 -
	select two or more images from supplied images
 *****************************************************/
static void disp_multi_img_n_display(uint8_t type, int32_t value, struct wf_element_style *style, GX_WIDGET *widget)
{
	uint8_t cnts = 0;
	GX_PIXELMAP *map = NULL;
	multi_img_n_param_t *para = &style->extend_param.ext_disp_para.multi_img_n_para;

	uint16_t resource_start = para->res_id_start;
	uint8_t resource_cnts = para->res_id_end - para->res_id_start + 1;
	switch (type) {
	case MULTI_IMG_TYPE_NXX:
		cnts = 3;
		break;
	case MULTI_IMG_TYPE_XX:
		cnts = 2;
		break;
	case MULTI_IMG_TYPE_XXX:
		cnts = 3;
		break;
	case MULTI_IMG_TYPE_XXXX:
		cnts = 4;
		break;
	case MULTI_IMG_TYPE_XXXXX:
		cnts = 5;
		break;
	case MULTI_IMG_TYPE_XXXXXX:
		cnts = 6;
		break;
	default:
		return;
	}
	uint16_t id = resource_start;
	GX_VALUE top = widget->gx_widget_size.gx_rectangle_top;
	GX_VALUE left = widget->gx_widget_size.gx_rectangle_left;

	uint8_t first_no_zero_flag = 0;
	for (uint8_t i = 0; i < cnts; i++) {
		if ((i == 0) && (type == MULTI_IMG_TYPE_NXX)) { // temperature or other
			if (value < 0) {
				if (resource_cnts >= 12) { // must 12 images
					wf_mgr_pixelmap_get(resource_start + 11, (void **)&map);
				} else {
					map = GX_NULL;
				}
			} else {
				map = GX_NULL;
			}
		} else {
			int32_t value_tmp = value < 0 ? -value : value;
			uint8_t digital_num = value_tmp / power_of_ten(cnts - i - 1) % 10;

			if (0 == first_no_zero_flag) {
				if (0 == digital_num) {
					if ((i != cnts - 1) && (resource_cnts >= 11)) {
						id = resource_start + 10;
					} else {
						id = resource_start;
					}
				} else {
					first_no_zero_flag = 1;
					id = resource_start + digital_num;
				}
			} else {
				id = resource_start + digital_num;
			}
			wf_mgr_pixelmap_get(id, (void **)&map);
		}
		if (map != NULL) {
			gx_canvas_pixelmap_draw(left, top, map);
		}
		left += style->extend_param.ext_disp_para.multi_img_n_para.width_per_unit;
	}
}

// display multi image at a time, for example, hour need two image:XX
void wf_draw_multi_img_n(struct wf_element_style *style, GX_WIDGET *widget)
{
	uint8_t disp_type = MULTI_IMG_TYPE_XX;
	int32_t value = 0;
	element_type_enum data_type = style->e_type;
	wf_port_get_value func = wf_get_data_func(data_type);
	if (func != NULL) {
		func(&value);
	}

	switch (data_type) {
	case ELEMENT_TYPE_MONTH:
		value += 1;
	case ELEMENT_TYPE_DAY:
	case ELEMENT_TYPE_HOUR:
	case ELEMENT_TYPE_MIN:
	case ELEMENT_TYPE_SEC:
		disp_type = MULTI_IMG_TYPE_XX;
		break;
	case ELEMENT_TYPE_HEART_RATE:
		disp_type = MULTI_IMG_TYPE_XX;
		break;
	case ELEMENT_TYPE_TEMP:
		disp_type = MULTI_IMG_TYPE_NXX;
		break;
	case ELEMENT_TYPE_STEPS:
		disp_type = MULTI_IMG_TYPE_XXXXX;
		break;
	case ELEMENT_TYPE_CALORIE:
		disp_type = MULTI_IMG_TYPE_XXXX;
		break;
	case ELEMENT_TYPE_BATTERY_CAPACITY:
		disp_type = MULTI_IMG_TYPE_XXX;
		break;
	default: // TODO: add more
		return;
	}
	disp_multi_img_n_display(disp_type, value, style, widget);
}

/***********************************************************
   angle_start: gx_canvas_arc_draw function's end angle,
				it will always keep a value of setting.
   angle_end:   gx_canvas_arc_draw function's start angle,
				it will reduce with the percent.
 **********************************************************/
void wf_draw_arc(struct wf_element_style *style, GX_WIDGET *widget)
{
	uint8_t percent = 0;
	switch (style->e_type) {
	case ELEMENT_TYPE_STEPS: {
		uint32_t steps = 0;
		wf_port_get_value func = wf_get_data_func(ELEMENT_TYPE_STEPS);
		if (func != NULL) {
			func(&steps);
		}
		percent = steps > 10000 ? 100 : steps / 100;
		break;
	}
	case ELEMENT_TYPE_CALORIE: {
		uint32_t calories = 0;
		wf_port_get_value func = wf_get_data_func(ELEMENT_TYPE_CALORIE);
		if (func != NULL) {
			func(&calories);
		}
		percent = (calories >= 240) ? 100 : (calories * 100 / 240);
		break;
	}
	case ELEMENT_TYPE_BATTERY_CAPACITY: {
		percent = 0;
		wf_port_get_value func = wf_get_data_func(ELEMENT_TYPE_CALORIE);
		if (func != NULL) {
			func(&percent);
		}
		percent = percent > 100 ? 100 : percent;
		break;
	}
	default:
		return;
	}
	arc_para_t *arc_para = &style->extend_param.ext_disp_para.arc_para;
	GX_DRAW_CONTEXT *current_context;
	gx_system_draw_context_get(&current_context);
	GX_COLOR brush_color;
	if (wf_mgr_color_get(arc_para->color_id, &brush_color)) {
		brush_color = 0;
	}
	gx_brush_define(&current_context->gx_draw_context_brush, brush_color, 0, GX_BRUSH_ALIAS);
	gx_context_brush_width_set(arc_para->brush_width);

	int32_t x_center = widget->gx_widget_size.gx_rectangle_left + (arc_para->diameter >> 1);
	int32_t y_center = widget->gx_widget_size.gx_rectangle_top + (arc_para->diameter >> 1);
	int32_t angle_total = arc_para->angle_start - arc_para->angle_end;

	angle_total = angle_total <= 0 ? angle_total + 360 : (angle_total > 360 ? angle_total % 360 : angle_total);

	uint32_t r = arc_para->diameter >> 1;

	int32_t end_angle = arc_para->angle_start;
	int32_t start_angle = 0;
	if (0 != percent) {
		start_angle = arc_para->angle_end + (100 - percent) * angle_total / 100;
	} else {
		start_angle = end_angle - 1;
	}
	gx_canvas_arc_draw(x_center, y_center, r, start_angle, end_angle);
}

static char *week_day[7] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"};
static char *am_pm[2] = {"AM", "PM"};
void wf_draw_sys_font(struct wf_element_style *style, GX_WIDGET *widget)
{
	uint32_t value = 0;
	if ((style->e_type == ELEMENT_TYPE_WEATHER) || (style->e_type == ELEMENT_TYPE_IMG) ||
		(style->e_type == ELEMENT_TYPE_BG)) {
		return;
	}

	wf_port_get_value func = wf_get_data_func(style->e_type);
	if (func != NULL) {
		func(&value);
	}

	GX_PROMPT *prompt = (GX_PROMPT *)widget;
	static char disp[20];
	if ((style->e_type == ELEMENT_TYPE_HOUR) || (style->e_type == ELEMENT_TYPE_MIN) ||
		(style->e_type == ELEMENT_TYPE_SEC) || (style->e_type == ELEMENT_TYPE_DAY)) {
		snprintf(disp, sizeof(disp), "%02d", value);
	} else if (style->e_type == ELEMENT_TYPE_WEEK_DAY) {
		strncpy(disp, week_day[value], sizeof(disp));
	} else if (style->e_type == ELEMENT_TYPE_AM_PM) {
		strncpy(disp, am_pm[value], sizeof(disp));
	} else {
		snprintf(disp, sizeof(disp), "%d", value);
	}
	GX_STRING string;
	string.gx_string_ptr = disp;
	string.gx_string_length = strlen(disp);
	gx_prompt_text_set_ext(prompt, &string);

	gx_prompt_draw(prompt);
}
