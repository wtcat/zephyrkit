#include "watch_face_theme_1_conf.h"

struct watch_face_theme1_ctl watch_face_theme_1 = {
	// head
	.head =
		{
			.wfh_magic = WATCH_FACE_HDR_MAGIC_NUM,
			.wfh_hdr_size = sizeof(struct watch_face_theme1_ctl),
			.wfh_element_cnt = (sizeof(struct watch_face_theme1_ctl) - sizeof(struct watch_face_header)) /
							   sizeof(struct wf_element_style),
			.wfh_thumb_resource_id = GX_PIXELMAP_ID_WF_THUMB,

			.wfh_protocol_major = 0, // protocol version major
			.wfh_protocol_minor = 1, // protocol version minor

			.name = "honbow theme 1",
			.wfh_uniqueID = 0,
		},
	// element 1: background
	.bg =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SINGLE_IMG,
			.e_type = ELEMENT_TYPE_IMG,
			.left_pos = 0,
			.top_pos = 0,
			.width = 320,
			.height = 360,
			.extend_param.ext_disp_para.single_img_para =
				{
					.res_id = GX_PIXELMAP_ID_BUILT_IN_WF1_BG,
					.element_rotate_type = ELEMENT_ROTATE_TYPE_NONE,
				},
			.e_id = 0,
		},
	.hour_tens =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_1,
			.e_type = ELEMENT_TYPE_HOUR_TENS,
			.left_pos = 30,
			.top_pos = 80,
			.width = 96,
			.height = 148,
			.extend_param.ext_disp_para.multi_img_1_para =
				{
					.res_id_start = GX_PIXELMAP_ID_BUILT_IN_WF1_FRONT_NUM_0,
					.res_id_end = GX_PIXELMAP_ID_BUILT_IN_WF1_FRONT_NUM_2,
				},
		},
	.hour_uints =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_1,
			.e_type = ELEMENT_TYPE_HOUR_UNITS,
			.left_pos = 126,
			.top_pos = 80,
			.width = 96,
			.height = 148,
			.extend_param.ext_disp_para.multi_img_1_para =
				{
					.res_id_start = GX_PIXELMAP_ID_BUILT_IN_WF1_HOUR_NUM_0,
					.res_id_end = GX_PIXELMAP_ID_BUILT_IN_WF1_HOUR_NUM_9,
				},
		},
	.min =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_MULTI_IMG_N,
			.e_type = ELEMENT_TYPE_MIN,
			.left_pos = 150,
			.top_pos = 200,
			.width = 192,
			.height = 148,
			.extend_param.ext_disp_para.multi_img_n_para =
				{
					.res_id_start = GX_PIXELMAP_ID_BUILT_IN_WF1_MIN_NUM_0,
					.res_id_end = GX_PIXELMAP_ID_BUILT_IN_WF1_MIN_NUM_9,
					.width_per_unit = 72,
				},
		},
	.week_day =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SYS_FONT,
			.e_type = ELEMENT_TYPE_WEEK_DAY,
			.left_pos = 10,
			.top_pos = 220,
			.width = 100,
			.height = 30,
			.extend_param.ext_disp_para.font_para =
				{
					.font_id = GX_FONT_ID_SIZE_30,
					.color_id = GX_COLOR_ID_HONBOW_WHITE,
				},
		},
	.day =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SYS_FONT,
			.e_type = ELEMENT_TYPE_DAY,
			.left_pos = 100,
			.top_pos = 220,
			.width = 50,
			.height = 30,
			.extend_param.ext_disp_para.font_para =
				{
					.font_id = GX_FONT_ID_SIZE_30,
					.color_id = GX_COLOR_ID_HONBOW_WHITE,
				},
		},
	.am_pm =
		{
			.e_disp_tpye = ELEMENT_DISP_TYPE_SYS_FONT,
			.e_type = ELEMENT_TYPE_AM_PM,
			.left_pos = 250,
			.top_pos = 170,
			.width = 50,
			.height = 26,
			.extend_param.ext_disp_para.font_para =
				{
					.font_id = GX_FONT_ID_SIZE_26,
					.color_id = GX_COLOR_ID_HONBOW_WHITE,
				},
		},
};
