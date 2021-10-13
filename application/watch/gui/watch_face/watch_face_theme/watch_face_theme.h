#ifndef WATCH_FACE_THEME_H
#define WATCH_FACE_THEME_H
#include "gx_api.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>
#include <zephyr.h>

#define WATCH_FACE_HDR_MAGIC_NUM 0xaa55aa7d
#define ELEMENT_MAX_CNT 30

struct watch_face_header {
	uint32_t wfh_magic;
	uint16_t wfh_hdr_size;	  /* Size of image header (bytes). */
	uint16_t wfh_element_cnt; /* cnts of element */
	uint8_t wfh_protocol_major;
	uint8_t wfh_protocol_minor;
	uint16_t wfh_thumb_resource_id;
	char name[16];
	uint32_t wfh_uniqueID;
	char reserved[4];
} __aligned(4);

typedef enum {
	ELEMENT_TYPE_BG = 0,
	ELEMENT_TYPE_YEAR,
	ELEMENT_TYPE_MONTH,
	ELEMENT_TYPE_DAY,
	ELEMENT_TYPE_HOUR,
	ELEMENT_TYPE_MIN,
	ELEMENT_TYPE_SEC,
	ELEMENT_TYPE_AM_PM,
	ELEMENT_TYPE_WEEK_DAY,
	ELEMENT_TYPE_HEART_RATE,
	ELEMENT_TYPE_CALORIE,
	ELEMENT_TYPE_STEPS,
	ELEMENT_TYPE_BATTERY_CAPACITY,
	ELEMENT_TYPE_WEATHER,
	ELEMENT_TYPE_TEMP,
	ELEMENT_TYPE_IMG,
} element_type_enum;

typedef enum {
	ELEMENT_DISP_TYPE_SINGLE_IMG = 0,
	ELEMENT_DISP_TYPE_MULTI_IMG_1,
	ELEMENT_DISP_TYPE_MULTI_IMG_N,
	ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR,
} element_disp_tpye_enum;

typedef enum {
	ELEMENT_ROTATE_TYPE_NONE = 0,
	ELEMENT_ROTATE_TYPE_ROTATING, // rotate by element type
} element_rotate_type_enum;

typedef struct {
	int16_t angle_start;
	int16_t angle_end;
	uint16_t diameter;
	uint16_t brush_width;
	uint16_t color_id;
} arc_para_t __aligned(4);

typedef struct {
	uint16_t width_per_unit;
	uint16_t res_id_start;
	uint16_t res_id_end;
} multi_img_n_param_t __aligned(4);

typedef struct {
	uint16_t res_id_start;
	uint16_t res_id_end;
} multi_img_1_param_t __aligned(4);

typedef struct {
	element_rotate_type_enum element_rotate_type;
	char res_1[3];
	int16_t angle_start;
	int16_t angle_end;
	int16_t x_pos;
	int16_t y_pos;
	uint16_t res_id;
	char res_2[2];

} single_img_param_t __aligned(4);

typedef struct {
	char reserved[16];
} ext_disp_para_reserved_t __aligned(4);

typedef union {
	ext_disp_para_reserved_t reserved_max;
	arc_para_t arc_para;
	single_img_param_t single_img_para;
	multi_img_n_param_t multi_img_n_para;
	multi_img_1_param_t multi_img_1_para;
} ext_disp_para_t __aligned(4);

typedef struct {
	uint8_t plus_or_minus;
	int8_t hour;
	int8_t min;
} ext_hour_type __aligned(4);

typedef struct {
	char reserved[8];
} ext_type_para_reserved_t __aligned(4);

typedef union {
	ext_type_para_reserved_t reserved_max;
	ext_hour_type ext_hour_type;
} ext_type_para_t __aligned(4);

typedef struct {
	ext_disp_para_t ext_disp_para;
	ext_type_para_t ext_type_para;
} extend_param_t __aligned(4);

struct wf_element_style {
	uint8_t e_id;
	element_type_enum e_type;
	element_disp_tpye_enum e_disp_tpye;
	uint8_t hidden_flag; // 0: show,others:hide
	int16_t left_pos;
	int16_t top_pos;
	uint16_t width;
	uint16_t height;
	extend_param_t extend_param;
} __aligned(4);

typedef enum {
	ELEMENT_EDIT_TYPE_HIDDEN_FLAG = 0,
	ELEMENT_EDIT_TYPE_POS,
	ELEMENT_EDIT_TYPE_RES,
	ELEMENT_EDIT_TYPE_COLOR,
	ELEMENT_EDIT_TYPE_WIDTH,
	ELEMENT_EDIT_TYPE_BRUSH_WIDTH,
	ELEMENT_EDIT_TYPE_ANGLE,
} element_edit_type_enum;

typedef struct widget_of_element {
	GX_WIDGET e_widget;
	struct wf_element_style e_info;
} element_style_t;

void wf_theme_init(void);
void wf_theme_deinit(void);
// int wf_theme_active(unsigned char *theme_root_addr);
int wf_theme_active(unsigned char *theme_root_addr, GX_WIDGET *parent);
uint8_t wf_theme_element_cnts_get(void);
// struct wf_element_style *wf_theme_element_styles_get(void);
element_style_t *wf_theme_element_styles_get(void);
bool wf_theme_element_refresh_adjust(element_style_t *style);
uint16_t wf_theme_hdr_size_get(void);
int wf_theme_head_parse(unsigned char *theme_root_addr,
						struct watch_face_header *head_parsed);
int wf_theme_style_sync(uint8_t element_id, uint8_t type, uint32_t value);

#endif
