#ifndef __WATCH_FACE_THEME_1_H__
#define __WATCH_FACE_THEME_1_H__

#include "guix_simple_resources.h"
#include "watch_face_theme.h"

struct watch_face_theme1_ctl {
	struct watch_face_header head;
	struct wf_element_style bg; // background
#if 0

	struct wf_element_style hour; // hour
	struct wf_element_style min;  // min

	struct wf_element_style heart_rate;
	struct wf_element_style temp;

	struct wf_element_style week;
	struct wf_element_style day;
	struct wf_element_style month;

	struct wf_element_style steps_progress_bar;
	struct wf_element_style calories_progress_bar;

	struct wf_element_style steps_num;
	struct wf_element_style calories_num;

	struct wf_element_style am_pm;
	struct wf_element_style batt_num;
	struct wf_element_style batt_pointer;
#endif
	struct wf_element_style hour_pointer; // hour pointer
	struct wf_element_style min_ponter;
	struct wf_element_style sec_pointer;

} __attribute__((aligned(4)));
#endif
