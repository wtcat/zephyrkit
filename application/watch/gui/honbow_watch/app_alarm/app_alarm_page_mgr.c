#include "app_alarm_page_mgr.h"
#include "custom_animation.h"
#include "custom_screen_stack.h"
#include "logging/log.h"

LOG_MODULE_DECLARE(guix_log);

static GX_SCREEN_STACK_CONTROL alarm_screen_stack_control;
static GX_WIDGET *alarm_screen_stack_buff[10];
static GX_WIDGET *s_alarm_page_list[ALARM_PAGE_MAX];
static alarm_page_info_update s_alarm_page_info_update_func[ALARM_PAGE_MAX];
static ALARM_EDIT_TYPE s_edit_type = 0;
static ALARM_INFO_T s_alarm_info_tmp;
static ALARM_INFO_T *record_which_to_edit;

// for special process!
static GX_WIDGET *custom_screen_stack_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};

void app_alarm_page_reg(ALARM_PAGE_T type, GX_WIDGET *widget, alarm_page_info_update func)
{
	s_alarm_page_list[type] = widget;
	s_alarm_page_info_update_func[type] = func;
}

void app_alarm_page_mgr_init(void)
{
	for (uint8_t i = 0; i < ALARM_PAGE_MAX; i++) {
		s_alarm_page_list[i] = NULL;
	}
	custom_screen_stack_create(&alarm_screen_stack_control, alarm_screen_stack_buff, sizeof(alarm_screen_stack_buff));
	custom_screen_stack_reset(&alarm_screen_stack_control);
}

void app_alarm_instance_record(ALARM_INFO_T *info)
{
	if (info != NULL)
		record_which_to_edit = info;
}

void app_alarm_page_jump(ALARM_PAGE_T curr_type, ALARM_OP_T op)
{
	if (GX_TRUE == custom_animation_busy_check()) {
		return;
	}

	ALARM_PAGE_T dst_type;
	if (curr_type == ALARM_PAGE_MAIN) {
		if (op == ALARM_OP_ADD_NEW) {
			s_edit_type = ALARM_EDIT_TYPE_WIZZARD;
			dst_type = ALARM_PAGE_TIME_SEL;
			s_alarm_info_tmp.hour = 7;
			s_alarm_info_tmp.min = 0;
			s_alarm_info_tmp.info_avalid = ALARM_INFO_VALID_FLAG;
			s_alarm_info_tmp.alarm_flag = 0x07;
			s_alarm_info_tmp.repeat_flags = 0x80;
			strncpy(s_alarm_info_tmp.desc, "alarm", sizeof(s_alarm_info_tmp.desc));
			if (s_alarm_page_info_update_func[dst_type])
				s_alarm_page_info_update_func[dst_type](s_edit_type, &s_alarm_info_tmp);
			custom_screen_stack_push(&alarm_screen_stack_control, s_alarm_page_list[curr_type],
									 s_alarm_page_list[dst_type]);
		} else if (op == ALARM_OP_EDIT) {
			dst_type = ALARM_PAGE_EDIT;
			s_edit_type = ALARM_EDIT_TYPE_SINGLE;
			// we should record which alarm need to be edit!
			if (s_alarm_page_info_update_func[dst_type])
				s_alarm_page_info_update_func[dst_type](s_edit_type, record_which_to_edit);
			custom_screen_stack_push(&alarm_screen_stack_control, s_alarm_page_list[curr_type],
									 s_alarm_page_list[dst_type]);
		} else {
			return; // wrong op
		}
	} else if (curr_type == ALARM_PAGE_EDIT) {
		s_edit_type = ALARM_EDIT_TYPE_SINGLE;
		switch (op) {
		case ALARM_OP_TIME_SEL:
			dst_type = ALARM_PAGE_TIME_SEL;
			break;
		case ALARM_OP_RPT_SEL:
			dst_type = ALARM_PAGE_RPT_SEL;
			break;
		case ALARM_OP_VIB_DURATION:
			dst_type = ALARM_PAGE_VIB_DURATION;
			break;
		case ALARM_OP_CANCEL:
			// abort edit op!
			custom_screen_stack_pop(&alarm_screen_stack_control);
			return;
		default:
			s_edit_type = 0;
			return; // wrong op
		}
		s_alarm_info_tmp = *record_which_to_edit;
		if (s_alarm_page_list[dst_type]) {
			if (s_alarm_page_info_update_func[dst_type])
				s_alarm_page_info_update_func[dst_type](s_edit_type, &s_alarm_info_tmp);
			custom_screen_stack_push(&alarm_screen_stack_control, s_alarm_page_list[curr_type],
									 s_alarm_page_list[dst_type]);
		}
	} else {
		if (op == ALARM_OP_CANCEL) {
			custom_screen_stack_pop(&alarm_screen_stack_control);
		} else if (op == ALARM_OP_NEXT_OR_FINISH) {
			if (s_edit_type == ALARM_EDIT_TYPE_SINGLE) {
				// pop前应首先通知view servie 数据发生变化.
				*record_which_to_edit = s_alarm_info_tmp;
				view_service_event_submit(VIEW_EVENT_TYPE_ALARM_EDITED, NULL, 0);
				custom_screen_stack_pop(&alarm_screen_stack_control);
			} else if (s_edit_type == ALARM_EDIT_TYPE_WIZZARD) {
				if (curr_type != ALARM_PAGE_VIB_DURATION) {
					dst_type = curr_type + 1;
					if (s_alarm_page_list[dst_type]) {
						if (s_alarm_page_info_update_func[dst_type])
							s_alarm_page_info_update_func[dst_type](s_edit_type, &s_alarm_info_tmp);
						custom_screen_stack_push(&alarm_screen_stack_control, s_alarm_page_list[curr_type],
												 s_alarm_page_list[dst_type]);
					}
				} else {
					// pop前应首先通知view servie 数据发生变化.
					ALARM_INFO_T *info_tmp = &s_alarm_info_tmp;
					view_service_event_submit(VIEW_EVENT_TYPE_ALARM_ADD, &info_tmp, 4);
					custom_screen_stack_reset(&alarm_screen_stack_control);
					custom_screen_stack_list[0] = s_alarm_page_list[curr_type];
					custom_screen_stack_list[1] = s_alarm_page_list[ALARM_PAGE_MAIN];
					custom_animation_start(custom_screen_stack_list, s_alarm_page_list[curr_type]->gx_widget_parent,
										   SCREEN_SLIDE_RIGHT);
				}
			}
		}
	}
}
