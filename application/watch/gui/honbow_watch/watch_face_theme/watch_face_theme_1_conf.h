#ifndef __WATCH_FACE_THEME_1_H__
#define __WATCH_FACE_THEME_1_H__

#include "guix_simple_resources.h"
#include "gx_api.h"
#include "watch_face_theme.h"

struct watch_face_theme1_ctl {
	struct watch_face_header head;
	struct wf_element_style bg; // background
	struct wf_element_style hour_tens;
	struct wf_element_style hour_uints;
	struct wf_element_style min;
	struct wf_element_style week_day;
	struct wf_element_style day;
	struct wf_element_style am_pm;
} __attribute__((aligned(4)));
#endif
