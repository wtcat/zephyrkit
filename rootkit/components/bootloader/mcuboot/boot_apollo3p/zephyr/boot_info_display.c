#include "src/lv_core/lv_disp.h"
#include "src/lv_core/lv_obj.h"
#include "src/lv_core/lv_obj_style_dec.h"
#include "src/lv_core/lv_refr.h"
#include "src/lv_core/lv_style.h"
#include "src/lv_misc/lv_area.h"
#include "src/lv_misc/lv_color.h"
#include <device.h>
#include <drivers/display.h>
#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>

#include "boot_event.h"
#include "src/lv_widgets/lv_label.h"

/* size of stack area used by display thread */
#define DISP_THREAD_STACKSIZE 8192
/* scheduling priority used by display thread */
#define DISP_THREAD_PRIORITY  5

static lv_style_t style_default;
static lv_style_t style_full;
LV_IMG_DECLARE(logo);

#define LV_CUSTOME_LINE_COLOR         LV_COLOR_MAKE(0x80, 0x80, 0x80)
#define LV_CUSTOME_LINE_END_COLOR     LV_COLOR_MAKE(0x20, 0x20, 0x20)
#define LV_CUSTOM_LINE_LENGTH       12
#define LV_CUSTOM_LINE_WIDTH        2

static void style_init(void)
{
	lv_style_init(&style_default);
	lv_style_init(&style_full);

	lv_style_set_bg_color(&style_default, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_line_width(&style_default, LV_STATE_DEFAULT, LV_CUSTOM_LINE_WIDTH);
	lv_style_set_scale_grad_color(&style_default, LV_STATE_DEFAULT, LV_CUSTOME_LINE_COLOR);
	lv_style_set_line_color(&style_default, LV_STATE_DEFAULT, LV_CUSTOME_LINE_COLOR);
	lv_style_set_border_color(&style_default, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	//line length
	lv_style_set_scale_width(&style_default, LV_STATE_DEFAULT, LV_CUSTOM_LINE_LENGTH);
	//end line color & width
	lv_style_set_scale_end_color(&style_default, LV_STATE_DEFAULT, LV_CUSTOME_LINE_END_COLOR);
	lv_style_set_scale_end_line_width(&style_default, LV_STATE_DEFAULT, LV_CUSTOM_LINE_WIDTH);

	lv_style_set_bg_color(&style_full, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_style_set_line_width(&style_full, LV_STATE_DEFAULT, LV_CUSTOM_LINE_WIDTH);
	lv_style_set_scale_grad_color(&style_full, LV_STATE_DEFAULT, LV_CUSTOME_LINE_COLOR);
	lv_style_set_line_color(&style_full, LV_STATE_DEFAULT, LV_CUSTOME_LINE_COLOR);
	lv_style_set_border_color(&style_full, LV_STATE_DEFAULT, LV_COLOR_BLACK);
	//line length
	lv_style_set_scale_width(&style_default, LV_STATE_DEFAULT, LV_CUSTOM_LINE_LENGTH);
	//end line color & width
	lv_style_set_scale_end_color(&style_full, LV_STATE_DEFAULT, LV_CUSTOME_LINE_COLOR);
	lv_style_set_scale_end_line_width(&style_full, LV_STATE_DEFAULT, LV_CUSTOM_LINE_WIDTH);
}

void boot_disp_thread(void)
{
	const struct device *display_dev;

	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL) {
		return;
	}
	style_init();

	lv_obj_t *bg_obj = lv_scr_act();
	lv_obj_set_size(bg_obj, 360, 360);
	lv_obj_set_style_local_bg_color(bg_obj, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

	lv_obj_t * logo_obj = lv_img_create(bg_obj, NULL);
    lv_img_set_src(logo_obj, &logo);
    lv_obj_align(logo_obj, bg_obj, LV_ALIGN_CENTER, 0, 0);

	/*Create a line meter */
    lv_obj_t * progress_obj;
    progress_obj = lv_linemeter_create(bg_obj, NULL);
	lv_linemeter_set_angle_offset(progress_obj, -3 + 180);
    lv_linemeter_set_range(progress_obj, 0, 59);                   /*Set the range*/
    lv_linemeter_set_value(progress_obj, 0);                       /*Set the current value*/
    lv_linemeter_set_scale(progress_obj, 354, 60);                 /*Set the angle and number of lines*/
    lv_obj_set_size(progress_obj, 360, 360);
    lv_obj_align(progress_obj, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_style(progress_obj, LV_LINEMETER_PART_MAIN, &style_default);
	lv_obj_set_hidden(progress_obj, true);

	lv_obj_move_foreground(logo_obj);

	lv_obj_t * err_info = lv_label_create(bg_obj, NULL); 
	lv_label_set_text(err_info, "");
	lv_obj_set_hidden(err_info, true);


	lv_obj_invalidate(lv_scr_act());

	display_blanking_off(display_dev);
	lv_task_handler();

	while (1) {

		struct boot_event event_rec;

		if (!boot_event_event_pop(&event_rec, false)) {
			switch (event_rec.msg[0])
			{
				case EVENT_STATUS_BOOT:
					if (!lv_obj_get_hidden(progress_obj)) {
						lv_obj_set_hidden(progress_obj, true);
					}

					if ((event_rec.msg[1] == EVENT_BOOT_ERRNO_RESUME_FAIL) 
						|| (event_rec.msg[1] == EVENT_BOOT_ERRNO_IMG_NOT_VALID)){

						if (lv_obj_get_hidden(err_info)) {
							lv_obj_set_hidden(err_info, false);
						}
						lv_label_set_text(err_info,  "Contact manufacturer for repair!");
						lv_obj_align(err_info, bg_obj, LV_ALIGN_IN_BOTTOM_MID, 0, -50);
					} else { //do nothing
					}
					
					break;
				case EVENT_STATUS_UPDATING:  //updating status
					if (lv_obj_get_hidden(progress_obj)) {
						lv_obj_set_hidden(progress_obj, false);
					}

					if (!lv_obj_get_hidden(err_info)) {
						lv_obj_set_hidden(err_info, true);
					}

					uint8_t value = event_rec.msg[1] * 60 / 100;

					if (value <= 59) {
						lv_obj_add_style(progress_obj, LV_LINEMETER_PART_MAIN, &style_default);
					} else {
						lv_obj_add_style(progress_obj, LV_LINEMETER_PART_MAIN, &style_full);
					}
					lv_linemeter_set_value(progress_obj, value);
					break;
				case EVENT_STATUS_UPDATE_ERROR: //update error!
					if (!lv_obj_get_hidden(progress_obj)) {
						lv_obj_set_hidden(progress_obj, true);
					}

					if (lv_obj_get_hidden(err_info)) {
						lv_obj_set_hidden(err_info, false);
					}

					if (event_rec.msg[1] == EVENT_ERRNO_UPDATE_ERROR_IMG_NOT_VALID) {
						lv_label_set_text(err_info,  "please download the image again");
					} else if (event_rec.msg[1] == EVENT_ERRNO_UPDATE_ERROR_WRITE) {
						lv_label_set_text(err_info, "reboot machine to have a retry.");
					} else { //unknown error, should never happen
						lv_label_set_text(err_info, "unknown error!");
					}

					lv_obj_align(err_info, bg_obj, LV_ALIGN_IN_BOTTOM_MID, 0, -50);
					break;
				default: // do nothing
					break;
			}
		}

		lv_obj_invalidate(lv_scr_act());
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}

K_THREAD_DEFINE(boot_display, DISP_THREAD_STACKSIZE, boot_disp_thread, NULL, NULL, NULL,
		DISP_THREAD_PRIORITY, 0, 0);