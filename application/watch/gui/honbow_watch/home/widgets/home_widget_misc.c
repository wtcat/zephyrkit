#include "home_widget_misc.h"

extern void widget_music_page_init(GX_WINDOW *window);
extern void widget_compass_page_init(GX_WINDOW *window);
extern void widget_countdown_page_init(GX_WINDOW *window);
extern void widget_stopwatch_page_init(GX_WINDOW *window);
void home_widget_init(void)
{
	widget_music_page_init(NULL);
	widget_compass_page_init(NULL);
	widget_countdown_page_init(NULL);
	widget_stopwatch_page_init(NULL);
}