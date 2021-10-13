#include "app_reminders_page_mgr.h"
#include "custom_animation.h"
#include "custom_screen_stack.h"
#include "logging/log.h"

LOG_MODULE_DECLARE(guix_log);

static GX_SCREEN_STACK_CONTROL reminders_screen_stack_control;
static GX_WIDGET *reminders_screen_stack_buff[10];
static GX_WIDGET *s_reminders_page_list[REMINDERS_PAGE_MAX];
static reminders_page_info_update s_reminders_page_info_update_func[REMINDERS_PAGE_MAX];
static REMINDERS_EDIT_TYPE s_edit_type = 0;
static REMINDER_INFO_T s_reminders_info_tmp;
static REMINDER_INFO_T *record_which_to_edit;

// for special process!
static GX_WIDGET *custom_screen_stack_list[] = {
	GX_NULL,
	GX_NULL,
	GX_NULL,
};

void app_reminders_page_reg(REMINDERS_PAGE_T type, GX_WIDGET *widget, reminders_page_info_update func)
{
	s_reminders_page_list[type] = widget;
	s_reminders_page_info_update_func[type] = func;
}

void app_reminders_page_mgr_init(void)
{
	for (uint8_t i = 0; i < REMINDERS_PAGE_MAX; i++) {
		s_reminders_page_list[i] = NULL;
	}
	custom_screen_stack_create(&reminders_screen_stack_control, reminders_screen_stack_buff,
							   sizeof(reminders_screen_stack_buff));
	custom_screen_stack_reset(&reminders_screen_stack_control);
}

void app_reminders_instance_record(REMINDER_INFO_T *info)
{
	if (info != NULL)
		record_which_to_edit = info;
}

void app_reminders_page_jump(REMINDERS_PAGE_T curr_type, REMINDERS_OP_T op)
{
	if (GX_TRUE == custom_animation_busy_check()) {
		return;
	}

	REMINDERS_PAGE_T dst_type;
	if (curr_type == REMINDERS_PAGE_MAIN) {
		if (op == REMINDERS_OP_ADD_NEW) {
			s_edit_type = REMINDERS_EDIT_TYPE_WIZZARD;
			dst_type = REMINDERS_PAGE_TIME_SEL;
			s_reminders_info_tmp.hour = 7;
			s_reminders_info_tmp.min = 0;
			s_reminders_info_tmp.info_avalid = REMINDERS_INFO_VALID_FLAG;
			s_reminders_info_tmp.alarm_flag = 0x01;
			s_reminders_info_tmp.repeat_flags = 0x80;
			strncpy(s_reminders_info_tmp.desc, "reminder", sizeof(s_reminders_info_tmp.desc));
			if (s_reminders_page_info_update_func[dst_type])
				s_reminders_page_info_update_func[dst_type](s_edit_type, &s_reminders_info_tmp);
			custom_screen_stack_push(&reminders_screen_stack_control, s_reminders_page_list[curr_type],
									 s_reminders_page_list[dst_type]);
		} else if (op == REMINDERS_OP_EDIT) {
			dst_type = REMINDERS_PAGE_EDIT;
			s_edit_type = REMINDERS_EDIT_TYPE_SINGLE;
			// we should record which alarm need to be edit!
			if (s_reminders_page_info_update_func[dst_type])
				s_reminders_page_info_update_func[dst_type](s_edit_type, record_which_to_edit);
			custom_screen_stack_push(&reminders_screen_stack_control, s_reminders_page_list[curr_type],
									 s_reminders_page_list[dst_type]);
		} else {
			return; // wrong op
		}
	} else if (curr_type == REMINDERS_PAGE_EDIT) {
		s_edit_type = REMINDERS_EDIT_TYPE_SINGLE;
		switch (op) {
		case REMINDERS_OP_TIME_SEL:
			dst_type = REMINDERS_PAGE_TIME_SEL;
			break;
		case REMINDERS_OP_RPT_SEL:
			dst_type = REMINDERS_PAGE_RPT_SEL;
			break;
		case REMINDERS_OP_VIB_DURATION:
			dst_type = REMINDERS_PAGE_VIB_DURATION;
			break;
		case REMINDERS_OP_CANCEL:
			// abort edit op!
			custom_screen_stack_pop(&reminders_screen_stack_control);
			return;
		default:
			s_edit_type = 0;
			return; // wrong op
		}
		s_reminders_info_tmp = *record_which_to_edit;
		if (s_reminders_page_list[dst_type]) {
			if (s_reminders_page_info_update_func[dst_type])
				s_reminders_page_info_update_func[dst_type](s_edit_type, &s_reminders_info_tmp);
			custom_screen_stack_push(&reminders_screen_stack_control, s_reminders_page_list[curr_type],
									 s_reminders_page_list[dst_type]);
		}
	} else {
		if (op == REMINDERS_OP_CANCEL) {
			custom_screen_stack_pop(&reminders_screen_stack_control);
		} else if (op == REMINDERS_OP_NEXT_OR_FINISH) {
			if (s_edit_type == REMINDERS_EDIT_TYPE_SINGLE) {
				// pop前应首先通知view servie 数据发生变化.
				*record_which_to_edit = s_reminders_info_tmp;
				view_service_event_submit(VIEW_EVENT_TYPE_REMINDERS_EDITED, NULL, 0);
				custom_screen_stack_pop(&reminders_screen_stack_control);
			} else if (s_edit_type == REMINDERS_EDIT_TYPE_WIZZARD) {
				if (curr_type != REMINDERS_PAGE_VIB_DURATION) {
					dst_type = curr_type + 1;
					if (s_reminders_page_list[dst_type]) {
						if (s_reminders_page_info_update_func[dst_type])
							s_reminders_page_info_update_func[dst_type](s_edit_type, &s_reminders_info_tmp);
						custom_screen_stack_push(&reminders_screen_stack_control, s_reminders_page_list[curr_type],
												 s_reminders_page_list[dst_type]);
					}
				} else {
					// pop前应首先通知view servie 数据发生变化.
					REMINDER_INFO_T *info_tmp = &s_reminders_info_tmp;
					view_service_event_submit(VIEW_EVENT_TYPE_REMINDERS_ADD, &info_tmp, 4);
					custom_screen_stack_reset(&reminders_screen_stack_control);
					custom_screen_stack_list[0] = s_reminders_page_list[curr_type];
					custom_screen_stack_list[1] = s_reminders_page_list[REMINDERS_PAGE_MAIN];
					custom_animation_start(custom_screen_stack_list, s_reminders_page_list[curr_type]->gx_widget_parent,
										   SCREEN_SLIDE_RIGHT);
				}
			}
		}
	}
}
