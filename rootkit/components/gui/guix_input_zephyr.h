#ifndef __GUIX_INPUT_ZEPHYR_H__
#define __GUIX_INPUT_ZEPHYR_H__
#include "gx_api.h"
#include "drivers/kscan.h"

struct event_record {
	GX_POINT current;
	GX_POINT last;
	int state;
	int drag_delta;
};

void guix_zephyr_input_dev_init(struct guix_driver *list);
#endif