#include "watch_face_theme_1_conf.h"
#include "guix_simple_resources.h"
#include "gx_api.h"
#include "watch_face_theme.h"

struct watch_face_theme1_ctl watch_face_theme_1 = {
	// head
	.head =
		{
			.wfh_magic = WATCH_FACE_HDR_MAGIC_NUM,
			.wfh_hdr_size = sizeof(struct watch_face_theme1_ctl),
			.wfh_element_cnt = (sizeof(struct watch_face_theme1_ctl) - sizeof(struct watch_face_header)) /
							   sizeof(struct wf_element_style),
			.wfh_thumb_resource_id = GX_PIXELMAP_ID__,
		},
	// element 1: background
	.bg =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_IMG,
			.left_pos = 0,
			.top_pos = 0,
			.width = 360,
			.height = 360,
			.extend_param.ext_disp_para.single_img_para =
				{
					.res_id = GX_PIXELMAP_ID__,
					.element_rotate_type = ELEMENT_ROTATE_TYPE_NONE,
				},
			.e_id = 0,
		},

	// element2: hour
	.hour_pointer =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_HOUR,
			//.left_pos = 159,
			.left_pos = 0,
			.top_pos = 0,
			.width = 360, // 41
			.height = 360,
			.extend_param.ext_disp_para.single_img_para =
				{
					.res_id = GX_PIXELMAP_ID_HOUR_ID,
					.element_rotate_type = ELEMENT_ROTATE_TYPE_ROTATING,
					.angle_start = 0,
					.angle_end = 360,
					.x_pos = 159,
					.y_pos = 0,
				},
			.e_id = 1,
		},

	// element3: minute
	.min_ponter =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_MIN,
			.left_pos = 0, // 155,
			.top_pos = 0,
			.width = 360, // 51,
			.height = 360,
			.extend_param.ext_disp_para.single_img_para =
				{
					.res_id = GX_PIXELMAP_ID_MIN_ID,
					.element_rotate_type = ELEMENT_ROTATE_TYPE_ROTATING,
					.angle_start = 0,
					.angle_end = 360,
					.x_pos = 155,
					.y_pos = 0,
				},
			.e_id = 2,
		},

	// element4: sec
	.sec_pointer =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_SEC,
			.left_pos = 0,
			.top_pos = 0,
			.width = 360,
			.height = 360,
			.extend_param.ext_disp_para.single_img_para =
				{
					.res_id = GX_PIXELMAP_ID_SEC_ID,
					.element_rotate_type = ELEMENT_ROTATE_TYPE_ROTATING,
					.angle_start = 0,
					.angle_end = 360,
					.x_pos = 162,
					.y_pos = 0,
				},
			.e_id = 3,
		},
#if 0

	// element5: hour
	.hour =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_HOUR,
			.left_pos = 218,
			.top_pos = 163,
			.res_id_start = GX_PIXELMAP_ID_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_NUM_A,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 4,
		},

	// element6: min
	.min =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_MIN,
			.left_pos = 254,
			.top_pos = 163,
			.res_id_start = GX_PIXELMAP_ID_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_NUM_A,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 5,
		},

	// heart_rate
	.heart_rate =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_HEART_RATE,
			.left_pos = 254,
			.top_pos = 134,
			.res_id_start = GX_PIXELMAP_ID_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_NUM_A,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 6,
		},

	// temp
	.temp =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_TEMP,
			.left_pos = 236,
			.top_pos = 258,
			.res_id_start = GX_PIXELMAP_ID_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_NUM_B,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 7,
		},

	// week
	.week =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_1,
			.e_type = ELEMENT_TYPE_WEEK_DAY,
			.left_pos = 204,
			.top_pos = 185,
			.res_id_start = GX_PIXELMAP_ID_WEEK_1,
			.res_id_end = GX_PIXELMAP_ID_WEEK_7,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 8,
		},

	// day
	.day =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_DAY,
			.left_pos = 256,
			.top_pos = 185,
			.res_id_start = GX_PIXELMAP_ID_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_NUM_A,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 9,
		},

	// month
	.month =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_1,
			.e_type = ELEMENT_TYPE_MONTH,
			.left_pos = 290,
			.top_pos = 185,
			.res_id_start = GX_PIXELMAP_ID_MONTH_1,
			.res_id_end = GX_PIXELMAP_ID_MONTH_C,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 10,
		},

	.steps_progress_bar =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR,
			.e_type = ELEMENT_TYPE_STEPS,
			.left_pos = 55,
			.top_pos = 131,
			.res_id_start = GX_COLOR_ID_BLUE,
			.res_id_end = GX_COLOR_ID_DISABLED_FILL,
			.extend_param.ext_disp_para.arc_para =
				{
					.angle_start = 90,
					.angle_end = 90,
					.brush_width = 2,
					.diameter = 92,
				},

			.e_id = 11,
		},

	.calories_progress_bar =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_ARC_PROGRESS_BAR,
			.e_type = ELEMENT_TYPE_CALORIE,
			.left_pos = 144,
			.top_pos = 70,
			.res_id_start = GX_COLOR_ID_BLUE,
			.res_id_end = GX_COLOR_ID_DISABLED_FILL,
			.extend_param.ext_disp_para.arc_para =
				{
					.angle_start = 60,
					.angle_end = 120,
					.brush_width = 2,
					.diameter = 70,
				},
			.e_id = 12,
		},

	// calories_num
	.calories_num =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_STEPS,
			.left_pos = 65,
			.top_pos = 169,
			.res_id_start = GX_PIXELMAP_ID_KAL_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_KAL_NUM_MINUS,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 13,
		},

	// calories_num
	.steps_num =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_CALORIE,
			.left_pos = 154,
			.top_pos = 92,
			.res_id_start = GX_PIXELMAP_ID_KAL_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_KAL_NUM_MINUS,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 15,
				},
			.e_id = 14,
		},

	// am_pm
	.am_pm =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_1,
			.e_type = ELEMENT_TYPE_AM_PM,
			.left_pos = 291,
			.top_pos = 160,
			.res_id_start = GX_PIXELMAP_ID_AM_PM_0,
			.res_id_end = GX_PIXELMAP_ID_AM_PM_1,
			.extend_param.ext_disp_para.single_img_para =
				{
					.element_rotate_type = ELEMENT_ROTATE_TYPE_NONE,
				},
			.e_id = 15,
		},

	// batt_num
	.batt_num =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_BATTERY_CAPACITY,
			.left_pos = 232,
			.top_pos = 288,
			.res_id_start = GX_PIXELMAP_ID_BATT_NUM_0,
			.res_id_end = GX_PIXELMAP_ID_BATT_NUM_A,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.width_per_unit = 8,
				},
			.e_id = 16,
		},

	// batt_pointer
	.batt_pointer =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_BATTERY_CAPACITY,
			.left_pos = 164,
			.top_pos = 203,
			.res_id_start = GX_PIXELMAP_ID_BATT_ID,

			.extend_param.ext_disp_para.single_img_para =
				{
					.element_rotate_type = ELEMENT_ROTATE_TYPE_ROTATING,
					.angle_start = 270,
					.angle_end = 90,
				},
			.e_id = 17,
		},
#endif
};
