#include "custom_vertical_total_rows.h"
#include "gx_widget.h"
#include "gx_window.h"

VOID _gx_system_lock(VOID);
VOID _gx_system_unlock(VOID);

UINT custom_horizontal_list_total_rows_set(GX_HORIZONTAL_LIST *list, INT count)
{
	GX_WIDGET *test;
	INT row;

	GX_ENTER_CRITICAL;

	// list->gx_vertical_list_total_rows = count;
	list->gx_horizontal_list_total_columns = count;

	/* Calculate current page index. */
	// list->gx_vertical_list_top_index = 0;
	list->gx_horizontal_list_top_index = 0;

	// if count < visible, widget num should resize?

	GX_SCROLLBAR *pScroll;

	/* Make new page index visible */
	gx_vertical_list_page_index_set(list, 0);

	gx_window_scrollbar_find((GX_WINDOW *)list, GX_TYPE_VERTICAL_SCROLL,
							 &pScroll);

	if (pScroll) {
		gx_scrollbar_reset(pScroll, GX_NULL);
	}

	row = 0;

	test = _gx_widget_first_client_child_get((GX_WIDGET *)list);

	while (test) {
		/* Reset child id. */
		test->gx_widget_id = (USHORT)(LIST_CHILD_ID_START + row);
		test->gx_widget_style &= ~GX_STYLE_DRAW_SELECTED;
		list->gx_horizontal_list_callback(list, test, row);

		test = _gx_widget_next_client_child_get(test);
		row++;
	}

	GX_EXIT_CRITICAL;

	gx_horizontal_list_children_position(list);

	/* Refresh screen. */
	if (list->gx_widget_status & GX_STATUS_VISIBLE) {
		gx_system_dirty_mark((GX_WIDGET *)list);
	}
	return 0;
}